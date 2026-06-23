#!/usr/bin/env python3
"""The Windows IOCP lane for the Ultracopier-Spec CopyEngine tests.

Same DESIGN PRINCIPLE as harness.py: we NEVER patch ultracopier's C++ to make it
testable.  We drive the REAL shipping ultracopier.exe on a remote Windows box
entirely from the OUTSIDE, over SSH:

  * options & collision/user-actions  -> written into an ISOLATED ultracopier.conf
                                         (a throwaway %USERPROFILE%\\.config on the box)
  * the resident process               -> always killed EXTERNALLY (Stop-Process)
  * the 1 GB memory cap                -> a Win32 Job Object (JobMemoryLimit)
  * "copy finished"                    -> CPU goes idle (dest files are PRE-SIZED on
                                         IOCP, so byte-count of the dest is meaningless)
  * correctness                        -> robocopy /L (or hash) diff of the two trees
  * crashes                            -> Application event-log scan for Event 1000
                                         (Windows Error Reporting) against ultracopier.exe

It mirrors harness.run() but for the IOCP backend:

    run_windows('cp', [src...], file_collision=..., expect=..., ...) -> WindowsResult

Everything (host, exe, staging paths) is read ONLY from test/config.ini -> no secrets
in the source tree.  If [windows] host is EMPTY, every IOCP case is SKIPPED (returns a
WindowsResult with .skipped == True and .ok == True so the Linux lanes still pass), and
no SSH is attempted at all.
"""
from __future__ import annotations
import os, posixpath, shlex, subprocess, tempfile, time, uuid, pathlib, dataclasses

# Reuse the harness constants / config loader / synthetic-tree expectations so the two
# lanes stay byte-for-byte in agreement about combo indexes, limits and the Result shape.
from lib import harness as H

IOCP = H.IOCP  # "iocp"


# ---------------------------------------------------------------------------
# Result -- shaped like harness.Result, with an extra .skipped flag.
# ---------------------------------------------------------------------------
@dataclasses.dataclass
class WindowsResult:
    backend: str = IOCP
    completed: bool = False        # copy reached CPU-idle within the timeout
    stayed_alive: bool = False     # still running after stay_alive_seconds (no teardown crash)
    content_ok: bool = False       # robocopy/hash diff of the trees == clean
    peak_rss_mb: int = 0           # peak working set of ultracopier.exe (MB)
    oom_killed: bool = False       # the Job Object killed it for exceeding mem_limit_mb
    mem_errors: int = 0            # WER Event 1000 crashes attributed to ultracopier.exe
    exit_code: int | None = None   # exit code if it died on its own (None if we killed it)
    diff_text: str = ""            # robocopy/hash diff output (empty == identical)
    notes: str = ""                # human notes: crash faulting-module/offset, skip reason...
    skipped: bool = False          # the lane had no [windows] host configured

    @property
    def ok(self) -> bool:
        if self.skipped:
            return True            # a disabled lane is not a failure
        return (self.completed and self.stayed_alive and self.content_ok
                and not self.oom_killed and self.mem_errors == 0)


def _skipped(reason: str) -> WindowsResult:
    return WindowsResult(skipped=True, completed=True, stayed_alive=True,
                         content_ok=True, notes=reason)


# ---------------------------------------------------------------------------
# SSH / SCP plumbing -- batch mode, no interactive prompts, fail fast.
# ---------------------------------------------------------------------------
_SSH_OPTS = ["-o", "BatchMode=yes", "-o", "StrictHostKeyChecking=accept-new",
             "-o", "ConnectTimeout=15"]


