#!/usr/bin/env python3
"""EVERY command-line argument Ultracopier accepts, driven end to end on the real binary.

The CLI is a first-class entry point (the .desktop files, the file-manager service menus and the
single-instance forward all go through `CliParser::cli()`), yet it had NO automated coverage at all.
Each argument form below is launched for real -- either as the FIRST instance (argv parsed at
startup) or forwarded to a resident one over the single-instance socket -- and judged from the
outside: what landed on disk, whether the process is still alive, what it printed, its exit code.

Covered (CliParser.cpp, in order): no argument, `quit`, `uninstall`, `--help`, `--options`, a
`*.urc` plugin file, `Transfer-list <file>`, `CBcp`/`CBmv <dest>` (clipboard), `cp`/`Copy` and
`mv`/`Move` with a destination, with `?` (ask the user) and with several sources, plus the
malformed forms that must be REJECTED cleanly instead of crashing.

Two real defects this case pins down (both RED before the fix in CliParser.cpp):
  * `Transfer-list <any other file>` **SEGFAULTED** (exit -11): the header was split on ';' and
    field [3] indexed without checking there were 4 fields.
  * `Transfer-list <a list Ultracopier exported itself>` transferred NOTHING: the header's 4th
    field is the FORMAT marker "Ultracopier", but it was passed on as the copy-engine NAME, and the
    engine is named "Ultracopier Spec" -> "Cannot find any engine with this name" every time.

Modal-dialog note: several forms legitimately end in a blocking dialog (help, "command not valid",
`?` asking for a destination). A blocked instance cannot answer a further command, so every such
form gets its OWN instance -- and every instance is killed EXTERNALLY, as the shipping app never
exits by itself.
"""
import sys, os, time, pathlib, shutil

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

PAYLOAD = b"cli-argument-payload\n"


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
    """Collect (name, ok, detail) so one failing form never hides the others."""

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


def _resident_forms(root, backend, memcheck, c: Checks):
    """Forms that succeed silently -> one resident instance can serve them all (fast)."""
    src = _fresh(root, "src")
    a = _mk(src, "a.txt")
    b = _mk(src, "b.txt", b"second-source\n")
    m = _mk(_fresh(root, "mvsrc"), "m.txt", b"to-move\n")
    d_cp, d_multi, d_mv = _fresh(root, "d_cp"), _fresh(root, "d_multi"), _fresh(root, "d_mv")
    d_tl = _fresh(root, "d_tl")
    tl = root / "list.uc"
    tl.write_text("Ultracopier;Transfer-list;Copy;Ultracopier\n"
                  f"{a};{d_tl / 'from_list.txt'}\n"
                  f"{b};{d_tl / 'from_list2.txt'}\n")

    with U.Instance("cli-res", backend=backend, memcheck=memcheck) as inst:
        c.add("no argument: single-instance socket is listening", inst.wait_listening("cli", 60))
        c.add("no argument: catchcopy socket is listening", inst.wait_listening("catchcopy", 30))

        r = inst.cli(["cp", str(a), str(d_cp)])
        c.add("`cp <src> <dest>` forwards to the running instance (exit 1)", r.returncode == 1,
              f"exit={r.returncode}")
        c.add("`cp <src> <dest>` copies the file",
              inst.wait_until(lambda: (d_cp / "a.txt").read_bytes() == PAYLOAD if (d_cp / "a.txt").exists() else False, 60))

        inst.cli(["Copy", str(a), str(d_multi)])
        inst.cli(["cp", str(a), str(b), str(d_multi)])
        c.add("`cp <src1> <src2> <dest>` copies every source",
              inst.wait_until(lambda: (d_multi / "a.txt").exists() and (d_multi / "b.txt").exists(), 60))

        inst.cli(["mv", str(m), str(d_mv)])
        c.add("`mv <src> <dest>` moves (dest written AND source gone)",
              inst.wait_until(lambda: (d_mv / "m.txt").exists() and not m.exists(), 60),
              f"dest={(d_mv / 'm.txt').exists()} source_gone={not m.exists()}")

        inst.cli(["Transfer-list", str(tl)])
        c.add("`Transfer-list <exported list>` runs the listed transfers",
              inst.wait_until(lambda: (d_tl / "from_list.txt").exists()
                              and (d_tl / "from_list2.txt").exists(), 60),
              "was broken: the header's format marker was used as the engine NAME")

        c.add("the instance survived every forwarded command", inst.alive())
        c.add("no memory error on the CLI paths", inst.mem_errors() == 0)


