#!/usr/bin/env python3
"""Drive a RESIDENT ultracopier and its two local sockets, from the outside.

`harness.run()` is the contract for "run one cp/mv and judge the destination tree". It cannot
express what the CLI and listener cases need: launch the app with an ARBITRARY argv, keep it
resident, then talk to it (forward a command line to it through its single-instance socket, or
speak the catchcopy protocol on its file-manager socket) and watch what it does. That is what this
module adds -- with the SAME rules as the harness: the shipping binary is never modified, the
settings come from an isolated HOME, the process is always killed EXTERNALLY, and every wait is
bounded (no unbounded poll loop, no pgrep -f).

Two sockets are involved, both under QDir::tempPath() ($TMPDIR, else /tmp):
  * "ultracopier-<uid>-<suffix>"     -- single instance / CLI forward (LocalListener). A second
                                        `ultracopier <args>` process sends its argv here and exits
                                        with code 1; the resident one parses it (CliParser).
  * "advanced-copier-<uid>-<suffix>" -- the catchcopy v0002 protocol the patched file managers
                                        speak (ServerCatchcopy, Listener plugin).

Every instance gets its OWN socket suffix ("test-<tag>"), so sub-cases never talk to each other by
accident -- while still starting with "test", which is what harness._kill_all_ultracopier() matches
on, so a stray instance is still reaped by the rest of the suite and can never hold the real user's
socket (see [[test-socket-isolation]]).
"""
from __future__ import annotations
import os, sys, time, signal, socket, struct, subprocess, tempfile, pathlib, shutil

from . import harness as H

# A frame is: quint32 total size (this field included) | quint32 orderId | payload...
# Client->server payload is a QStringList; server->client adds a quint32 return code first.
_U32 = struct.Struct(">I")

# Reply codes of the catchcopy v0002 protocol (ServerCatchcopy.cpp)
PROTOCOL_SUPPORTED = 1000
CLIENT_REGISTERED = 1003
SERVER_NAME = 1004
COPY_FINISHED = 1005
COPY_FINISHED_WITH_ERROR = 1006
COPY_CANCELED = 1007
INCORRECT_ARGUMENT_LIST_SIZE = 5000
INCORRECT_ARGUMENT = 5001
UNKNOWN_ORDER = 5002
PROTOCOL_NOT_SUPPORTED = 5003
TRANSFER_REFUSED = 5004


def _qstringlist(items) -> bytes:
    """QDataStream (Qt_4_4) encoding of a QStringList: count, then per string its BYTE length
    followed by UTF-16BE data (a null string is length 0xFFFFFFFF)."""
    out = _U32.pack(len(items))
    for s in items:
        data = s.encode("utf-16-be")
        out += _U32.pack(len(data)) + data
    return out


def _read_qstringlist(buf: bytes, off: int):
    (count,) = _U32.unpack_from(buf, off)
    off += 4
    items = []
    for _ in range(count):
        (size,) = _U32.unpack_from(buf, off)
        off += 4
        if size == 0xFFFFFFFF:
            items.append(None)
        else:
            items.append(buf[off:off + size].decode("utf-16-be"))
            off += size
    return items, off


