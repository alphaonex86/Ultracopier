#!/usr/bin/env python3
"""DATA-LOSS: skipping a multi-chunk read-fault file must NOT drop a sibling multi-chunk GOOD file.

faulty_hdd proved (async, fileError=SKIP) that when a >1 MiB BADSECTOR file faults mid-pipeline and is
SKIPped, the good multi-chunk file `over_1mib.dat` goes MISSING from the destination, while every small
good file copies fine. This is the core dying-disk failure to eliminate: lose good data because a sibling
sat on a dead sector. (PUT_TO_END does not lose it; only SKIP does.)

Minimal isolation: a handful of small good files + one multi-chunk GOOD file BEFORE and one AFTER a
multi-chunk read-fault file, copied with fileError=SKIP and a libc readfail on the bad file. REQUIRED:
the process completes, the BADSECTOR file is skipped, and BOTH multi-chunk good files are present and
byte-identical. (async only -- the libc readfail shim does not reach io_uring/IOCP.)
"""
import os
import sys
import shutil
import pathlib
import filecmp

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

MB = 1024 * 1024


def _seeded(n, seed):
    b = bytearray(n)
    x = seed & 0xFFFFFFFF
    for i in range(n):
        x = (1103515245 * x + 12345) & 0xFFFFFFFF
        b[i] = (x >> 16) & 0xFF
    return bytes(b)


def run(backends=None, memcheck=H.NONE) -> bool:
    src = pathlib.Path(K.fresh_src_root("skipmc_src"))
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    # ordering by name (scan order is roughly directory order); a multi-chunk good file on each side
    files = {
        "a_small1.bin": _seeded(4096, 1),
        "b_good_multichunk_before.bin": _seeded(MB + 4096, 2),
        "c_small2.bin": _seeded(4096, 3),
        "m_BADSECTOR_dead.bin": _seeded(MB + 4096, 4),
        "n_small3.bin": _seeded(4096, 5),
        "z_good_multichunk_after.bin": _seeded(MB + 4096, 6),
    }
    for name, data in files.items():
        (src / name).write_bytes(data)
    good = [n for n in files if "BADSECTOR" not in n]

    dest = K.fresh_dest("skipmc_dest")
    base = os.path.basename(str(src))
    copied = pathlib.Path(dest, base)

    K.with_scenario("readfail:BADSECTOR")
    r = H.run(H.ASYNC, "cp", [str(src)], dest,
              file_collision=H.FileCollision.OVERWRITE,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP,
              expect_dir=None,
              fs_preload=K.fs_so())
    K.with_scenario("")

    missing = []
    differs = []
    for n in good:
        dp = copied / n
        if not dp.exists():
            missing.append(n)
        elif not filecmp.cmp(str(src / n), str(dp), shallow=False):
            differs.append(n)
    good_ok = not missing and not differs
    ok = (r.completed and r.stayed_alive and good_ok
          and not r.oom_killed and r.mem_errors == 0)
    print(f"      completed={r.completed} alive={r.stayed_alive} "
          f"good_ok={good_ok} mem_err={r.mem_errors}")
    if missing:
        print(f"        DATA LOSS: missing good files: {missing}")
    if differs:
        print(f"        DATA LOSS: good files differ: {differs}")
    print(f"    [skip_drops_multichunk:async] {'PASS' if ok else 'FAIL'}")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
