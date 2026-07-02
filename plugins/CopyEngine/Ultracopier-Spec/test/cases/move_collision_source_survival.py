#!/usr/bin/env python3
"""MOVE x fileCollision Skip(1) + conditional keep-direction (3,4,5,7,8): the NOT-copied
file's SOURCE must survive -- the move-side mirror of collision_file_modes (which is cp-only).

A move = copy + per-file delete (../CLAUDE.md "MOVE must NEVER blind-remove the source").
When the collision policy resolves a file to KEEP/SKIP, that file is never copied -- so its
source MUST remain intact: a collision "resolved" is still "processed", and if the engine's
per-file source-removal gate ever keys on processed/exists(destination) (the exact class of
the historical move_destination_error bug: a pre-existing dest DOES exist), the skipped
file's ONLY copy is silently destroyed. This is a mainstream workflow: moving a tree onto an
existing backup with "overwrite if newer" -- every file the predicate KEEPS must survive at
the source.

Per index this case moves a 3-file folder onto a pre-seeded destination:
  * OVW  victim -- predicate TRUE  -> dest := source bytes AND the source file is REMOVED
                   (real move semantics for the overwritten file);
  * KEEP victim -- predicate FALSE -> dest keeps its STALE pre-seed bytes AND the source
                   file SURVIVES byte-identical  <- THE data-safety assertion;
  * NEW  file   -- no collision    -> moved normally (dest := source bytes, source gone),
                   proving the job ran and the KEEP outcome is a decision, not a stall.
For idx 1 (Skip) BOTH victims are keep-direction (recipe makes src newer AND bigger, so a
regression that misreads Skip as any predicate would overwrite and fail loudly).
The source FOLDER must still exist afterwards holding exactly the kept files (a folder is
removed only when empty -- bottom-up rmdir invariant).

Runs CROSS-FILESYSTEM (source on /tmp tmpfs, dest on the scratch fs -> isSameDrive()==false,
the copy+delete path where the delete decision lives) for every index, plus a SAME-FS variant
for idx 1 and 4 (the rename()-based move path must consult the same policy).

Mirrors collision_file_modes.py's recipes/content discipline: every role's bytes are distinct
so no assertion can pass for the wrong reason. Uses the DEFAULT inode thread count (the
inode_threads=1 async skip-pump stall is a separate known issue, see inode1_skip_pump).
"""
import sys, pathlib, os, shutil, tempfile
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

T_OLD = 1000000000
T_NEW = 1500000000

SRC_OVW = b"MV-SRC-OVERWRITE-victim-content-must-land-in-destination\n"
SEED_OVW = b"MV-STALE-overwrite-victim-pre-seed-MUST-BE-REPLACED\n"
SRC_KEEP = b"MV-SRC-keep-victim-content-MUST-STAY-AT-SOURCE-intact\n"
SEED_KEEP = b"MV-STALE-keep-victim-pre-seed-MUST-SURVIVE-unchanged\n"
SRC_NEW = b"MV-SRC-new-no-collision-file-moves-normally\n"

OVW = "ovw.dat"
KEEP = "keep.dat"
NEW = "newfile.dat"

# (src_mtime, src_extra_bytes, seed_mtime, seed_extra_bytes) per victim -- same discipline as
# collision_file_modes.RECIPES; sizes equalised via NUL padding so only the intended dimension
# drives the predicate.
RECIPES = {
    # idx 1 Skip: NO predicate -- both victims must be kept. Bait a misread: src newer AND
    # bigger than the seed (any date/size predicate would say overwrite; Skip must not).
    1: {
        OVW:  (T_NEW, 32, T_OLD, 0),
        KEEP: (T_NEW, 32, T_OLD, 0),
    },
    4: {  # OverwriteIfNewer: overwrite IFF src newer
        OVW:  (T_NEW, 0, T_OLD, 0),
        KEEP: (T_OLD, 0, T_NEW, 0),
    },
    5: {  # OverwriteIfOlder: overwrite IFF src older
        OVW:  (T_OLD, 0, T_NEW, 0),
        KEEP: (T_NEW, 0, T_OLD, 0),
    },
    8: {  # OverwriteIfNotSameMdate (date-only)
        OVW:  (T_NEW, 0, T_OLD, 0),
        KEEP: (T_OLD, 0, T_OLD, 0),
    },
    7: {  # OverwriteIfNotSameSize (size-only)
        OVW:  (T_OLD, 32, T_OLD, 0),
        KEEP: (T_OLD, 0, T_OLD, 0),
    },
    3: {  # OverwriteIfNotSameSizeAndDate: keep only when BOTH match
        OVW:  (T_NEW, 0, T_OLD, 0),
        KEEP: (T_OLD, 0, T_OLD, 0),
    },
}
# For idx 1 BOTH victims are keep-direction; for the predicate indices only KEEP is.
KEPT_NAMES = {1: (OVW, KEEP), 3: (KEEP,), 4: (KEEP,), 5: (KEEP,), 7: (KEEP,), 8: (KEEP,)}


def _sized(base: bytes, total_len: int) -> bytes:
    assert len(base) <= total_len, (len(base), total_len)
    return base + b"\0" * (total_len - len(base))


