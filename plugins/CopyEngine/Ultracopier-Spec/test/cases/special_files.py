#!/usr/bin/env python3
"""FIFO / UNIX-socket special files in the source tree -- the open-hang class.

open(O_RDONLY) on a FIFO with no writer blocks FOREVER; reading a socket file is EOPNOTSUPP
at best. A backup tool pointed at a real $HOME will meet both (mkfifo leftovers, .X11-unix,
app sockets). If the scan enqueues them as regular files, the transfer thread blocks in
open()/read() -- the whole job hangs on an object that carries no data at all. (No device
nodes here: mknod needs root; FIFO+socket need no privileges and exercise the same
classification decision.)

Source: a normal tree with a FIFO and a bound AF_UNIX socket placed BETWEEN regular files in
name order (so a positional stall would strand provably-later files), fileError=SKIP.
ASSERTS:
  * the job COMPLETES (no hang -- the no-progress watchdog turns an open()-block into a
    loud fail) and the process stays alive, memory clean;
  * every REGULAR file is copied byte-perfect -- files sorting AFTER the specials included;
  * evidence-only (printed): whether the specials were skipped or materialised at the dest
    (either is stability-acceptable; blocking on them is not).
"""
import sys, pathlib, os, socket, hashlib, shutil
_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K

REGULARS = {
    "a_before.txt": b"regular file sorting BEFORE the specials\n" * 8,
    "m_fifo": None,     # placeholder -- FIFO created below (name between a_ and z_)
    "n_sock": None,     # placeholder -- socket bound below
    "p_after.dat": bytes(range(256)) * 2048,   # 512 KiB AFTER the specials in sort order
    "z_last.txt": b"the very last regular file -- must still arrive\n",
    os.path.join("sub", "nested.bin"): b"nested payload\n" * 32,
}


def run(backends=None, memcheck=H.NONE) -> bool:

    def one(backend):
        src = K.fresh_src_root(f"specials_{backend}_src")
        shutil.rmtree(src, ignore_errors=True)  # fresh_src_root does not wipe
        hashes = {}
        for rel, data in REGULARS.items():
            if data is not None:
                K.write_file(os.path.join(src, rel), data)
                hashes[rel] = hashlib.sha256(data).hexdigest()
        os.mkfifo(os.path.join(src, "m_fifo"))
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.bind(os.path.join(src, "n_sock"))   # socket file exists on disk; keep it bound

        dest = K.fresh_dest(f"specials_{backend}_dest")
        try:
            r = H.run(backend, "cp", [src], dest,
                      file_collision=H.FileCollision.OVERWRITE,
                      folder_collision=H.FolderCollision.MERGE,
                      file_error=H.FileError.SKIP, folder_error=H.FolderError.SKIP,
                      expect_dir=None, memcheck=memcheck)
        finally:
            s.close()

        copied = os.path.join(dest, os.path.basename(src))
        problems = []
        if not (r.completed and r.stayed_alive and not r.oom_killed and r.mem_errors == 0):
            problems.append(f"run: completed={r.completed} alive={r.stayed_alive} "
                            f"oom={r.oom_killed} mem_err={r.mem_errors}"
                            + ("  <-- OPEN-HANG on a special file" if not r.completed else ""))
        for rel, want in hashes.items():
            p = os.path.join(copied, rel)
            if not os.path.exists(p):
                problems.append(f"regular {rel} missing (stranded behind a special?)")
            elif hashlib.sha256(K.read_file(p)).hexdigest() != want:
                problems.append(f"regular {rel} corrupt")

        fifo_d = os.path.join(copied, "m_fifo")
        sock_d = os.path.join(copied, "n_sock")
        def _state(p):
            if not os.path.lexists(p):
                return "skipped"
            import stat as st
            m = os.lstat(p).st_mode
            return ("fifo" if st.S_ISFIFO(m) else "sock" if st.S_ISSOCK(m)
                    else "file" if st.S_ISREG(m) else "other")
        print(f"      [evidence] dest fifo -> {_state(fifo_d)}; dest socket -> {_state(sock_d)}")

        if problems:
            print(f"      FAIL: " + "; ".join(problems))
        return not problems

    return K.for_backends(backends, one)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