class _Box:
    """A thin wrapper around `ssh host ...` / `scp ... host:...` for one Windows box."""

    def __init__(self, host: str):
        self.host = host

    # -- raw remote shell (the box's default shell; we always invoke powershell.exe --
    #    explicitly so behaviour is identical whether the SSH default is cmd or pwsh) -----
    def ssh_raw(self, remote_argv_str: str, timeout=120) -> subprocess.CompletedProcess:
        # errors="replace": a non-English Windows box (e.g. Spanish, cp850/cp1252) emits bytes that
        # are not valid UTF-8; decode tolerantly so the ASCII we actually parse (numbers/keywords)
        # survives instead of raising UnicodeDecodeError.
        return subprocess.run(["ssh", *_SSH_OPTS, self.host, remote_argv_str],
                              capture_output=True, text=True, errors="replace", timeout=timeout)

    def ps(self, script: str, timeout=120) -> subprocess.CompletedProcess:
        """Run a PowerShell script block on the box.

        The script is passed as a single base64-EncodedCommand argument so quoting,
        newlines and our INI text survive the SSH + cmd + powershell layers intact.
        """
        import base64
        enc = base64.b64encode(script.encode("utf-16-le")).decode("ascii")
        return self.ssh_raw(
            f"powershell.exe -NoProfile -NonInteractive -EncodedCommand {enc}",
            timeout=timeout)

    def push(self, local: str, remote: str, timeout=600) -> None:
        # Stage a whole source TREE to the box. scp -r is NOT used: it mangles Windows
        # paths, refuses hostile filenames and silently drops symlinks -- all of which the
        # parity test exercises. Instead tar the tree LOCALLY (preserves every byte of every
        # name + symlinks), scp the single archive to a HOME-RELATIVE temp (the only form
        # OpenSSH-on-Windows scp accepts reliably -- absolute Windows paths get mangled), then
        # Move it next to the target and extract with bsdtar (built into Windows 10).
        #
        # NOTE: bsdtar on Windows CANNOT extract names containing chars Windows forbids in
        # filenames (tab, newline, < > : " | ? *, and the 0x01-0x1f control range). Those are
        # POSIX-only hostile names; the IOCP parity case must use a Windows-compatible subset.
        # See test/lib/synthtree.py _win_safe_weird_name_bytes().
        import tempfile
        local = os.path.normpath(local)
        parent, base = os.path.split(local)
        if not base:
            raise RuntimeError(f"push: cannot push root: {local!r}")
        with tempfile.NamedTemporaryFile(prefix="uc-stage-", suffix=".tar", delete=False) as tf:
            tar_local = tf.name
        try:
            r = subprocess.run(["tar", "-cf", tar_local, "-C", parent, base],
                               capture_output=True, text=True, timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"local tar of {local} failed: {r.stderr.strip()}")
            # scp the single archive to a HOME-RELATIVE path (the only reliable form on
            # Windows OpenSSH). A unique name so concurrent runs don't collide.
            tar_name = f"uc-stage-{uuid.uuid4().hex[:8]}.tar"
            r = subprocess.run(["scp", *_SSH_OPTS, tar_local, f"{self.host}:{tar_name}"],
                               capture_output=True, text=True, errors="replace", timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"scp archive {local} -> ~/{tar_name} failed: {r.stderr.strip()}")
            # Move the archive from HOME to next to the target, extract, clean up. The parent
            # of `remote` must exist (run_windows creates `stage`'s parent `base` before push,
            # and `remote` = stage itself which we create here). Build the HOME tar path with
            # Join-Path so $env:USERPROFILE is EXPANDED (single-quoted '$env:...' is a literal).
            tar_on_box = remote.rstrip("\\") + "\\stage.tar"
            ps = (f"if (-not (Test-Path '{remote}')) {{ New-Item -ItemType Directory -Force -Path '{remote}' | Out-Null }}; "
                  f"$homeTar = Join-Path $env:USERPROFILE '{tar_name}'; "
                  f"Move-Item -LiteralPath $homeTar -Destination '{tar_on_box}' -Force -ErrorAction SilentlyContinue; "
                  f"if (-not (Test-Path '{tar_on_box}')) {{ Write-Output ('MOVE_FAILED src=' + $homeTar); exit 1 }}; "
                  f"$e = & cmd.exe /c 'tar -xf \"{tar_on_box}\" -C \"{remote}\" 2>&1'; "
                  f"Remove-Item -LiteralPath '{tar_on_box}' -Force -ErrorAction SilentlyContinue; "
                  f"if ($LASTEXITCODE -ne 0) {{ Write-Output ('TARERR rc=' + $LASTEXITCODE + ' ' + ($e -join \" \")); exit 1 }}")
            r = self.ps(ps, timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"remote tar extract into {remote} failed: "
                                   f"{(r.stdout+r.stderr).strip()[:600]}")
        finally:
            try: os.unlink(tar_local)
            except OSError: pass

    def push_contents(self, local: str, remote: str, timeout=600) -> None:
        """Like push() but stages the CONTENTS of `local` directly into `remote` (no extra
        <basename(local)>/ level). Used by dest_pre_seed: a pre-seed tree must land at
        remote/<file>..., not remote/<tmpdir>/<file>... Tar with '-C local .' so the archive
        has no top-level dir entry, then extract into remote (created if needed)."""
        import tempfile
        local = os.path.normpath(local)
        with tempfile.NamedTemporaryFile(prefix="uc-seed-", suffix=".tar", delete=False) as tf:
            tar_local = tf.name
        try:
            # tar the CONTENTS (-C local .) -> archive entries are ./<file>, ./<dir>/<file>...
            r = subprocess.run(["tar", "-cf", tar_local, "-C", local, "."],
                               capture_output=True, text=True, timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"local tar (contents) of {local} failed: {r.stderr.strip()}")
            tar_name = f"uc-seed-{uuid.uuid4().hex[:8]}.tar"
            r = subprocess.run(["scp", *_SSH_OPTS, tar_local, f"{self.host}:{tar_name}"],
                               capture_output=True, text=True, errors="replace", timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"scp seed archive -> ~/{tar_name} failed: {r.stderr.strip()}")
            tar_on_box = remote.rstrip("\\") + "\\seed.tar"
            ps = (f"if (-not (Test-Path '{remote}')) {{ New-Item -ItemType Directory -Force -Path '{remote}' | Out-Null }}; "
                  f"$homeTar = Join-Path $env:USERPROFILE '{tar_name}'; "
                  f"Move-Item -LiteralPath $homeTar -Destination '{tar_on_box}' -Force -ErrorAction SilentlyContinue; "
                  f"if (-not (Test-Path '{tar_on_box}')) {{ Write-Output 'MOVE_FAILED'; exit 1 }}; "
                  f"$e = & cmd.exe /c 'tar -xf \"{tar_on_box}\" -C \"{remote}\" 2>&1'; "
                  f"Remove-Item -LiteralPath '{tar_on_box}' -Force -ErrorAction SilentlyContinue; "
                  f"if ($LASTEXITCODE -ne 0) {{ Write-Output ('TARERR rc=' + $LASTEXITCODE + ' ' + ($e -join \" \")); exit 1 }}")
            r = self.ps(ps, timeout=timeout)
            if r.returncode != 0:
                raise RuntimeError(f"remote seed extract into {remote} failed: "
                                   f"{(r.stdout+r.stderr).strip()[:600]}")
        finally:
            try: os.unlink(tar_local)
            except OSError: pass


