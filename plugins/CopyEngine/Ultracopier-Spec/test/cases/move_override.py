#!/usr/bin/env python3
"""Same as copy_override but with mode 'mv'."""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from cases import copy_override


def run(backends=None, memcheck=H.NONE) -> bool:
    return copy_override.run(backends=backends, memcheck=memcheck, _mode="mv")


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
