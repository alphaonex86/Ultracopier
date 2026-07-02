#!/usr/bin/env python3
"""Dir/file TYPE-mismatch collision at the destination: source has a DIRECTORY `x/` where the
dest already has a FILE `x`, and a FILE `y` where the dest has a DIRECTORY `y/`.

Every existing collision case seeds same-type collisions (file-vs-file, folder-vs-folder);
the type-mismatch cells have zero coverage, yet they are exactly where blind predicates go
wrong: destinationExists() compares size/mtime assuming a FILE dest; mkdir over an existing
file fails EEXIST-but-not-a-dir; open-for-write over a directory fails EISDIR. A regression
here means a hang (unhandled error looping), a crash, or destruction of the pre-existing
dest object.

Policy: folderCollision=MERGE, fileCollision=OVERWRITE, fileError=SKIP, folderError=SKIP --
the most automatic settings, so any dialog fallback (a hang headless) or crash is loudly
visible. ASSERTS:
  * the job completes and stays alive (no hang on the mismatch, no crash), memory clean;
  * the NON-conflicting sibling files land byte-perfect (the mismatch must not abort the job);
  * the SOURCE is untouched (it's a copy);
  * evidence-only (printed, not asserted): what the engine did with each mismatched dest
    object -- overwritten/replaced/kept -- so a behaviour CHANGE is visible in the log.
"""
import sys, pathlib, os, hashlib, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

SIB1 = b"sibling-1 must land regardless of the type-mismatch neighbours\n"
SIB2 = b"sibling-2 nested payload\n"
XFILE = b"source file inside dir x -- lands only if x/ wins over the dest FILE x\n"
YFILE = b"source FILE y -- lands only if y wins over the dest DIRECTORY y/\n"
SEED_XFILE = b"pre-existing dest FILE named x (collides with source DIR x/)\n"
SEED_YINNER = b"pre-existing file inside dest DIRECTORY y/ (collides with source FILE y)\n"


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        # Per-backend + wipe: fresh_src_root does NOT wipe (unlike fresh_dest), so a second
        # backend would otherwise inherit the first run's tree.
        src = K.fresh_src_root(f"tmc_{backend}_src")
        shutil.rmtree(src, ignore_errors=True)
        K.write_file(os.path.join(src, "sib1.txt"), SIB1)
        K.write_file(os.path.join(src, "x", "inner.txt"), XFILE)      # source DIR x/
        K.write_file(os.path.join(src, "y"), YFILE)                    # source FILE y
        K.write_file(os.path.join(src, "sub", "sib2.txt"), SIB2)
        base = os.path.basename(src)
        src_hashes = {}
        for dp, _dn, fn in os.walk(src):
            for f in fn:
                p = os.path.join(dp, f)
                src_hashes[os.path.relpath(p, src)] = hashlib.sha256(K.read_file(p)).hexdigest()

        dest = K.fresh_dest(f"tmc_{backend}_dest")
        K.write_file(os.path.join(dest, base, "x"), SEED_XFILE)               # dest FILE x
        K.write_file(os.path.join(dest, base, "y", "inner.txt"), SEED_YINNER)  # dest DIR y/

        r = H.run(backend, "cp", [src], dest,
                  file_collision=H.FileCollision.OVERWRITE,
                  folder_collision=H.FolderCollision.MERGE,
                  file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                  expect_dir=None, memcheck=memcheck)

        copied = os.path.join(dest, base)
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}  <-- hang/crash on mismatch")

        # Non-conflicting siblings MUST have landed byte-perfect.
        for rel, data in (("sib1.txt", SIB1), (os.path.join("sub", "sib2.txt"), SIB2)):
            p = os.path.join(copied, rel)
            if not os.path.exists(p) or K.read_file(p) != data:
                problems.append(f"sibling {rel} missing/corrupt -- the mismatch aborted the job")

        # Source untouched (copy semantics).
        for rel, want in src_hashes.items():
            p = os.path.join(src, rel)
            if not os.path.exists(p) or hashlib.sha256(K.read_file(p)).hexdigest() != want:
                problems.append(f"SOURCE {rel} mutated by a copy")

        # Evidence (not asserted): the engine's resolution of each mismatch.
        x_p, y_p = os.path.join(copied, "x"), os.path.join(copied, "y")
        x_state = ("dir" if os.path.isdir(x_p) else "file" if os.path.exists(x_p) else "gone")
        y_state = ("dir" if os.path.isdir(y_p) else "file" if os.path.exists(y_p) else "gone")
        print(f"      [evidence] dest x (was FILE, src DIR) -> {x_state}; "
              f"dest y (was DIR, src FILE) -> {y_state}")

        if problems:
            print(f"      FAIL: " + "; ".join(problems))
        return not problems

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
