#!/usr/bin/env python3
"""The two LOCAL SOCKETS Ultracopier listens on, spoken directly -- no second process involved.

This is how the outside world actually reaches a running Ultracopier, and neither channel had any
coverage:

  * `advanced-copier-<uid>` -- the **catchcopy v0002** protocol of the listener plugin
    (plugins/Listener/catchcopy). A patched Dolphin / pcmanfm-qt / the Windows Explorer extension
    connects here on Ctrl+V and sends `cp`/`mv` orders; Ultracopier replies with a return code.
  * `ultracopier-<uid>` -- the single-instance channel (LocalListener). A second `ultracopier`
    process serialises its argv into it; the resident one parses it as a command line.

The case speaks both wire formats itself (test/lib/uclaunch.py), so it pins the FRAMING as well as
the behaviour: a 4-byte big-endian total size, the order id, and a QDataStream QStringList. That
framing is not cosmetic -- ServerCatchcopy wrote the size with `out << block.size()`, which under
Qt6 is a qsizetype and emitted EIGHT bytes over the four-byte placeholder, smashing the order id
that follows it. Every reply a Qt6 build sent was therefore unparsable by its own clients; the
reply-code assertions below (and the frame check in Catchcopy.read_reply) are what catch it.

Asserted:
  * protocol handshake accepted (1000) / a wrong version refused (5003);
  * `client <name>` registers (1003), `server name?` answers (1004);
  * `cp` / `mv` / several sources really transfer, `cp-?` transfers nothing on its own;
  * malformed orders are answered, not crashed on: too few arguments (5000), an unknown verb (5002);
  * the end-of-transfer notification comes back to the client for the RIGHT order -- including
    after an unrelated CLI transfer, which used to desynchronise the two id counters so the client
    was never told anything again;
  * the single-instance socket accepts a command line, including one split across two writes
    (the recomposition path), and survives a truncated one;
  * the process is still alive and healthy at the end of all that.
"""
import sys, os, time, socket, struct, pathlib, shutil

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

PAYLOAD = b"listener-socket-payload\n"
_U32 = struct.Struct(">I")


def _mk(root: pathlib.Path, name: str, data=PAYLOAD) -> pathlib.Path:
    p = root / name
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(data)
    return p


def _fresh(root: pathlib.Path, name: str) -> pathlib.Path:
    p = root / name
    shutil.rmtree(p, ignore_errors=True)
    p.mkdir(parents=True)
    return p


class Checks:
    def __init__(self):
        self.rows = []

    def add(self, name, ok, detail=""):
        self.rows.append((name, bool(ok), detail))
        print(f"      [{'PASS' if ok else 'FAIL'}] {name}{('  -- ' + detail) if detail else ''}")
        return bool(ok)

    @property
    def ok(self):
        return all(r[1] for r in self.rows)

    def failed(self):
        return [r[0] for r in self.rows if not r[1]]


def _cli_block(args) -> bytes:
    """The single-instance wire format: quint32 total size, then a QDataStream QStringList.
    LocalListener::tryConnect() builds exactly this (argv[0] replaced by the caller's cwd)."""
    payload = _U32.pack(len(args))
    for a in args:
        data = a.encode("utf-16-be")
        payload += _U32.pack(len(data)) + data
    return _U32.pack(len(payload) + 4) + payload