# ---------------------------------------------------------------------------
# Config helpers
# ---------------------------------------------------------------------------
def _win_join(*parts: str) -> str:
    # Join Windows path fragments without letting posixpath rewrite the drive backslashes.
    out = parts[0].rstrip("\\")
    for p in parts[1:]:
        out = out.rstrip("\\") + "\\" + p.strip("\\")
    return out


def _conf_text(*, file_collision, folder_collision, file_error, folder_error,
               keep_date, do_right) -> str:
    """The SAME isolated QSettings INI harness.write_config() produces on Linux.

    The engine reads the QComboBox INDEX (see ../CLAUDE.md), so the combo-index values
    are identical across platforms; only the file location differs.
    """
    return (
        "[CopyEngine-Ultracopier Spec]\r\n"
        f"fileCollision={file_collision}\r\n"
        f"folderCollision={folder_collision}\r\n"
        f"fileError={file_error}\r\n"
        f"folderError={folder_error}\r\n"
        f"keepDate={'true' if keep_date else 'false'}\r\n"
        f"doRightTransfer={'true' if do_right else 'false'}\r\n"
        "autoStart=true\r\n"
        "checkDiskSpace=false\r\n"
        "[CopyEngine]\r\n"
        "List=Ultracopier-Spec\r\n"
        "[Ultracopier]\r\n"
        "GroupWindowWhen=0\r\n"
        "confirmToGroupWindows=false\r\n")