def _one_shot(tag, args, root, backend, memcheck, c: Checks, *, expect_exit=None,
              expect_alive=True, settle=4.0, check=None, name=None):
    """Launch one instance with `args`, let it settle, then judge it."""
    inst = U.Instance(tag, backend=backend, args=args, memcheck=memcheck).start()
    try:
        label = name or ("`" + " ".join(args) + "`")
        if expect_exit is not None:
            exited = inst.wait_exit(30)
            c.add(f"{label} exits", exited and inst.exit_code() == expect_exit,
                  f"exited={exited} code={inst.exit_code()}")
        else:
            # Wait for the app to actually BE UP (its single-instance socket is bound early in
            # startup) before the settle, so a slow box does not turn a negative assertion
            # ("nothing was transferred") into a vacuous one taken before argv was even parsed.
            inst.wait_listening("cli", 60)
            deadline = time.time() + settle * inst.slow
            while time.time() < deadline and inst.alive():
                time.sleep(0.2)
            c.add(f"{label} does not crash", inst.alive() == expect_alive,
                  f"alive={inst.alive()} code={inst.exit_code()}")
        if check is not None:
            check(inst, c)
    finally:
        inst.kill()


def _single_forms(root, backend, memcheck, c: Checks):
    src = _fresh(root, "src1")
    a = _mk(src, "one.txt")

    # --help: the help text really is printed (qDebug -> stderr), and the app keeps running.
    # WAITED FOR, not slept for: under load the app can take a while to reach the CLI parsing, and
    # a fixed sleep would either false-fail or pass vacuously (see the load-immunity rule).
    def _help(inst, c):
        c.add("`--help` prints the argument list",
              inst.wait_until(lambda: "The arguments possible are:" in inst.stderr_text()
                              and "cp [source" in inst.stderr_text(), 60))
    _one_shot("cli-help", ["--help"], root, backend, memcheck, c, check=_help)

    _one_shot("cli-opts", ["--options"], root, backend, memcheck, c)

    # quit / uninstall: the ONLY arguments that make the process exit by itself
    _one_shot("cli-quit", ["quit"], root, backend, memcheck, c, expect_exit=0)
    _one_shot("cli-uninst", ["uninstall"], root, backend, memcheck, c, expect_exit=0)

    # an unknown argument must be reported, not swallowed and not fatal
    def _unknown(inst, c):
        c.add("an unknown argument prints the help",
              inst.wait_until(lambda: "The arguments possible are:" in inst.stderr_text(), 60))
    _one_shot("cli-unknown", ["--nonsense"], root, backend, memcheck, c, check=_unknown)

    # truncated transfer commands: rejected, never a crash
    _one_shot("cli-cponly", ["cp"], root, backend, memcheck, c)
    _one_shot("cli-cp1", ["cp", str(a)], root, backend, memcheck, c)

    # "?" = ask the user for the destination: nothing may be transferred on its own
    ask_dst = _fresh(root, "ask_dst")
    def _ask(inst, c):
        c.add("`cp <src> ?` transfers nothing without an answer",
              not any(ask_dst.iterdir()) and a.exists())
    _one_shot("cli-cpask", ["cp", str(a), "?"], root, backend, memcheck, c, check=_ask)

    mv_src = _mk(_fresh(root, "askmv"), "keep.txt")
    def _askmv(inst, c):
        c.add("`mv <src> ?` keeps the source until the user answers", mv_src.exists())
    _one_shot("cli-mvask", ["mv", str(mv_src), "?"], root, backend, memcheck, c, check=_askmv)

    # Transfer-list rejections (the SEGFAULT regression guard)
    bad = root / "not-a-list.txt"
    bad.write_text("hello, I am not a transfer list\n")
    _one_shot("cli-badtl", ["Transfer-list", str(bad)], root, backend, memcheck, c,
              name="`Transfer-list <a file that is not one>` (was a SEGFAULT)")
    empty = root / "empty-list.txt"
    empty.write_bytes(b"")
    _one_shot("cli-emptytl", ["Transfer-list", str(empty)], root, backend, memcheck, c)
    wrong = root / "wrong-header.txt"
    wrong.write_text("Ultracopier;Transfer-list;Copy;SomeOtherEngine\n")
    _one_shot("cli-wrongtl", ["Transfer-list", str(wrong)], root, backend, memcheck, c)
    _one_shot("cli-notl", ["Transfer-list", str(root / "does-not-exist.uc")], root, backend, memcheck, c)

    # a plugin file that is not there must not take the app down either
    _one_shot("cli-urc", [str(root / "missing-plugin.urc")], root, backend, memcheck, c)