def _catchcopy_forms(inst, root, c: Checks):
    src = _fresh(root, "src")
    a, b = _mk(src, "a.txt"), _mk(src, "b.txt", b"second\n")
    m = _mk(_fresh(root, "mvsrc"), "m.txt", b"to-move\n")
    d_cp, d_multi, d_mv = (_fresh(root, "cc_cp"), _fresh(root, "cc_multi"), _fresh(root, "cc_mv"))

    with U.Catchcopy(inst) as cc:
        r = cc.handshake(1)
        c.add("catchcopy: `protocol 0002` is accepted (1000)",
              r is not None and r[1] == U.PROTOCOL_SUPPORTED, f"reply={r}")

        cc.send(2, ["client", "ultracopier-test-suite"])
        r = cc.read_reply()
        c.add("catchcopy: `client <name>` registers the client (1003)",
              r is not None and r[0] == 2 and r[1] == U.CLIENT_REGISTERED, f"reply={r}")

        cc.send(3, ["server", "name?"])
        r = cc.read_reply()
        c.add("catchcopy: `server name?` answers with the server name (1004)",
              r is not None and r[0] == 3 and r[1] == U.SERVER_NAME and r[2] == ["Ultracopier"],
              f"reply={r}")

        cc.send(4, ["cp", str(a), str(d_cp)])
        c.add("catchcopy: `cp <src> <dest>` transfers the file",
              inst.wait_until(lambda: (d_cp / "a.txt").exists()
                              and (d_cp / "a.txt").read_bytes() == PAYLOAD, 60))

        cc.send(5, ["cp", str(a), str(b), str(d_multi)])
        c.add("catchcopy: `cp <src1> <src2> <dest>` transfers every source",
              inst.wait_until(lambda: (d_multi / "a.txt").exists() and (d_multi / "b.txt").exists(), 60))

        cc.send(6, ["mv", str(m), str(d_mv)])
        c.add("catchcopy: `mv <src> <dest>` moves (dest written AND source gone)",
              inst.wait_until(lambda: (d_mv / "m.txt").exists() and not m.exists(), 60),
              f"dest={(d_mv / 'm.txt').exists()} source_gone={not m.exists()}")

        # malformed orders: answered with an error code, never a crash
        cc.send(7, ["cp"])
        r = cc.read_reply_for(7)
        c.add("catchcopy: `cp` alone is refused (5000 wrong list size)",
              r is not None and r[0] == 7 and r[1] == U.INCORRECT_ARGUMENT_LIST_SIZE, f"reply={r}")

        cc.send(8, ["cp", str(a)])
        r = cc.read_reply_for(8)
        c.add("catchcopy: `cp <one argument>` is refused (5000)",
              r is not None and r[0] == 8 and r[1] == U.INCORRECT_ARGUMENT_LIST_SIZE, f"reply={r}")

        cc.send(9, ["not-an-order", "x"])
        r = cc.read_reply_for(9)
        c.add("catchcopy: an unknown verb is refused (5002)",
              r is not None and r[0] == 9 and r[1] == U.UNKNOWN_ORDER, f"reply={r}")

        cc.send(10, ["server", "something-else"])
        r = cc.read_reply_for(10)
        c.add("catchcopy: a known verb with a bad argument is refused (5001)",
              r is not None and r[0] == 10 and r[1] == U.INCORRECT_ARGUMENT, f"reply={r}")

    # The version is checked on the FIRST query of a connection (ServerCatchcopy answers that one
    # itself, before any parsing), so an incompatible client is told on its own fresh connection.
    with U.Catchcopy(inst) as cc:
        cc.send(1, ["protocol", "9999"])
        r = cc.read_reply_for(1)
        c.add("catchcopy: a client announcing another protocol version is refused (5003)",
              r is not None and r[1] == U.PROTOCOL_NOT_SUPPORTED, f"reply={r}")


def _end_notification(inst, root, c: Checks):
    """The client must be told when ITS order ends -- after a CLI transfer has already bumped the
    internal order counter. The listener used to hand Core the LISTENER PLUGIN's order id while
    keeping its own in the table, so the two only matched while both counters happened to run in
    lockstep; one CLI transfer in between broke every later notification."""
    src = _fresh(root, "notif_src")
    cli_file = _mk(src, "cli.txt")
    cc_file = _mk(src, "cc.txt")
    d_cli, d_cc = _fresh(root, "notif_cli"), _fresh(root, "notif_cc")

    inst.cli(["cp", str(cli_file), str(d_cli)])
    if not inst.wait_until(lambda: (d_cli / "cli.txt").exists(), 60):
        c.add("end notification: the CLI transfer that desynchronises the counters ran", False)
        return
    with U.Catchcopy(inst) as cc:
        cc.handshake(1)
        cc.send(2, ["cp", str(cc_file), str(d_cc)])
        if not inst.wait_until(lambda: (d_cc / "cc.txt").exists(), 60):
            c.add("end notification: the catchcopy transfer ran", False)
            return
        # the reply for order 2 must arrive on its own (finished/canceled), not be lost
        got = cc.read_reply_for(2, timeout=30)
        # 1005 FINISHED, not 1007 "canceled": a completed transfer used to be reported to every
        # client as CANCELED (Core emitted copyCanceled at Idle and copyFinished nowhere), so a
        # client could not tell a finished job from an aborted one.
        c.add("end notification: a COMPLETED transfer is reported as finished (1005), not canceled",
              got is not None and got[1] == U.COPY_FINISHED, f"reply={got}")


