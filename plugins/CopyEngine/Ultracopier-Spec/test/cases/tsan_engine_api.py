#!/usr/bin/env python3
"""ThreadSanitizer gate over the REAL engine: build the engine_api_test unit driver with
-fsanitize=thread and run a real multi-file copy through the real CopyEngine/ListThread/
TransferThread stack. ASSERTS: rc==0 (also proves no TSan thread-registry CHECK crash),
dest byte-identical, and ZERO TSan reports touching our code.

WHY THE UNIT DRIVER, NOT THE FULL APP: the full ultracopier binary under TSan dies with
`CHECK failed: sanitizer_thread_registry.cpp` -- QtGui/offscreen + QThreadPool expired
threads exit in a way TSan's registry can't track (pthread_t reuse), a Qt-vs-TSan
incompatibility OUTSIDE our code. The unit driver hosts the identical engine threads
(ListThread + TransferThreads + reader/writer) without QtGui/tray/threadpool, so engine
races are fully visible and the run is stable. Established 2026-07-02: a real copy through
this gate is CLEAN (0 reports) -- the ~60 reports the first full-app run showed were all the
glib-eventfd wakeup false-positive class, suppressed in lib/tsan.supp (third-party-internal
only; an our-vs-our race would still be reported and MUST be fixed, never suppressed).

Backend-independent gate (the async engine threads are the subject); runs once, not per
requested backend. Slowish (TSan build + ~5-15x runtime) -- excluded from --quick by timing.
"""
import sys, os, re, pathlib, subprocess, tempfile, shutil, glob
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "engine_api_test.pro"
_SUPP = pathlib.Path(__file__).resolve().parents[1] / "lib" / "tsan.supp"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build() -> str:
    """Build (or incrementally re-make) the TSan-instrumented unit driver in a stable dir so
    rebuilds are cheap; a missing Makefile regenerates with the TSan flags baked in."""
    bdir = pathlib.Path(tempfile.gettempdir()) / "uc-tsan-engine-api"
    bdir.mkdir(parents=True, exist_ok=True)
    if not (bdir / "Makefile").exists():
        q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                            "-spec", "linux-g++", "CONFIG+=release", "CONFIG+=nodebug",
                            "QMAKE_CXXFLAGS+=-fsanitize=thread -fno-omit-frame-pointer -g",
                            "QMAKE_LFLAGS+=-fsanitize=thread"],
                           capture_output=True, text=True)
        if q.returncode != 0:
            raise RuntimeError("qmake failed:\n" + q.stderr[-2000:])
    m = subprocess.run(["make", "-C", str(bdir), f"-j{os.cpu_count()}"],
                       capture_output=True, text=True)
    binp = bdir / "engine_api_test"
    if m.returncode != 0 or not binp.exists():
        raise RuntimeError("make failed:\n" + (m.stderr or m.stdout)[-2000:])
    return str(binp)


def run(backends=None, memcheck=H.NONE) -> bool:
    try:
        binp = _build()
    except Exception as e:
        print(f"    [tsan_engine_api] BUILD FAILED: {e}")
        return False

    src = K.fresh_src_root("tsan_gate_src")
    shutil.rmtree(src, ignore_errors=True)
    os.makedirs(os.path.join(src, "sub"))
    with open(os.path.join(src, "a.bin"), "wb") as f:
        f.write(os.urandom(8 * 1024 * 1024))
    with open(os.path.join(src, "sub", "b.bin"), "wb") as f:
        f.write(os.urandom(8 * 1024 * 1024))
    dest = K.fresh_dest("tsan_gate_dest")
    logdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-tsan-gate-log-"))
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", DISPLAY="",
               TSAN_OPTIONS=("halt_on_error=0:report_thread_leaks=0:"
                             f"suppressions={_SUPP}:log_path={logdir}/tsan"))
    try:
        r = subprocess.run([binp, src, dest], env=env,
                           capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        subprocess.run(["pkill", "-9", "-x", "engine_api_test"], capture_output=True)
        print("    [tsan_engine_api] FAIL: timed out (livelock under TSan?)")
        shutil.rmtree(logdir, ignore_errors=True)
        return False
    subprocess.run(["pkill", "-9", "-x", "engine_api_test"], capture_output=True)

    problems = []
    if r.returncode != 0:
        problems.append(f"rc={r.returncode} (a TSan registry CHECK crash also lands here); "
                        f"stderr tail: {r.stderr[-200:]}")
    total = ours = 0
    our_sample = ""
    for f in glob.glob(str(logdir / "tsan*")):
        t = open(f, errors="ignore").read()
        for block in t.split("=================="):
            if "WARNING: ThreadSanitizer:" in block:
                total += 1
                if any(k in block for k in ("Ultracopier-Spec", "TransferThread", "ListThread")):
                    ours += 1
                    if not our_sample:
                        our_sample = "\n".join(block.strip().splitlines()[:10])
    if ours:
        problems.append(f"{ours} TSan report(s) touching OUR code (fix the race, never "
                        f"suppress it):\n{our_sample}")
    d = subprocess.run(["diff", "-rq", "--no-dereference", src,
                        os.path.join(dest, os.path.basename(src))],
                       capture_output=True, text=True)
    if d.returncode != 0:
        problems.append("dest not byte-identical:\n" + d.stdout[:300])
    print(f"      rc={r.returncode} tsan_reports={total} our_code={ours} diff_ok={d.returncode == 0}")
    shutil.rmtree(logdir, ignore_errors=True)
    if problems:
        print("    [tsan_engine_api] FAIL: " + "; ".join(problems))
        return False
    print("    [tsan_engine_api] PASS  (real engine copy TSan-clean)")
    return True


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
