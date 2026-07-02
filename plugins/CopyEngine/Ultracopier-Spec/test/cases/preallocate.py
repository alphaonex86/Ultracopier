#!/usr/bin/env python3
"""Destination preallocation (anti-fragmentation) — io_uring / IOCP only.

The pipelined backends reserve a FRESH destination file's full extent up front (io_uring:
fallocate(FALLOC_FL_KEEP_SIZE); IOCP: SetEndOfFile) so the out-of-order pipelined writes land in
contiguous blocks instead of growing the file piecemeal. It is:
  * FRESH-file only (never on a media-reconnect resume — that keeps its verified prefix),
  * size-GATED (only files >= PREALLOCATE_MIN_SIZE = 1 MiB — tiny files don't fragment and preallocating
    them would only add syscalls to the small-file path we are trying to speed up), and
  * BEST-EFFORT (any failure — EOPNOTSUPP on FAT/network FS, ENOSPC on a full dest — is ignored; the copy
    proceeds and the real ENOSPC still surfaces at write time).

Verified from OUTSIDE (no engine code touched):
  * content: `diff -rq --no-dereference` == 0 after the copy (preallocation must not corrupt a byte);
  * that it actually ENGAGED and is correctly gated: the LD_PRELOAD op trace logs the libc `fallocate`
    (io_uring calls it through libc — unlike its ring reads/writes), so we assert a FALLOC op for the
    LARGE dest file and NONE for the small ones (the size gate works), and that it did not error.

io_uring only on Linux (fallocate is libc-visible). Async is untouched (no preallocation). IOCP's
SetEndOfFile preallocation is verified on the Windows laptop (iocp lane), not here.
"""
import sys, pathlib, os
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

MIB = 1024 * 1024


def _build_tree(root):
    """Small files (< 1 MiB, must NOT be preallocated) + one LARGE file (>= 1 MiB, must be).
    Returns (files{relpath:size}, dirs[])."""
    import shutil
    if os.path.exists(root):
        shutil.rmtree(root)
    os.makedirs(os.path.join(root, "sub"), exist_ok=True)
    files = {
        "small_a.txt": 6,
        "small_b.bin": 200 * 1024,     # 200 KiB — below the 1 MiB gate
        "sub/small_c.dat": 900 * 1024, # 900 KiB — still below the gate
        "big.iso": 4 * MIB,            # 4 MiB — ABOVE the gate: must be preallocated
    }
    payload = {
        "small_a.txt": b"hello\n",
        "small_b.bin": bytes(200 * 1024),
        "sub/small_c.dat": bytes(900 * 1024),
        "big.iso": (b"ABCD" * (MIB))[: 4 * MIB],   # deterministic, non-zero content
    }
    for rel, data in payload.items():
        with open(os.path.join(root, rel), "wb") as fh:
            fh.write(data)
    return files, ["sub"]


def _check(backend):
    src = K.fresh_src_root("prealloc_src")
    files, dirs = _build_tree(src)
    dest = K.fresh_dest("prealloc_dest")
    r, t, exp_files, exp_dirs = K.run_traced(
        backend, "cp", src, dest, files, dirs,
        file_collision=H.FileCollision.OVERWRITE, folder_collision=H.FolderCollision.MERGE,
        expect_dir=src)

    ok = True
    if not (r.ok and r.content_ok):
        print(f"      run not ok: completed={r.completed} alive={r.stayed_alive} "
              f"content={r.content_ok} mem_errors={r.mem_errors}\n{r.diff_text}")
        ok = False

    # The op trace's FALLOC events prove preallocation engaged AND is correctly size-gated.
    pre = t.prealloc_paths()          # {dest_path: (len, rc)}
    big = None
    copied = K.copied_root(dest, src)
    big_dest = os.path.normpath(os.path.join(copied, "big.iso"))
    for p, (ln, rc) in pre.items():
        if os.path.normpath(p) == big_dest:
            big = (ln, rc)
    if big is None:
        print(f"      FALLOC missing for the large file {big_dest} — preallocation did not engage "
              f"(traced FALLOC paths: {list(pre)})")
        ok = False
    else:
        ln, rc = big
        if ln != files["big.iso"]:
            print(f"      FALLOC len {ln} != file size {files['big.iso']} (should reserve the full extent)")
            ok = False
        if rc != 0:
            print(f"      NOTE: FALLOC returned rc={rc} on this FS (best-effort skip) — acceptable, "
                  f"but content must still be correct")
    # size gate: NONE of the sub-1-MiB files may be preallocated
    for rel in ("small_a.txt", "small_b.bin", "sub/small_c.dat"):
        dp = os.path.normpath(os.path.join(copied, rel))
        if dp in {os.path.normpath(p) for p in pre}:
            print(f"      small file {rel} was preallocated — size gate ({MIB} B) not honored")
            ok = False
    return ok


def run(backends=None, memcheck=H.NONE) -> bool:
    # Only io_uring exercises the libc-visible fallocate on Linux; async has no preallocation and IOCP
    # runs on the laptop. Restrict to io_uring here.
    bs = backends or [H.IO_URING]
    bs = [b for b in bs if b == H.IO_URING]
    if not bs:
        print("    [preallocate] SKIP (io_uring-only on Linux; IOCP verified on the laptop)")
        return True
    return K.for_backends(bs, _check)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