def _cli_socket_forms(inst, root, c: Checks):
    """The single-instance socket, written by hand: a whole block, a SPLIT block (the
    recomposition path in LocalListener::dataIncomming), and a truncated one."""
    src = _fresh(root, "sock_src")
    f1, f2, f3 = _mk(src, "s1.txt"), _mk(src, "s2.txt"), _mk(src, "s3.txt")
    d1, d2 = _fresh(root, "sock_d1"), _fresh(root, "sock_d2")
    path = inst.socket_path("cli")

    def _send(data, split_at=None):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(20)
        s.connect(path)
        if split_at is None:
            s.sendall(data)
        else:
            s.sendall(data[:split_at])
            time.sleep(0.2)          # force a second readyRead -> the recomposition branch
            s.sendall(data[split_at:])
        time.sleep(0.3)
        s.close()

    _send(_cli_block([os.getcwd(), "cp", str(f1), str(d1)]))
    c.add("single-instance socket: a command line sent as one block is executed",
          inst.wait_until(lambda: (d1 / "s1.txt").exists(), 60))

    block = _cli_block([os.getcwd(), "cp", str(f2), str(d2)])
    _send(block, split_at=len(block) // 2)
    c.add("single-instance socket: a command line SPLIT over two writes is recomposed",
          inst.wait_until(lambda: (d2 / "s2.txt").exists(), 60))

    # A bogus, huge announced size must be rejected without allocating or dying (this one is
    # answered immediately server-side, so it is checked BEFORE the truncated block below).
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(20)
    s.connect(path)
    s.sendall(_U32.pack(0xFFFFFFFF) + b"garbage-payload")
    time.sleep(1 * inst.slow)
    s.close()
    time.sleep(1 * inst.slow)
    c.add("single-instance socket: an absurd announced size is rejected", inst.alive())

    # A truncated block must simply be dropped, never crash or copy. LAST of the socket checks: the
    # server arms a 500 ms recomposition timeout for it, which can end in a modal warning and would
    # then block anything sent after it.
    truncated = _cli_block([os.getcwd(), "cp", str(f3), str(d2)])[:-8]
    _send(truncated)
    time.sleep(2 * inst.slow)
    c.add("single-instance socket: a truncated command line is dropped, not executed",
          inst.alive() and not (d2 / "s3.txt").exists())


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        # The FULL ultracopier binary cannot run under ThreadSanitizer: it dies within seconds with
        # "CHECK failed: sanitizer_thread_registry.cpp:186" (QtGui/offscreen + QThreadPool pooled
        # threads exit in a way TSan's registry cannot track -- a Qt-vs-TSan incompatibility
        # OUTSIDE our code; re-confirmed 2026-09-09, exit 66 on a plain local copy). These cases
        # DRIVE that binary, so there is nothing to run here. The engine's real TSan gate is
        # cases/tsan_engine_api.py, which hosts the same engine threads without QtGui.
        print("    [listener_socket] SKIP under TSan (the full app dies in Qt's threadpool under TSan; the\n"
              "        thread gate is cases/tsan_engine_api.py)")
        return True
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [listener_socket] SKIP (async/io_uring only)")
        return True
    backend = H.ASYNC if H.ASYNC in bes else bes[0]
    H._kill_all_ultracopier()
    root = pathlib.Path(K.fresh_dest("listener_socket"))
    c = Checks()
    try:
        with U.Instance("lsock", backend=backend, memcheck=memcheck) as inst:
            if not c.add("the catchcopy socket is listening", inst.wait_listening("catchcopy", 60)):
                return False
            c.add("the single-instance socket is listening", inst.wait_listening("cli", 30))
            _catchcopy_forms(inst, root, c)
            _end_notification(inst, root, c)
            _cli_socket_forms(inst, root, c)
            c.add("the instance survived every socket order", inst.alive())
            c.add("no memory error on the listener paths", inst.mem_errors() == 0)
        # `cp-?` opens a destination chooser that would block this instance, so it is exercised
        # on an instance of its own -- and must transfer nothing while unanswered.
        ask_dst = _fresh(root, "cc_ask_own")
        ask_src = _mk(_fresh(root, "cc_ask_src"), "ask.txt")
        with U.Instance("lsock-ask", backend=backend, memcheck=memcheck) as inst:
            if inst.wait_listening("catchcopy", 60):
                with U.Catchcopy(inst) as cc:
                    cc.handshake(1)
                    cc.send(2, ["cp-?", str(ask_src)])
                    time.sleep(4 * inst.slow)
                c.add("catchcopy: `cp-?` transfers nothing and keeps the app alive",
                      inst.alive() and ask_src.exists() and not any(ask_dst.iterdir()))
            else:
                c.add("catchcopy `cp-?`: instance is listening", False)
    finally:
        H._kill_all_ultracopier()
        shutil.rmtree(root, ignore_errors=True)
    if c.ok:
        print(f"    [listener_socket] PASS  ({len(c.rows)} socket checks on {backend})")
    else:
        print(f"    [listener_socket] FAIL  ({len(c.failed())}/{len(c.rows)}): {', '.join(c.failed())}")
    return c.ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