# ---------------------------------------------------------------------------
# The one entry point every IOCP test case uses -- mirror of harness.run().
# ---------------------------------------------------------------------------
def run_windows(mode, sources_local, *, cfg=None,
                file_collision=H.FileCollision.ASK,
                folder_collision=H.FolderCollision.MERGE,
                file_error=H.FileError.SKIP,
                folder_error=H.FolderError.SKIP,
                keep_date=True, do_right=True,
                expect=None, mem_limit_mb=1024, stay_alive_seconds=10,
                dest_pre_seed=None) -> WindowsResult:
    """Run one real cp/mv through the IOCP backend on the remote Windows box.

    mode           : 'cp' or 'mv'.
    sources_local  : list[str] of LOCAL (Linux-side) absolute paths to push to the box,
                     UNLESS [paths] SOURCEWINDOWS is set (then we copy from that on-box
                     path and `sources_local` is ignored for staging).
    expect         : optional LOCAL directory; we diff it against the copied tree on the
                     box (pulled back / hashed).  None -> skip the content check.
    mem_limit_mb   : Job Object memory ceiling (default 1 GB, matching the Linux lane).
    stay_alive_seconds : how long it must stay resident after going idle before we kill it.
    dest_pre_seed  : optional LOCAL directory tree; pushed to the box and extracted INTO the
                     destination BEFORE ultracopier runs (the dest dir must already exist;
                     run_windows creates it). Used by the folder_merge / copy_override cases
                     to pre-create files the source also touches, so MERGE/OVERWRITE/SKIP
                     collide against real pre-existing dest content. The pre-seed tree is laid
                     down as <dest>\<basename(sources_local[0])>\<...> (matching how ultracopier
                     copies a source FOLDER into dest). None -> dest starts empty.
    """
    assert mode in ("cp", "mv"), "mode must be 'cp' or 'mv'"
    cfg = cfg or H.load_config()
    host = cfg.get("windows", "host", fallback="").strip()
    if not host:
        return _skipped("[windows] host empty in config.ini -> IOCP lane disabled")

    exe = cfg.get("windows", "exe", fallback="").strip()
    if not exe:
        return _skipped("[windows] exe empty in config.ini -> cannot launch IOCP")
    dest_root = cfg.get("paths", "DESTINATIONWINDOWS", fallback=r"C:\cc-test\uc-auto").strip()
    source_win = cfg.get("paths", "SOURCEWINDOWS", fallback="").strip()
    timeout = cfg.getint("limits", "copy_timeout_seconds", fallback=1800)

    box = _Box(host)
    run_id = uuid.uuid4().hex[:8]
    # Per-run sandbox on the box: a throwaway HOME (isolated conf) + staging + dest.
    base = _win_join(dest_root, f"run-{run_id}")
    home = _win_join(base, "home")                 # %USERPROFILE% for this run
    conf_dir = _win_join(home, ".config")
    # The ALL_IN_ONE Windows build reads its config from "Ultracopier.conf" NEXT TO THE EXE
    # (ResourcesManager: writablePath = QApplication::applicationDirPath()), NOT from
    # %USERPROFILE%/.config. So the isolated conf MUST be written into the exe's directory for
    # autoStart / collision policy to take effect (otherwise the window opens and waits, copying
    # nothing). The test box is dedicated, so overwriting that conf each run is fine.
    exe_dir = exe.rsplit("\\", 1)[0] if "\\" in exe else exe
    conf_path = _win_join(exe_dir, "Ultracopier.conf")
    stage = _win_join(base, "src")                 # where the synthetic tree lands
    dest = _win_join(base, "dst")                  # the copy destination

    notes = []
    try:
        # (1) clear any running ultracopier on the box (single-instance lock is per-user).
        box.ps("Stop-Process -Name ultracopier -Force -ErrorAction SilentlyContinue; "
               "Start-Sleep -Milliseconds 800")

        # (2) make the sandbox dirs, then stage the source tree.
        box.ps(f"@('{base}','{home}','{conf_dir}','{dest}') | "
               "ForEach-Object {{ New-Item -ItemType Directory -Force -Path $_ | Out-Null }}"
               .replace("{{", "{").replace("}}", "}"))

        if source_win:
            # Use the real on-box source as-is (nothing pushed -> no secret leaves the box).
            src_on_box = source_win
        else:
            # Push the local synthetic tree via tar (see _Box.push): the archive extracts
            # to <stage>\<basename>, which we point ultracopier at. Multiple sources OK.
            box.ps(f"New-Item -ItemType Directory -Force -Path '{stage}' | Out-Null")
            staged = []
            for s in sources_local:
                s = os.path.normpath(s)
                box.push(s, stage)
                staged.append(_win_join(stage, os.path.basename(s)))
            src_on_box = staged  # list

        # (2b) optional pre-seed: stage a tree INTO the destination so MERGE/OVERWRITE/SKIP
        #      collide against real pre-existing dest content (used by folder_merge /
        #      copy_override). ultracopier copies a source FOLDER into dest -> dest\<basename>;
        #      the pre-seed is laid down at dest\<basename(src[0])>\<...> to match. We push the
        #      CONTENTS of dest_pre_seed (not the dir itself) so files land directly under
        #      pre_dest, not under an extra <tmpdir>/ level.
        if dest_pre_seed is not None:
            pre_base = os.path.basename(os.path.normpath(sources_local[0] if sources_local else "src"))
            pre_dest = _win_join(dest, pre_base)
            box.ps(f"New-Item -ItemType Directory -Force -Path '{pre_dest}' | Out-Null")
            box.push_contents(os.path.normpath(dest_pre_seed), pre_dest)

        # (3) write the isolated conf on the box (UTF-16/CRLF QSettings-INI is fine; we
        #     write plain ASCII keys so default encoding is safe).
        _put_text(box, conf_path, _conf_text(
            file_collision=file_collision, folder_collision=folder_collision,
            file_error=file_error, folder_error=folder_error,
            keep_date=keep_date, do_right=do_right))

        # (4) launch '<exe> <mode> <src...> <dst>' inside a Job Object capped at mem_limit_mb,
        #     with QT_QPA_PLATFORM=windows and USERPROFILE/XDG_CONFIG_HOME pinned to our HOME
        #     so it reads OUR conf and never the box's real ~/.config/ultracopier.conf.
        srcs = src_on_box if isinstance(src_on_box, list) else [src_on_box]
        launch = _launch_script(exe, mode, srcs, dest, home, conf_dir, mem_limit_mb)
        r = box.ps(launch, timeout=60)
        if r.returncode != 0:
            return WindowsResult(notes=f"launch failed rc={r.returncode}: "
                                 f"{(r.stdout + r.stderr).strip()[:500]}")
        launched_at = _parse_kv(r.stdout)
        pid = launched_at.get("PID")
        if not pid:
            return WindowsResult(notes=f"could not read launched PID: {r.stdout.strip()[:500]}")

        # (5) poll for CPU-idle completion (NOT byte-count: IOCP pre-sizes dest files).
        completed, peak, oom, exit_code = _wait_idle(box, pid, timeout, mem_limit_mb)

        stayed_alive = False
        if completed and not oom and exit_code is None:
            # (6) it must still be alive `stay_alive_seconds` later (proves no teardown crash).
            time.sleep(max(0, stay_alive_seconds))
            st = box.ps(f"$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; "
                        "if($p){{Write-Output ('ALIVE=1;WS='+[int]($p.WorkingSet64/1MB))}}"
                        "else{{Write-Output 'ALIVE=0'}}"
                        .replace("{{", "{").replace("}}", "}"))
            kv = _parse_kv(st.stdout)
            stayed_alive = kv.get("ALIVE") == "1"
            if "WS" in kv:
                try: peak = max(peak, int(kv["WS"]))
                except ValueError: pass

        # always kill from OUTSIDE -- never a code "exit after copy" shortcut.
        box.ps(f"Stop-Process -Id {pid} -Force -ErrorAction SilentlyContinue; "
               "Stop-Process -Name ultracopier -Force -ErrorAction SilentlyContinue")

        # (7a) crash scan: Application event-log Event 1000 (WER) for ultracopier.exe during
        #      this run window.  Surface faulting module/exception/offset as mem_errors+notes.
        crashes, crash_note = _scan_crashes(box, launched_at.get("START"))
        if crash_note:
            notes.append(crash_note)

        # (7b) content diff (only when an expectation was given).
        content_ok, diff_text = (True, "")
        if expect is not None:
            content_ok, diff_text = _diff_tree(box, expect, dest, srcs)

        return WindowsResult(
            completed=completed, stayed_alive=stayed_alive, content_ok=content_ok,
            peak_rss_mb=peak, oom_killed=oom, mem_errors=crashes, exit_code=exit_code,
            diff_text=diff_text, notes="; ".join(n for n in notes if n))
    finally:
        # Best-effort sandbox cleanup on the box (absolute, run-scoped path only).
        box.ps(f"Stop-Process -Name ultracopier -Force -ErrorAction SilentlyContinue; "
               f"if (Test-Path '{base}') {{ Remove-Item -LiteralPath '{base}' -Recurse "
               "-Force -ErrorAction SilentlyContinue }")


