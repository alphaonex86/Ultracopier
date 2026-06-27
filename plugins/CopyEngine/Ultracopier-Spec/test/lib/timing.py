#!/usr/bin/env python3
"""Per-case / per-backend timing recorder for the Ultracopier-Spec test suite.

Writes one tab-separated line per timed unit to a persistent log so slow cases can be
pruned or optimized later, and echoes the START datetime + duration to the console.

Log line format (TSV):
    <start ISO8601>\t<duration s>\t<label>\t<result>\t<run_id>

`label` is "<case>" for a whole-case total, or "<case>:<backend>" for one backend run.
The whole thing is best-effort: a failure to write the log NEVER breaks a test.
"""
from __future__ import annotations
import os, time, datetime, pathlib, threading

# Persistent timing log (tmpfs, per user request). Override with UC_TIMING_LOG.
TIMING_LOG = os.environ.get("UC_TIMING_LOG",
                            "/mnt/data/perso/tmpfs/ultracopier-temp/ultracoper-test-time.log")

_lock = threading.Lock()
_current_case = "?"
# A short id for this suite invocation so rows from one run group together.
_run_id = datetime.datetime.now().strftime("%m%d-%H%M%S")


def _ensure_dir() -> None:
    try:
        pathlib.Path(TIMING_LOG).parent.mkdir(parents=True, exist_ok=True)
    except OSError:
        pass


def set_case(name: str) -> None:
    """Set the case name used to label subsequent per-backend records."""
    global _current_case
    _current_case = name


def current_case() -> str:
    return _current_case


def _iso(ts: float) -> str:
    return datetime.datetime.fromtimestamp(ts).isoformat(timespec="seconds")


def record(label: str, start_ts: float, duration: float, result: str = "") -> None:
    """Append one timing row and echo it to the console."""
    _ensure_dir()
    line = f"{_iso(start_ts)}\t{duration:8.1f}\t{label}\t{result}\t{_run_id}"
    with _lock:
        try:
            with open(TIMING_LOG, "a") as f:
                f.write(line + "\n")
        except OSError:
            pass
    print(f"    [time] start={_iso(start_ts)} dur={duration:6.1f}s  {label}"
          + (f"  {result}" if result else ""), flush=True)


def suite_header(backends, cases) -> None:
    """Print + log a header line at the start of a suite invocation."""
    _ensure_dir()
    now = time.time()
    hdr = (f"# ==== suite start {_iso(now)} run={_run_id} "
           f"backends={','.join(backends)} cases={len(cases)} ====")
    with _lock:
        try:
            with open(TIMING_LOG, "a") as f:
                f.write(hdr + "\n")
        except OSError:
            pass
    print(f"[time] suite START {_iso(now)}  run={_run_id}  "
          f"backends={','.join(backends)}  cases={len(cases)}  -> {TIMING_LOG}", flush=True)


class backend_timer:
    """Context manager timing one backend run; records '<case>:<backend>' on exit.

    Usage:
        with timing.backend_timer(backend):
            r = _run_inner(backend, ...)
    The result string is taken from .result if the caller sets it.
    """
    def __init__(self, backend: str, label: str | None = None):
        self.label = label or f"{_current_case}:{backend}"
        self.result = ""

    def __enter__(self):
        self.t0 = time.time()
        return self

    def __exit__(self, exc_type, exc, tb):
        res = self.result or ("EXC" if exc_type else "")
        record(self.label, self.t0, time.time() - self.t0, res)
        return False