class Instance:
    """One resident ultracopier launched with a given argv, in an isolated HOME.

    Use as a context manager: it is ALWAYS killed externally on exit, even on assertion failure,
    so a case can never leak an instance holding a socket.
    """

    def __init__(self, tag: str, backend=H.ASYNC, args=None, cfg=None, memcheck=H.NONE,
                 display=None, extra_options=None, file_collision=H.FileCollision.OVERWRITE,
                 folder_collision=H.FolderCollision.MERGE, file_error=H.FileError.SKIP,
                 folder_error=H.FolderError.SKIP, env_extra=None, cwd=None, binary=None,
                 socket_suffix=True, tmpdir=None):
        self.tag = tag
        self.backend = backend
        self.cfg = cfg or H.load_config()
        # socket_suffix=False launches with NO ULTRACOPIER_SOCKET_SUFFIX, i.e. the SHIPPING socket
        # name -- needed when the code under test builds that name itself (the file-manager patches
        # hardcode "advanced-copier-<uid>"). Isolation then comes from a private $TMPDIR: Qt names a
        # QLocalServer under QDir::tempPath(), so our instance and the operator's real one bind in
        # different directories and can never see each other.
        self.suffix = f"{H.TEST_SOCKET_SUFFIX}-{tag}" if socket_suffix else ""
        self.tmpdir = str(tmpdir) if tmpdir else None
        # `binary` overrides the per-backend build: used by the case that drives a build made with
        # a different FEATURE FLAG (e.g. DEFINES+=ULTRACOPIER_PLUGIN_KIO), which is not one of the
        # transfer backends the harness knows how to build.
        self.binary = binary or H.binary_for(backend, self.cfg, asan=(memcheck == H.SANITIZE),
                                             tsan=(memcheck == H.TSAN))
        self.home = pathlib.Path(tempfile.mkdtemp(prefix=f"uc-cli-home-{tag}-"))
        H.write_config(self.home, file_collision=file_collision, folder_collision=folder_collision,
                       file_error=file_error, folder_error=folder_error,
                       extra_options=extra_options)
        self.env = dict(os.environ)
        self.env.update(HOME=str(self.home), XDG_CONFIG_HOME=str(self.home / ".config"))
        if self.suffix:
            self.env["ULTRACOPIER_SOCKET_SUFFIX"] = self.suffix
        else:
            self.env.pop("ULTRACOPIER_SOCKET_SUFFIX", None)
        if self.tmpdir:
            os.makedirs(self.tmpdir, exist_ok=True)
            self.env["TMPDIR"] = self.tmpdir
        if display:
            # A REAL (private Xvfb) display: needed by the clipboard commands, which read the X
            # selection. QT_QPA_PLATFORM=xcb and a CLEARED WAYLAND_DISPLAY are both mandatory --
            # on a Wayland desktop Qt6 picks the wayland plugin and IGNORES DISPLAY, so the test
            # instance would talk to (and pop windows on) the OPERATOR'S REAL SESSION, and read
            # their real clipboard instead of ours.
            self.env.update(DISPLAY=display, QT_QPA_PLATFORM="xcb")
            for var in ("WAYLAND_DISPLAY", "XDG_SESSION_TYPE", "QT_WAYLAND_RECONNECT"):
                self.env.pop(var, None)
        else:
            self.env.update(QT_QPA_PLATFORM="offscreen", DISPLAY="")
        self.memcheck = memcheck
        # A sanitizer build is 5-20x slower to start and to copy (TSan especially), so every
        # BOUNDED wait below is scaled by this factor -- the same thing the harness does to its
        # own watchdog under memcheck. The assertions are unchanged; only the patience is.
        self.slow = 6 if memcheck in (H.SANITIZE, H.TSAN, H.VALGRIND) else 1
        if memcheck == H.SANITIZE:
            self.env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=0:log_path=" + str(self.home / "asan")
        elif memcheck == H.TSAN:
            supp = pathlib.Path(__file__).resolve().parent / "tsan.supp"
            self.env["TSAN_OPTIONS"] = ("halt_on_error=0:report_thread_leaks=0:"
                                        f"suppressions={supp}:log_path=" + str(self.home / "tsan"))
        self.args = list(args or [])
        self.stderr_path = self.home / "stderr.log"
        # The working directory matters: a relative path the engine ends up building (e.g. the junk
        # folder an unsupported destination URL used to create) lands HERE, so a case can point it
        # at an empty directory and assert nothing appeared.
        self.cwd = str(cwd) if cwd else None
        self.proc = None

    # -- lifecycle ---------------------------------------------------------
    def start(self):
        self._ferr = open(self.stderr_path, "wb")
        self.proc = subprocess.Popen([self.binary] + self.args, env=self.env, cwd=self.cwd,
                                     stdout=subprocess.DEVNULL, stderr=self._ferr)
        return self

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.kill()
        return False

    def alive(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def exit_code(self):
        return None if self.proc is None else self.proc.poll()

    def kill(self):
        """External kill ONLY (the app is tray-resident and never exits by itself)."""
        if self.proc is not None and self.proc.poll() is None:
            self.proc.terminate()
            if not self.wait_exit(10):
                self.proc.kill()
                self.wait_exit(5)
        try:
            self._ferr.close()
        except Exception:
            pass
        for name in ("ultracopier", "advanced-copier"):
            try:
                os.unlink(self.socket_path("cli" if name == "ultracopier" else "catchcopy"))
            except OSError:
                pass

    def wait_exit(self, timeout: float) -> bool:
        try:
            self.proc.wait(timeout=timeout)
            return True
        except subprocess.TimeoutExpired:
            return False

    def mem_errors(self) -> int:
        """ASan/TSan errors reported in OUR code for this instance (0 on the plain lane)."""
        if self.memcheck == H.SANITIZE:
            return H._asan_errors(self.home)
        if self.memcheck == H.TSAN:
            return H._tsan_errors(self.home)
        return 0

    def stderr_text(self) -> str:
        try:
            return self.stderr_path.read_text(errors="replace")
        except OSError:
            return ""

    # -- sockets -----------------------------------------------------------
    def socket_path(self, which="cli") -> str:
        # Qt names a plain QLocalServer under QDir::tempPath(), which honors $TMPDIR -- NOT
        # always /tmp (this box exports TMPDIR, and hardcoding /tmp made every socket "missing").
        base = "ultracopier" if which == "cli" else "advanced-copier"
        name = f"{base}-{os.getuid()}-{self.suffix}" if self.suffix else f"{base}-{os.getuid()}"
        return os.path.join(self.tmpdir or H.temp_dir(), name)

    def wait_listening(self, which="cli", timeout=60.0) -> bool:
        """Wait until the socket exists AND accepts a connection (bounded, sanitizer-scaled)."""
        path = self.socket_path(which)
        deadline = time.time() + timeout * self.slow
        while time.time() < deadline:
            if self.proc is not None and self.proc.poll() is not None:
                return False
            if os.path.exists(path):
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                try:
                    s.connect(path)
                    return True
                except OSError:
                    pass
                finally:
                    s.close()
            time.sleep(0.05)
        return False

    def wait_until(self, predicate, timeout=60.0, poll=0.05) -> bool:
        """Bounded wait for a predicate (e.g. a destination file appearing), sanitizer-scaled."""
        deadline = time.time() + timeout * self.slow
        while time.time() < deadline:
            if predicate():
                return True
            time.sleep(poll)
        return bool(predicate())

    # -- the CLI-forward channel ------------------------------------------
    def cli(self, args, timeout=60) -> subprocess.CompletedProcess:
        """Run `ultracopier <args>` as a SECOND process in the same isolated environment. If this
        instance is already resident it forwards the argv over the single-instance socket and
        exits 1 ("a process is already in progress"); otherwise it would become the instance."""
        return subprocess.run([self.binary] + list(args), env=self.env, cwd=self.cwd,
                              timeout=timeout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


class Catchcopy:
    """Minimal catchcopy v0002 client -- what a patched file manager does on the paste socket."""

    def __init__(self, instance: Instance):
        self.instance = instance
        self.sock = None
        self.buffer = b""

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, *exc):
        self.close()
        return False

    def connect(self, timeout=30.0):
        path = self.instance.socket_path("catchcopy")
        deadline = time.time() + timeout
        last = None
        while time.time() < deadline:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                s.connect(path)
                self.sock = s
                return self
            except OSError as e:
                last = e
                s.close()
                time.sleep(0.05)
        raise RuntimeError(f"cannot connect to {path}: {last}")

    def close(self):
        if self.sock is not None:
            try:
                self.sock.close()
            finally:
                self.sock = None

    def send(self, order_id: int, items):
        payload = _U32.pack(order_id) + _qstringlist(items)
        self.sock.sendall(_U32.pack(len(payload) + 4) + payload)

    def read_reply(self, timeout=10.0):
        # (timeout is scaled by the instance's sanitizer factor -- see Instance.slow)
        """Return the next (orderId, returnCode, [strings]) reply, or None on timeout/EOF."""
        deadline = time.time() + timeout * self.instance.slow
        while True:
            if len(self.buffer) >= 12:
                (size,) = _U32.unpack_from(self.buffer, 0)
                if 12 <= size <= len(self.buffer):
                    frame = self.buffer[:size]
                    self.buffer = self.buffer[size:]
                    (order_id,) = _U32.unpack_from(frame, 4)
                    (code,) = _U32.unpack_from(frame, 8)
                    items, _ = _read_qstringlist(frame, 12)
                    return (order_id, code, items)
                if size < 12:
                    raise AssertionError(f"corrupted reply frame, size={size} "
                                         f"(the Qt6 8-byte size-header bug writes 0 here)")
            left = deadline - time.time()
            if left <= 0:
                return None
            self.sock.settimeout(left)
            try:
                data = self.sock.recv(65536)
            except socket.timeout:
                return None
            if not data:
                return None
            self.buffer += data

    def read_reply_for(self, order_id, timeout=30.0):
        """Reply to THIS order, skipping any that arrive first. Replies interleave: the answer to
        a cp/mv order only comes when that transfer ENDS, so a later malformed order can easily be
        answered before an earlier transfer is done."""
        deadline = time.time() + timeout * self.instance.slow
        while time.time() < deadline:
            r = self.read_reply(timeout=max(0.1, deadline - time.time()) / self.instance.slow)
            if r is None:
                return None
            if r[0] == order_id:
                return r
        return None

    def handshake(self, order_id=1):
        """protocol 0002 -> must be answered 1000."""
        self.send(order_id, ["protocol", "0002"])
        return self.read_reply()

    def order(self, order_id, items):
        self.send(order_id, items)

    def verdict(self, order_id, barrier_order_id, timeout=10.0):
        """EXACTLY what the patched file managers do: after the cp/mv order send a cheap
        "server name?" query and read replies until that barrier comes back. Returns
        (refused, replies): refused is True only if the order was answered 5004."""
        self.send(barrier_order_id, ["server", "name?"])
        replies = []
        refused = False
        deadline = time.time() + timeout * self.instance.slow
        while time.time() < deadline:
            r = self.read_reply(timeout=max(0.1, deadline - time.time()) / self.instance.slow)
            if r is None:
                break
            replies.append(r)
            if r[0] == order_id and r[1] == TRANSFER_REFUSED:
                refused = True
            if r[0] == barrier_order_id:
                break
        return refused, replies


def xvfb(tag: str):
    """Start an Xvfb the case owns (returns (proc, display) or (None, None) when Xvfb is
    missing, so the clipboard sub-cases can SKIP instead of failing the suite)."""
    if shutil.which("Xvfb") is None or shutil.which("xclip") is None:
        return None, None
    for num in range(90, 100):
        display = f":{num}"
        if os.path.exists(f"/tmp/.X11-unix/X{num}"):
            continue
        proc = subprocess.Popen(["Xvfb", display, "-screen", "0", "800x600x24", "-nolisten", "tcp"],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        deadline = time.time() + 15
        while time.time() < deadline:
            if os.path.exists(f"/tmp/.X11-unix/X{num}"):
                return proc, display
            if proc.poll() is not None:
                break
            time.sleep(0.1)
        try:
            proc.terminate()
        except OSError:
            pass
    return None, None


def set_clipboard(display: str, text: str, hold_seconds=120):
    """Put text on the CLIPBOARD selection of that display. xclip must stay alive to own the
    selection, so the returned process is killed by the caller."""
    xenv = dict(os.environ, DISPLAY=display)
    xenv.pop("WAYLAND_DISPLAY", None)
    p = subprocess.Popen(["xclip", "-selection", "clipboard", "-i"], env=xenv,
                         stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    p.stdin.write(text.encode())
    p.stdin.close()
    # xclip forks and holds the selection; give it a moment to own it before we read it back
    deadline = time.time() + 5
    while time.time() < deadline:
        got = subprocess.run(["xclip", "-selection", "clipboard", "-o"], env=xenv,
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        if got.returncode == 0 and got.stdout.decode(errors="replace").strip() == text.strip():
            return p
        time.sleep(0.1)
    return p