# ---------------------------------------------------------------------------
# PowerShell snippet builders
# ---------------------------------------------------------------------------
def _launch_script(exe, mode, srcs, dest, home, conf_dir, mem_limit_mb) -> str:
    """Build the PowerShell that launches '<exe> <mode> <src...> <dst>' DETACHED and prints
    PID=...;START=<iso8601> so the poller can find it.

    CRITICAL: launch via WMI Win32_Process.Create, NOT Start-Process. Windows OpenSSH kills every
    child process of the session when the SSH command returns -- so a Start-Process'd ultracopier
    dies the instant this (separate) SSH session closes, copying nothing. Win32_Process.Create
    spawns a process that is NOT part of the SSH session's job, so it survives and runs the copy.

    The ALL_IN_ONE build reads its config from Ultracopier.conf next to the exe (written separately
    in run_windows), so we do NOT need to pass USERPROFILE/XDG_CONFIG_HOME here; and the default Qt
    platform on Windows is already 'windows', so no QT_QPA_PLATFORM is needed either. The memory
    ceiling is a SOFT cap in _wait_idle() (the Job Object's handle cannot survive across SSH
    sessions, so a hard cap is not possible here)."""
    def q(s):  # wrap a CommandLine token in double-quotes; double single-quotes for the PS '...' literal
        return '"' + s.replace("'", "''") + '"'
    cmdline = " ".join([q(exe), mode] + [q(s) for s in (list(srcs) + [dest])])
    body = f"""
$ErrorActionPreference = 'Stop'
$r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{{ CommandLine = '{cmdline}' }}
if ($r.ReturnValue -ne 0) {{ Write-Error ('Win32_Process.Create failed rc=' + $r.ReturnValue); exit 1 }}
$start = (Get-Date).ToUniversalTime().ToString('o')
Write-Output ('PID=' + $r.ProcessId + ';START=' + $start)
"""
    return body