def _second_instance(root, backend, memcheck, c: Checks):
    """A second `ultracopier` with NO argument tells the user one is already running, and `quit`
    sent that way must stop the RESIDENT one (the only argument that acts on another process)."""
    with U.Instance("cli-second", backend=backend, memcheck=memcheck) as inst:
        if not inst.wait_listening("cli", 60):
            c.add("second instance: resident is listening", False)
            return
        r = inst.cli([])
        c.add("a second instance with no argument forwards and exits 1", r.returncode == 1,
              f"exit={r.returncode}")
        c.add("the resident instance survives it", inst.alive())

    with U.Instance("cli-quitfwd", backend=backend, memcheck=memcheck) as inst:
        if not inst.wait_listening("cli", 60):
            c.add("forwarded quit: resident is listening", False)
            return
        inst.cli(["quit"])
        c.add("`quit` from a second instance stops the resident one",
              inst.wait_exit(30), f"code={inst.exit_code()}")


def _clipboard_forms(root, backend, memcheck, c: Checks):
    """CBcp / CBmv read the X CLIPBOARD. They need a real display and a selection owner, so this
    runs on a private Xvfb with xclip; where either is missing the forms are reported as skipped
    (they cannot be exercised, and pretending otherwise would be a fake pass)."""
    xproc, display = U.xvfb("cli")
    if display is None:
        print("      [SKIP] CBcp/CBmv: no Xvfb+xclip on this box")
        return
    holder = None
    try:
        cb_src = _fresh(root, "cbsrc")
        f1 = _mk(cb_src, "clip1.txt", b"clipboard-copy\n")
        f2 = _mk(cb_src, "clip2.txt", b"clipboard-move\n")
        d_cp, d_mv = _fresh(root, "cb_cp"), _fresh(root, "cb_mv")
        holder = U.set_clipboard(display, f"file://{f1}\nfile://{f2}\n")
        with U.Instance("cli-clip", backend=backend, memcheck=memcheck, display=display) as inst:
            if not inst.wait_listening("cli", 60):
                c.add("clipboard: resident is listening", False)
                return
            inst.cli(["CBcp", str(d_cp)])
            c.add("`CBcp <dest>` copies what the clipboard holds",
                  inst.wait_until(lambda: (d_cp / "clip1.txt").exists()
                                  and (d_cp / "clip2.txt").exists(), 60),
                  f"got={sorted(os.listdir(d_cp))}")
            inst.cli(["CBmv", str(d_mv)])
            c.add("`CBmv <dest>` moves what the clipboard holds",
                  inst.wait_until(lambda: (d_mv / "clip1.txt").exists()
                                  and (d_mv / "clip2.txt").exists()
                                  and not f1.exists() and not f2.exists(), 60),
                  f"got={sorted(os.listdir(d_mv))} sources_gone={not f1.exists() and not f2.exists()}")
    finally:
        for p in (holder, xproc):
            if p is not None:
                try:
                    p.terminate()
                    p.wait(timeout=10)
                except Exception:
                    pass


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        # The FULL ultracopier binary cannot run under ThreadSanitizer: it dies within seconds with
        # "CHECK failed: sanitizer_thread_registry.cpp:186" (QtGui/offscreen + QThreadPool pooled
        # threads exit in a way TSan's registry cannot track -- a Qt-vs-TSan incompatibility
        # OUTSIDE our code; re-confirmed 2026-09-09, exit 66 on a plain local copy). These cases
        # DRIVE that binary, so there is nothing to run here. The engine's real TSan gate is
        # cases/tsan_engine_api.py, which hosts the same engine threads without QtGui.
        print("    [cli_arguments] SKIP under TSan (the full app dies in Qt's threadpool under TSan; the\n"
              "        thread gate is cases/tsan_engine_api.py)")
        return True
    # The CLI parser is backend-independent (it never touches the transfer implementation), so one
    # Linux backend is enough -- running the same argv matrix twice would only double the time.
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [cli_arguments] SKIP (async/io_uring only)")
        return True
    backend = H.ASYNC if H.ASYNC in bes else bes[0]
    H._kill_all_ultracopier()
    root = pathlib.Path(K.fresh_dest("cli_args"))
    c = Checks()
    try:
        _resident_forms(root, backend, memcheck, c)
        _single_forms(root, backend, memcheck, c)
        _second_instance(root, backend, memcheck, c)
        _clipboard_forms(root, backend, memcheck, c)
    finally:
        H._kill_all_ultracopier()
        shutil.rmtree(root, ignore_errors=True)
    if c.ok:
        print(f"    [cli_arguments] PASS  ({len(c.rows)} argument forms on {backend})")
    else:
        print(f"    [cli_arguments] FAIL  ({len(c.failed())}/{len(c.rows)}): {', '.join(c.failed())}")
    return c.ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
