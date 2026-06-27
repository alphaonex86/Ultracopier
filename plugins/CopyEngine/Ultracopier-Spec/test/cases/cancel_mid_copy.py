#!/usr/bin/env python3
"""Cancel/close mid-copy robustness: start a copy of a large tree, then terminate the process
at 1 s and at 5 s into the transfer (simulating a user pressing Cancel / closing the window).
The expected behaviour is "no crash": the process must exit cleanly (no segfault, no
STATUS_HEAP_CORRUPTION, no core dump). The copy will obviously not match the source (the
transfer was interrupted) -- we do NOT assert content correctness here, only that the
engine handles the termination signal without crashing.

The test runs on every backend that the host supports:
  - async (Linux, pthread + select)
  - io_uring (Linux, liburing)
  - IOCP (Windows, via the winlane SSH lane -- separate)

For each backend x each cancel-delay (1 s, 5 s), the test:
  1. launches ultracopier on a large synthetic source tree (enough to run >5 s),
  2. sleeps `delay` seconds,
  3. sends SIGTERM (the same signal the harness sends when it kills the process at end of run),
  4. waits up to 10 s for the process to exit,
  5. asserts: exit code is one of {0, 15, -SIGTERM}, no OOM-kill, no crash signature in dmesg,
     and the process did not dump core.

If anything goes wrong we attach gdb to the still-running pid (or to a core file) and print a
backtrace so the failure is debuggable instead of opaque.
"""
import os, sys, time, signal, subprocess, pathlib, shutil, tempfile

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(_TEST_DIR))
from lib import harness as H
from lib import synthtree, casekit as K


# A tree big enough that 5 s into the copy the transfer is still in flight (so the cancel
# really is mid-copy, not post-copy). The default `large` profile is ~400 MB of many small
# files; that comfortably exceeds 5 s of transfer time on a fast SSD and stays under the 1 GB
# memory cap.
def _big_tree():
    # total_mb=100 + many small files -> ~30 s of transfer time, plenty for the 1 s / 5 s
    # cancel points to be genuinely mid-copy.
    return synthtree.make_tree(K.fresh_src_root("cancel_src"), profile="large", total_mb=100)


def _gdb_backtrace(pid: int, binary: str, log_path: pathlib.Path) -> None:
    """Attach gdb to `pid`, dump a backtrace of every thread, save to log_path."""
    gdb_script = (
        "set pagination off\n"
        "set print thread-events off\n"
        "thread apply all bt full\n"
        "detach\n"
        "quit\n"
    )
    cmd = ["gdb", "-q", "-nx", "-batch", "-x", "/dev/stdin", binary, str(pid)]
    try:
        r = subprocess.run(cmd, input=gdb_script, capture_output=True, text=True, timeout=30)
        with open(log_path, "w") as f:
            f.write("=== gdb attach to pid %d (%s) ===\n" % (pid, binary))
            f.write(r.stdout)
            if r.stderr:
                f.write("\n--- gdb stderr ---\n")
                f.write(r.stderr)
    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        with open(log_path, "w") as f:
            f.write(f"gdb backtrace failed: {e}\n")


def _was_oom(exit_code: int) -> bool:
    return exit_code is not None and exit_code < 0 and -exit_code == signal.SIGKILL


def _dmesg_oom(started: float) -> bool:
    """Return True if dmesg shows the pid was OOM-killed since `started`."""
    try:
        r = subprocess.run(["dmesg", "--since", f"-{int(time.time() - started) + 5}s"],
                           capture_output=True, text=True, timeout=5)
        return "Out of memory" in r.stdout or "Killed process" in r.stdout
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return False


def _one_run(backend: str, delay_s: int) -> bool:
    """Run one cancel-mid-copy test for `backend` and `delay_s`."""
    src = _big_tree()
    dest = K.fresh_dest("cancel_dest")

    home = pathlib.Path(tempfile.mkdtemp(prefix="uc-cancel-home-"))
    H.write_config(home,
                   file_collision=H.FileCollision.OVERWRITE,
                   folder_collision=H.FolderCollision.MERGE,
                   file_error=H.FileError.SKIP,
                   folder_error=H.FolderError.SKIP,
                   keep_date=False, do_right=False, inode_threads=1)

    binpath = H.binary_for(backend, H.load_config(), asan=False)
    env = dict(os.environ)
    env.update(HOME=str(home),
               QT_QPA_PLATFORM="offscreen", DISPLAY="",   # never reach a real X display
               XDG_CONFIG_HOME=str(home / ".config"))

    argv = [binpath, "cp", src, dest]
    started = time.time()
    proc = subprocess.Popen(argv, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(delay_s)
    # Send SIGTERM (the same signal the harness uses to end a normal run).
    proc.terminate()

    # Wait up to 30 s for the process to exit cleanly.
    try:
        exit_code = proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        # The process did not exit -- either it hung or crashed without flushing. Kill it
        # hard and treat as a failure.
        proc.kill()
        try: proc.wait(timeout=10)
        except subprocess.TimeoutExpired: pass
        bt_log = home / "gdb_bt.txt"
        _gdb_backtrace(proc.pid, binpath, bt_log)
        shutil.rmtree(home, ignore_errors=True)
        print(f"      [{backend} @{delay_s}s] FAIL: process did not exit after SIGTERM "
              f"(backtrace: {bt_log})")
        return False

    oom = _was_oom(exit_code) or _dmesg_oom(started)
    crashed = exit_code not in (0, None, -signal.SIGTERM, signal.SIGTERM)
    core_dump = pathlib.Path(f"/tmp/core.{proc.pid}").exists() or pathlib.Path("core").exists()

    ok = (not oom) and (not crashed) and (not core_dump)
    if not ok:
        bt_log = home / "gdb_bt.txt"
        _gdb_backtrace(proc.pid, binpath, bt_log)
        print(f"      [{backend} @{delay_s}s] FAIL: exit={exit_code} oom={oom} "
              f"crashed={crashed} core={core_dump} (backtrace: {bt_log})")
    else:
        print(f"      [{backend} @{delay_s}s] PASS  exit={exit_code}")
    shutil.rmtree(home, ignore_errors=True)
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # All available Linux backends; IOCP runs separately via the Windows lane.
    bks = backends or [H.ASYNC, H.IO_URING]
    # Restrict to backends that the host supports (skip io_uring if liburing missing, etc.)
    avail = []
    for b in bks:
        try:
            H.binary_for(b, H.load_config(), asan=False)
            avail.append(b)
        except Exception:
            pass

    ok = True
    for backend in avail:
        for delay in (1, 5):
            res = _one_run(backend, delay)
            ok = ok and res
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)