def _wait_idle(box: _Box, pid: str, timeout: int, mem_limit_mb: int = 0):
    """Poll ultracopier.exe CPU time on the box; declare 'completed' after ~8s of no growth.

    Returns (completed, peak_rss_mb, oom_killed, exit_code).
    Enforces a SOFT memory cap: if WorkingSet exceeds mem_limit_mb we kill the process and report
    oom_killed (the portable equivalent of the Linux RSS monitor; the Job Object hard cap was
    removed because its handle cannot survive across SSH sessions). oom_killed is also inferred
    when the process vanishes mid-copy (a disappearance with no clean idle == OOM/crash)."""
    probe = (
        "$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue;"
        "if($p){{Write-Output ('A=1;CPU='+[int64]($p.TotalProcessorTime.TotalMilliseconds)"
        "+';WS='+[int]($p.WorkingSet64/1MB))}}else{{Write-Output 'A=0'}}"
    ).format(pid=pid).replace("{{", "{").replace("}}", "}")

    last_cpu, idle_for, peak, t0 = -1, 0, 0, time.time()
    while time.time() - t0 < timeout:
        r = box.ps(probe, timeout=30)
        kv = _parse_kv(r.stdout)
        if kv.get("A") != "1":
            # gone before going idle -> treat as OOM/crash (caller's crash scan adds detail).
            return (False, peak, True, None)
        try:
            ws = int(kv.get("WS", "0")); peak = max(peak, ws)
        except ValueError:
            ws = 0
        if mem_limit_mb and ws > mem_limit_mb:        # soft memory cap exceeded -> kill + flag OOM
            box.ps(f"Stop-Process -Id {pid} -Force -ErrorAction SilentlyContinue", timeout=30)
            return (False, peak, True, None)
        try:
            cpu = int(kv.get("CPU", "-1"))
        except ValueError:
            cpu = -1
        idle_for = idle_for + 2 if (cpu == last_cpu and last_cpu >= 0) else 0
        last_cpu = cpu
        if idle_for >= 8:
            return (True, peak, False, None)
        time.sleep(2)
    return (False, peak, False, None)  # timed out / hung


