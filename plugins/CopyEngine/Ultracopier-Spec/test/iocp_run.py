#!/usr/bin/env python3
"""Ad-hoc IOCP (Windows) runtime verification of the current build, driven over SSH via winlane.
Builds a synthetic tree locally, pushes it to the box, runs a real ultracopier.exe IOCP copy,
and diffs the result. Usage: python3 iocp_run.py <profile> [profile2 ...]   (default: comprehensive)
Each profile is one synthtree profile name (default/comprehensive/weird_names/long_paths/symlinks)."""
import sys, pathlib
_HERE = pathlib.Path(__file__).resolve().parent
sys.path[:] = [p for p in sys.path if p not in ("", str(_HERE))]
sys.path.insert(0, str(_HERE))
from lib import harness as H, synthtree, winlane

def run(profiles):
    cfg = H.load_config()
    cfg.set("paths", "SOURCEWINDOWS", "")   # force staging of our synthetic tree (don't copy C:\source)
    ok_all = True
    for prof in profiles:
        src = synthtree.make_tree(f"/tmp/uc-iocp-{prof}-src", prof)
        r = winlane.run_windows("cp", [src], cfg=cfg,
                                file_collision=H.FileCollision.OVERWRITE,
                                folder_collision=H.FolderCollision.MERGE,
                                expect=src)
        ok = r.ok
        ok_all = ok_all and ok
        print(f"[IOCP] {prof:14s} {'PASS' if ok else 'FAIL'}  "
              f"completed={r.completed} alive={r.stayed_alive} content={r.content_ok} "
              f"peak_rss={r.peak_rss_mb}MB mem_errors={r.mem_errors} skipped={r.skipped}")
        if not ok:
            print(f"    notes: {r.notes}\n    diff:\n{r.diff_text[:2000]}")
    return ok_all

if __name__ == "__main__":
    profs = sys.argv[1:] or ["comprehensive"]
    sys.exit(0 if run(profs) else 1)