def _build(idx: int, src_root: str, seed_root: str):
    """Lay down the 3-file source (OVW, KEEP, NEW) and the 2-file colliding seed for one index."""
    for r in (src_root, seed_root):
        if os.path.exists(r):
            shutil.rmtree(r)
        os.makedirs(r, exist_ok=True)
    rec = RECIPES[idx]
    for name, src_base, seed_base in ((OVW, SRC_OVW, SEED_OVW), (KEEP, SRC_KEEP, SEED_KEEP)):
        src_mtime, src_extra, seed_mtime, seed_extra = rec[name]
        base_len = max(len(src_base), len(seed_base))
        src_bytes = _sized(src_base, base_len + src_extra)
        seed_bytes = _sized(seed_base, base_len + seed_extra)
        sp = os.path.join(src_root, name)
        with open(sp, "wb") as fh:
            fh.write(src_bytes)
        os.utime(sp, (src_mtime, src_mtime))
        dp = os.path.join(seed_root, name)
        with open(dp, "wb") as fh:
            fh.write(seed_bytes)
        os.utime(dp, (seed_mtime, seed_mtime))
    K.write_file(os.path.join(src_root, NEW), SRC_NEW)


def _one_index(backend, idx: int, src_root: str, seed_root: str, memcheck, tag: str) -> bool:
    """Move src_root onto a dest pre-seeded from seed_root; assert both dest outcomes AND
    source survival/removal per role. src_root must already exist (fs variant chosen by caller)."""
    base = os.path.basename(src_root)
    dest = K.fresh_dest(f"mcs_{idx}_{tag}_dest")
    for name in (OVW, KEEP):
        sp = os.path.join(seed_root, name)
        dp = os.path.join(dest, base, name)
        K.write_file(dp, b"")
        shutil.copyfile(sp, dp)
        st = os.stat(sp)
        os.utime(dp, (st.st_mtime, st.st_mtime))

    # Keep pristine copies of the source bytes for post-run comparison (the source tree is
    # about to be mutated by the move itself).
    src_bytes = {n: K.read_file(os.path.join(src_root, n)) for n in (OVW, KEEP, NEW)}
    seed_bytes = {n: K.read_file(os.path.join(seed_root, n)) for n in (OVW, KEEP)}

    r = H.run(backend, "mv", [src_root], dest,
              file_collision=idx,
              folder_collision=H.FolderCollision.MERGE,
              file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
              expect_dir=None, memcheck=memcheck)

    kept = KEPT_NAMES[idx]
    moved = tuple(n for n in (OVW, KEEP, NEW) if n not in kept)  # NEW always; OVW when predicate
    copied = os.path.join(dest, base)
    problems = []

    if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
        problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                        f"oom={r.oom_killed} mem_err={r.mem_errors}")

    # Destination outcomes.
    for n in moved:
        p = os.path.join(copied, n)
        got = K.read_file(p) if os.path.exists(p) else b""
        if got != src_bytes[n]:
            problems.append(f"dest {n}: expected SOURCE bytes, got "
                            f"{'<missing>' if not got else 'wrong content'}")
    for n in kept:
        p = os.path.join(copied, n)
        got = K.read_file(p) if os.path.exists(p) else b""
        if got != seed_bytes[n]:
            problems.append(f"dest {n}: expected STALE seed bytes to survive, got "
                            f"{'<missing>' if not got else 'overwritten!'}")

    # SOURCE survival -- the data-safety core of this case.
    for n in kept:
        p = os.path.join(src_root, n)
        if not os.path.exists(p):
            problems.append(f"source {n}: DELETED despite keep/skip -- DATA LOSS")
        elif K.read_file(p) != src_bytes[n]:
            problems.append(f"source {n}: content corrupted")
    for n in moved:
        if os.path.exists(os.path.join(src_root, n)):
            problems.append(f"source {n}: still present -- move did not remove the moved file")
    # The source folder itself must survive (it still holds the kept files -> not empty).
    if not os.path.isdir(src_root):
        problems.append("source FOLDER removed while still holding kept files")

    if problems:
        print(f"      [idx {idx} {tag}] FAIL: " + "; ".join(problems))
    return not problems


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        ok = True
        # Cross-filesystem (the dangerous copy+delete path) -- every index.
        for idx in sorted(RECIPES):
            tmp_parent = tempfile.mkdtemp(prefix=f"uc-mcs-{idx}-", dir="/tmp")
            try:
                src_root = os.path.join(tmp_parent, f"mcs_{idx}_src")
                seed_root = K.fresh_src_root(f"mcs_{idx}_seed")
                _build(idx, src_root, seed_root)
                # Guard the premise: /tmp really is a different filesystem than the dest scratch.
                if os.stat(tmp_parent).st_dev == os.stat(K._scratch_root()).st_dev:
                    print("      SKIP-premise: /tmp not a distinct filesystem -- cross-fs "
                          "variant impossible on this box")
                    return False
                ok = _one_index(backend, idx, src_root, seed_root, memcheck, "xfs") and ok
            finally:
                shutil.rmtree(tmp_parent, ignore_errors=True)
        # Same-filesystem (rename()-path move must consult the same policy) -- idx 1 and 4.
        for idx in (1, 4):
            src_root = K.fresh_src_root(f"mcs_{idx}_samefs_src")
            seed_root = K.fresh_src_root(f"mcs_{idx}_samefs_seed")
            _build(idx, src_root, seed_root)
            ok = _one_index(backend, idx, src_root, seed_root, memcheck, "samefs") and ok
        return ok

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