def _put_text(box: _Box, remote_path: str, text: str) -> None:
    """Write `text` verbatim to a file on the box (parent dir already created)."""
    import base64
    enc = base64.b64encode(text.encode("utf-8")).decode("ascii")
    script = (
        "$b=[System.Convert]::FromBase64String('{enc}');"
        "[System.IO.File]::WriteAllBytes('{path}',$b)"
    ).format(enc=enc, path=remote_path)
    r = box.ps(script)
    if r.returncode != 0:
        raise RuntimeError(f"writing {remote_path} failed: {(r.stdout+r.stderr).strip()}")


def _scan_crashes(box: _Box, start_iso: str | None):
    """Read Application event log for WER Event 1000 against ultracopier.exe since start.

    Returns (count, note).  The note carries faulting module / exception code / offset so a
    failing case is debuggable straight from the Result, without a second remote session.
    """
    after = start_iso or "(Get-Date).AddMinutes(-10).ToUniversalTime().ToString('o')"
    after_expr = f"[datetime]'{start_iso}'" if start_iso else "(Get-Date).AddMinutes(-10)"
    script = (
        "$ev = Get-WinEvent -FilterHashtable @{{LogName='Application';Id=1000;"
        f"StartTime={after_expr}}} -ErrorAction SilentlyContinue | "
        "Where-Object {{ $_.Message -match 'ultracopier' }};"
        "if($ev){{ $n=@($ev).Count; "
        "$m=($ev | ForEach-Object {{ ($_.Message -replace '\\s+',' ').Substring(0,"
        "[Math]::Min(400,$_.Message.Length)) }}) -join ' || '; "
        "Write-Output ('CRASHES='+$n+';DETAIL='+$m) }}"
        "else{{ Write-Output 'CRASHES=0' }}"
    ).replace("{{", "{").replace("}}", "}")
    r = box.ps(script, timeout=60)
    kv = _parse_kv(r.stdout)
    try:
        n = int(kv.get("CRASHES", "0"))
    except ValueError:
        n = 0
    note = ""
    if n:
        note = f"WER Event 1000 x{n}: " + kv.get("DETAIL", "")[:400]
    return (n, note)


def _diff_tree(box: _Box, expect_local: str, dest_win: str, srcs_win):
    """Diff the LOCAL expectation against the copied tree on the box.

    ultracopier copies the source FOLDER into dest -> dest\\<basename(src)> (same rule as
    harness._copied_tree).  We compare via a SHA-256 manifest produced on each side so we
    verify CONTENT (and, for symlink-less Windows trees, structure) without pulling GBs back.
    Returns (ok, diff_text).
    """
    base = os.path.basename(os.path.normpath(srcs_win[0].replace("\\", "/")))
    # mirror harness._copied_tree: prefer dest\<basename> if it is a directory.
    cand = _win_join(dest_win, base)
    probe = (
        "if (Test-Path -LiteralPath '{cand}' -PathType Container) "
        "{{ Write-Output '{cand}' }} else {{ Write-Output '{dest}' }}"
    ).format(cand=cand, dest=dest_win).replace("{{", "{").replace("}}", "}")
    root = box.ps(probe).stdout.strip() or dest_win

    remote_manifest = _hash_manifest_remote(box, root)
    local_manifest = _hash_manifest_local(expect_local)
    if remote_manifest == local_manifest:
        return (True, "")
    # produce a readable delta
    rm = dict(_parse_manifest(remote_manifest))
    lm = dict(_parse_manifest(local_manifest))
    lines = []
    for k in sorted(set(lm) | set(rm)):
        if k not in rm:
            lines.append(f"missing on box: {k}")
        elif k not in lm:
            lines.append(f"extra on box:   {k}")
        elif lm[k] != rm[k]:
            lines.append(f"differs:        {k}")
    return (False, "\n".join(lines))


