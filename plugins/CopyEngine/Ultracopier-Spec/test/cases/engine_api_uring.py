#!/usr/bin/env python3
"""io_uring twin of engine_api.py: the SAME in-process driver (unit/engine_api_test.cpp) linked with
the pipelined io_uring TransferThread (unit/engine_api_uring_test.pro). Drives the public engine API
on a running io_uring transfer:
  * plain copy  -> reaches Idle, byte-identical;
  * pause       -> pause() mid-transfer HALTS the destination growth (realByteTransfered plateau),
                   then resume() completes the copy byte-identical (the pipelined pause semaphore);
  * cancel      -> cancel() mid-transfer reaches canBeDeleted() within 20 s and no PARTIAL file is
                   left (the in-flight one is removed, files finished before the cancel stay).
                   Before: stop() only interrupted the worker and the thread never went Idle from
                   Transfer/WaitForTheTransfer -> the engine was never freed (leak + at-exit hang).
The io_uring data plane bypasses libc, so no shim slow-down: a large source keeps the copy in flight
long enough for the cancel to land mid-way. Skipped where liburing is unavailable."""
import sys, os, re, pathlib, subprocess, tempfile, shutil
_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

_UNIT_DIR = pathlib.Path(__file__).resolve().parents[1] / "unit"
_PRO = _UNIT_DIR / "engine_api_uring_test.pro"


def _qmake() -> str:
    return shutil.which("qmake6") or shutil.which("qmake") or "qmake6"


def _build() -> str:
    bdir = pathlib.Path(tempfile.mkdtemp(prefix="uc-engine-api-uring-"))
    q = subprocess.run([_qmake(), "-o", str(bdir / "Makefile"), str(_PRO),
                        "CONFIG+=release", "-spec", "linux-g++"], capture_output=True, text=True)
    if q.returncode != 0:
        raise RuntimeError(f"qmake failed:\n{q.stderr[-1500:]}")
    m = subprocess.run(["make", "-C", str(bdir), f"-j{os.cpu_count()}"], capture_output=True, text=True)
    binp = bdir / "engine_api_uring_test"
    if m.returncode != 0 or not binp.exists():
        errs = "\n".join(l for l in m.stdout.splitlines() + m.stderr.splitlines()
                         if "error:" in l or "undefined reference" in l)
        raise RuntimeError(f"build failed:\n{errs[-2000:]}")
    return str(binp)


def _mk_src(name: str, nfiles: int, fsize: int) -> str:
    src = pathlib.Path(K.fresh_src_root(name))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    for i in range(nfiles):
        (src / f"f{i}.dat").write_bytes(bytes(((i * 131 + j * 7) & 0xFF) for j in range(fsize)))
    return str(src)


def _run(binp, src, dest, env, mode=None):
    argv = [binp, src, dest] + ([mode] if mode else [])
    r = subprocess.run(argv, env=env, capture_output=True, text=True, timeout=140)
    subprocess.run(["pkill", "-9", "-x", "engine_api_uring_test"], capture_output=True)
    return r.returncode, r.stdout + r.stderr


def _is_prefix(src_file, dst_file) -> bool:
    """A kept partial must be a faithful PREFIX of its source (never wrong bytes)."""
    with open(dst_file, "rb") as d, open(src_file, "rb") as s:
        got = d.read()
        return got == s.read(len(got))


def _diff_ok(src, dest) -> bool:
    copied = os.path.join(dest, os.path.basename(src))
    return subprocess.run(["diff", "-rq", "--no-dereference", src, copied], capture_output=True).returncode == 0


def run(backends=None, memcheck=H.NONE) -> bool:
    if backends is not None and H.IO_URING not in backends:
        print("    [skip] engine_api_uring is io_uring-only")
        return True
    if subprocess.run(["pkg-config", "--exists", "liburing"]).returncode != 0:
        print("    [skip] liburing unavailable")
        return True
    try:
        binp = _build()
    except RuntimeError as e:
        print(f"    {e}")
        return False
    env = dict(os.environ, HOME="/tmp/uc-engine-api-uring-home", QT_QPA_PLATFORM="offscreen", DISPLAY="")
    results = []

    src = _mk_src("engapiu_plain_src", 8, 60000)
    dest = K.fresh_dest("engapiu_plain_dest")
    rc, out = _run(binp, src, dest, env)
    m = re.search(r"DONE done=(\d)", out)
    done = bool(m and m.group(1) == "1")
    content = _diff_ok(src, dest)
    ok1 = (rc == 0 and done and content)
    print(f"      [plain]  done={done} content_identical={content} -> {'PASS' if ok1 else 'FAIL'}")
    results.append(ok1)

    # pause mid-flight: dest growth must plateau while paused, then complete on resume
    src = _mk_src("engapiu_pause_src", 6, 48 * 1024 * 1024)
    dest = K.fresh_dest("engapiu_pause_dest")
    rc, out = _run(binp, src, dest, env, mode="pause")
    mp = re.search(r"PAUSE isInPause=(\d) drained=(\d+) plateau=(\d+) halted=(\d)", out)
    halted = bool(mp and mp.group(4) == "1")
    done_p = bool(re.search(r"DONE done=1", out))
    content_p = _diff_ok(src, dest)
    ok_p = (rc == 0 and halted and done_p and content_p)
    print(f"      [pause]  halted_while_paused={halted} done={done_p} content_identical={content_p} -> {'PASS' if ok_p else 'FAIL'}")
    if not ok_p:
        for l in out.strip().splitlines()[-3:]:
            print(f"        {l}")
    results.append(ok_p)
    shutil.rmtree(src, ignore_errors=True); shutil.rmtree(dest, ignore_errors=True)

    # cancel mid-flight: the partial is REMOVED with deletePartiallyTransferredFiles (default) and KEPT
    # as a faithful, contiguous prefix without it (UC_TEST_KEEP_PARTIALS=1; trimDestinationToContiguous)
    for keep in (False, True):
        tag = "engapiu_cancel_keep" if keep else "engapiu_cancel"
        src = _mk_src(tag + "_src", 6, 64 * 1024 * 1024)   # 384 MB: in flight long enough to cancel
        dest = K.fresh_dest(tag + "_dest")
        env_c = dict(env, UC_TEST_KEEP_PARTIALS="1") if keep else env
        rc, out = _run(binp, src, dest, env_c, mode="cancel")
        mc = re.search(r"CANCEL at destBytes=(\d+)/(\d+)", out)
        md = re.search(r"DONE done=(\d) canDelete=(\d)", out)
        cancelled = bool(mc)
        can_delete = bool(md and md.group(2) == "1")
        copied = pathlib.Path(dest) / os.path.basename(src)
        partials = [f.name for f in (copied.iterdir() if copied.is_dir() else [])
                    if f.is_file() and f.stat().st_size != (pathlib.Path(src) / f.name).stat().st_size]
        prefix_ok = all(_is_prefix(pathlib.Path(src) / n, copied / n) for n in partials)
        partial_state = (bool(partials) and prefix_ok) if keep else (not partials)
        ok2 = (rc == 0 and cancelled and can_delete and partial_state)
        print(f"      [cancel{' keep-partials' if keep else ''}] cancelled_midway={cancelled} "
              f"canBeDeleted={can_delete} partials={partials} prefix_ok={prefix_ok} -> {'PASS' if ok2 else 'FAIL'}")
        if not ok2:
            for l in out.strip().splitlines()[-3:]:
                print(f"        {l}")
        results.append(ok2)
        shutil.rmtree(src, ignore_errors=True); shutil.rmtree(dest, ignore_errors=True)

    ok = all(results)
    print(f"    [engine_api_uring] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
