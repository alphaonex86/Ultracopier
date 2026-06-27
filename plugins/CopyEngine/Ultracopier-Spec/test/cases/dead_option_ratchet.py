#!/usr/bin/env python3
"""Dead-option ratchet: every option registered in CopyEngineFactory's KeysList must be
CONSUMED by the engine (read via getOptionValue / written via setOptionValue somewhere in
the plugin source), otherwise it is a silently-dead option -- registered, shown/persisted,
but ignored. This is the meta-test behind "test ALL the copy engine options": it fails the
moment a newly-registered option has no consumer, or a known-inert one quietly gains/loses
one (so the allowlist below stays honest).

Static analysis only (no binary, backend-agnostic) -- runs once regardless of backends.

KNOWN_INERT is the ratchet baseline: options registered but intentionally not consumed by
THIS engine in the current build. Tightening (an inert option gaining a consumer) is GOOD
and the test tells you to remove it from the list; loosening (a consumed option going dead,
or a brand-new dead option) is a REGRESSION and fails. Never add to KNOWN_INERT just to dodge
a failure -- only when the option is genuinely, deliberately unconsumed.
"""
import sys, pathlib, re
# Strip the cases/ dir from sys.path so `import copy` (stdlib, used by dataclasses) is not
# shadowed by cases/copy.py when this file is run directly. See windows_ads.py.
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H

SPEC = pathlib.Path(__file__).resolve().parents[2]   # .../Ultracopier-Spec
FACTORY = SPEC / "CopyEngineFactory.cpp"

# Registered options that are genuinely not consumed in the current build, with the reason.
# This is the one-way ratchet baseline (see module docstring).
KNOWN_INERT = {
    # FINDING: registered with default ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE but never read --
    # the engine uses the COMPILE-TIME block size, so a user-set blockSize has no effect.
    # Wiring it (getInstance -> setBlockSize) is a prod behaviour change; left inert until approved.
    "blockSize": "registered but ignored; engine uses compile-time ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE",
    "transferAlgorithm": "legacy speed-calc selector; not read by Ultracopier-Spec engine",
    "checksumIgnoreIfImpossible": "checksum sub-option; only meaningful once checksum verify path consumes it",
    "checksumOnlyOnError": "checksum sub-option; only meaningful once checksum verify path consumes it",
    "osBufferLimited": "OS write-buffer tuning; not wired into the current transfer backends",
    "osBufferLimit": "OS write-buffer tuning; not wired into the current transfer backends",
    "osBuffer": "OS write-buffer tuning; not wired into the current transfer backends",
}


def _registered_keys(text: str):
    # KeysList.push_back(std::pair<std::string, std::string>("KEY", ...));  -- de-duplicated,
    # preserving first-seen order (osBuffer/buffer/keepDate appear in BOTH #ifdef branches).
    seen, out = set(), []
    for m in re.finditer(
            r'KeysList\.push_back\(std::pair<std::string,\s*std::string>\("([^"]+)"', text):
        k = m.group(1)
        if k not in seen:
            seen.add(k)
            out.append(k)
    return out


def _engine_blob() -> str:
    parts = []
    for p in SPEC.rglob("*"):
        if p.suffix in (".cpp", ".h") and p.is_file():
            s = str(p)
            if "/build/" in s or "/moc_" in s or p.name.startswith("moc_") \
                    or s.endswith(".backup") or "/test/" in s:
                continue
            try:
                parts.append(p.read_text(errors="ignore"))
            except OSError:
                pass
    return "\n".join(parts)


def run(backends=None, memcheck=H.NONE) -> bool:
    ftext = FACTORY.read_text()
    keys = _registered_keys(ftext)
    assert keys, "could not parse any registered option keys from CopyEngineFactory.cpp"
    blob = _engine_blob()

    consumed, suspect = [], []
    for k in keys:
        used = (f'getOptionValue("{k}")' in blob) or (f'setOptionValue("{k}")' in blob)
        (consumed if used else suspect).append(k)

    print(f"    registered={len(keys)} consumed={len(consumed)} suspect={len(suspect)}")

    ok = True
    # (1) any SUSPECT not in the allowlist is a NEW dead option -> regression.
    new_dead = [k for k in suspect if k not in KNOWN_INERT]
    if new_dead:
        ok = False
        print("    FAIL: newly-dead registered options (registered but never get/set):")
        for k in new_dead:
            print(f"      - {k}")
        print("    -> wire it into getInstance()/resetOptions() (read via getOptionValue),"
              " or, only if truly inert, add it to KNOWN_INERT with a reason.")
    # (2) any allowlisted option that is now CONSUMED -> ratchet tightened, update the list.
    revived = [k for k in KNOWN_INERT if k in consumed]
    if revived:
        ok = False
        print("    FAIL: options in KNOWN_INERT are now consumed -- remove them from the list:")
        for k in revived:
            print(f"      - {k}")
    # (3) allowlist entries that are no longer even registered -> stale, clean them.
    stale = [k for k in KNOWN_INERT if k not in keys]
    if stale:
        ok = False
        print("    FAIL: KNOWN_INERT lists options that are no longer registered (stale):")
        for k in stale:
            print(f"      - {k}")

    if ok:
        print(f"    OK: all {len(keys)} registered options are consumed or known-inert "
              f"({len(KNOWN_INERT)} inert).")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