def _hash_manifest_remote(box: _Box, root_win: str) -> str:
    """Return 'relpath<TAB>sha256' (file) or 'relpath<TAB>LINK<Tab>target' (symlink) lines.

    A symlink is compared by its TARGET string (like async readlink+symlink does), not by the
    content of what it points to -- a dangling or abs-path link has no meaningful content to hash
    and the parity guarantee is "the link is recreated verbatim", not "the target is copied".

    Files are hashed via the \\\\?\ prefix so Win32 path-normalization does NOT strip trailing
    dots/spaces (a name like 'trailing_dot.' is storable on NTFS via \\?\ but invisible to the
    plain Win32 API, which would strip the dot -> 'file not found' -> false diff)."""
    script = (
        "$root='{root}';"
        "if(-not (Test-Path -LiteralPath $root)){{ Write-Output ''; exit }}"
        "$rootFull=(Resolve-Path -LiteralPath $root).Path.TrimEnd('\\')+'\\';"
        "Get-ChildItem -LiteralPath $root -Recurse -Force | "
        "Where-Object {{ $_.Attributes -band [IO.FileAttributes]::ReparsePoint }} | "
        "Sort-Object FullName | ForEach-Object {{ "
        "  $rel=$_.FullName.Substring($rootFull.Length).Replace('\\','/');"
        "  $t=(Get-Item -LiteralPath $_.FullName).Target;"
        "  if($t -is [array]){{$t=$t -join ';'}};"
        "  Write-Output ($rel + \"`tLINK`t\" + $t) }};"
        "Get-ChildItem -LiteralPath $root -Recurse -File -Force | "
        "Where-Object {{ -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) }} | "
        "Sort-Object FullName | ForEach-Object {{ "
        "  $rel=$_.FullName.Substring($rootFull.Length).Replace('\\','/');"
        "  $prefixed='\\\\?\\' + $_.FullName;"
        "  $h=(Get-FileHash -LiteralPath $prefixed -Algorithm SHA256).Hash.ToLower();"
        "  Write-Output ($rel + \"`t\" + $h) }}"
    ).format(root=root_win).replace("{{", "{").replace("}}", "}")
    r = box.ps(script, timeout=600)
    return "\n".join(sorted(ln for ln in r.stdout.splitlines() if ln.strip()))


def _hash_manifest_local(root_local: str) -> str:
    """Mirror of _hash_manifest_remote for the local expectation: files by SHA-256, symlinks by
    target (readlink). The Linux source can have abs-path / dangling links (meaningless on
    Windows), but the parity contract is 'the link target string is preserved', which holds."""
    import hashlib
    root = pathlib.Path(root_local)
    out = []
    for p in sorted(root.rglob("*")):
        if p.is_symlink():
            rel = p.relative_to(root).as_posix()
            out.append(f"{rel}\tLINK\t{os.readlink(p)}")
        elif p.is_file():
            h = hashlib.sha256(p.read_bytes()).hexdigest()
            rel = p.relative_to(root).as_posix()
            out.append(f"{rel}\t{h}")
    return "\n".join(sorted(out))


def _parse_manifest(text: str):
    """Yield (relpath, value) pairs. value is the sha256 for a file, or 'LINK<tab>target'
    for a symlink (so the delta printer shows the link target inline)."""
    for ln in text.splitlines():
        if not ln.strip() or "\t" not in ln:
            continue
        parts = ln.split("\t", 2)
        rel = parts[0]
        if len(parts) == 3 and parts[1] == "LINK":
            yield rel, "LINK\t" + parts[2]
        elif len(parts) >= 2:
            yield rel, parts[1].strip().lower()


# ---------------------------------------------------------------------------
# Tiny KV parser for our 'A=1;CPU=...;WS=...' style status lines.
# ---------------------------------------------------------------------------
def _parse_kv(stdout: str) -> dict:
    out = {}
    for line in stdout.splitlines():
        line = line.strip()
        if "=" in line and (";" in line or line.count("=") == 1):
            for tok in line.split(";"):
                if "=" in tok:
                    k, v = tok.split("=", 1)
                    out[k.strip()] = v.strip()
    return out
