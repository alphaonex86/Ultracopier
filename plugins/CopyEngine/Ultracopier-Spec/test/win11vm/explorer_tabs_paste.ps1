# Guest side of cases/explorer_tabs_paste.py. Must run in the INTERACTIVE session (scheduled task):
# a real Explorer window with 3 tabs, a file put on the clipboard, Ctrl+V injected into the active tab.
# Expected: the PluginLoader keyboard hook swallows the paste and Ultracopier copies into the ACTIVE tab's folder.
# Writes $Work\result.json: status pass|fail|error, one entry per pasted tab.
param([Parameter(Mandatory = $true)][string]$Exe, [Parameter(Mandatory = $true)][string]$Work)
$ErrorActionPreference = 'Stop'
$result = [ordered]@{ status = 'error'; detail = ''; cases = @() }
$tabs = @('TabOne', 'TabTwo', 'TabThree')
$log = Join-Path $Work 'transfer.log'

Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System; using System.Runtime.InteropServices; using System.Text;
public static class W {
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, IntPtr pid);
    [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("user32.dll")] static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    public static string Title(IntPtr h) { var s = new StringBuilder(512); GetWindowText(h, s, 512); return s.ToString(); }
    public static bool IsForeground(IntPtr h) { return GetForegroundWindow() == h; }
    // foreground-lock workaround: share the input state of the current foreground thread while switching
    public static bool Focus(IntPtr h) {
        uint fg = GetWindowThreadProcessId(GetForegroundWindow(), IntPtr.Zero), me = GetCurrentThreadId();
        bool attached = fg != me && AttachThreadInput(me, fg, true);
        BringWindowToTop(h); SetForegroundWindow(h);
        if (attached) AttachThreadInput(me, fg, false);
        return GetForegroundWindow() == h;
    }
    public static void Chord(byte[] mods, byte vk) {
        foreach (byte m in mods) keybd_event(m, 0, 0, UIntPtr.Zero);
        keybd_event(vk, 0, 0, UIntPtr.Zero); keybd_event(vk, 0, 2, UIntPtr.Zero);
        for (int i = mods.Length - 1; i >= 0; i--) keybd_event(mods[i], 0, 2, UIntPtr.Zero);
    }
}
'@
$VK_CONTROL = [byte]0x11; $VK_SHIFT = [byte]0x10

function Fail([string]$why) { throw $why }
function WaitFor([scriptblock]$cond, [int]$seconds, [string]$what) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $v = & $cond
        if ($v) { return $v }
        Start-Sleep -Milliseconds 250
    }
    Fail "timeout after ${seconds}s waiting for $what"
}
function CloseExplorerWindows($shell) {
    # Quit() on a tab item does not always close the other tabs of its window: retry until none is left
    WaitFor { foreach ($w in @($shell.Windows())) { try { $w.Quit() } catch {} }; @($shell.Windows()).Count -eq 0 } 30 'Explorer windows to close' | Out-Null
}
function FolderOf($item) {
    try { return $item.Document.Folder.Self.Path } catch { return '' }
}

$shell = $null
try {
    $shell = New-Object -ComObject Shell.Application
    CloseExplorerWindows $shell
    # fresh folders: one per tab + the clipboard sources
    foreach ($d in $tabs + @('src')) {
        $p = Join-Path $Work $d
        if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Recurse -Force }
        New-Item -ItemType Directory -Path $p | Out-Null
    }
    foreach ($t in $tabs) { Set-Content -LiteralPath (Join-Path $Work "src\p$t.txt") -Value "pasted into $t" }
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }

    # Ultracopier, in this interactive session so its low-level keyboard hook sees the injected keys
    Get-Process ultracopier -ErrorAction SilentlyContinue | Stop-Process -Force
    WaitFor { -not (Get-Process ultracopier -ErrorAction SilentlyContinue) } 20 'old ultracopier to exit' | Out-Null
    $uc = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path -Parent $Exe) -PassThru
    WaitFor { [System.IO.Directory]::GetFiles('\\.\pipe\') -match 'advanced-copier' } 60 'ultracopier listener pipe' | Out-Null
    Start-Sleep -Seconds 3   # the PluginLoader hook is installed during the same startup
    if ($uc.HasExited) { Fail "ultracopier exited at startup, code $($uc.ExitCode)" }

    # one Explorer window, 3 tabs
    Start-Process explorer.exe (Join-Path $Work $tabs[0])
    $first = WaitFor { @($shell.Windows()) | Where-Object { (FolderOf $_) -eq (Join-Path $Work $tabs[0]) } | Select-Object -First 1 } 30 'the Explorer window'
    $hwnd = [IntPtr]$first.HWND
    foreach ($t in $tabs[1..2]) {
        $before = @($shell.Windows() | Where-Object { $_.HWND -eq $hwnd.ToInt64() }).Count
        if (-not [W]::Focus($hwnd)) { Fail 'cannot bring the Explorer window to the foreground' }
        [W]::Chord(@($VK_CONTROL), [byte][char]'T')
        WaitFor { @($shell.Windows() | Where-Object { $_.HWND -eq $hwnd.ToInt64() }).Count -gt $before } 20 "new tab for $t" | Out-Null
        $mine = $tabs | ForEach-Object { Join-Path $Work $_ }
        $new = WaitFor { @($shell.Windows()) | Where-Object { $_.HWND -eq $hwnd.ToInt64() -and $mine -notcontains (FolderOf $_) } | Select-Object -First 1 } 20 "the new tab item for $t"
        $new.Navigate2((Join-Path $Work $t))
        WaitFor { [W]::Title($hwnd) -like "$t*" } 20 "tab $t to be active (title)" | Out-Null
    }

    # paste into the last, middle then first tab (the bug always pasted into the first one)
    $order = @('TabThree', 'TabTwo', 'TabOne')
    for ($i = 0; $i -lt $order.Count; $i++) {
        $t = $order[$i]
        if (-not [W]::Focus($hwnd)) { Fail "cannot bring Explorer to the foreground before pasting into $t" }
        if ($i -gt 0) { [W]::Chord(@($VK_CONTROL, $VK_SHIFT), [byte]0x09) }   # Ctrl+Shift+Tab: previous tab
        WaitFor { [W]::Title($hwnd) -like "$t*" } 10 "tab $t to be active (title is '$([W]::Title($hwnd))')" | Out-Null
        $files = New-Object System.Collections.Specialized.StringCollection
        $files.Add((Join-Path $Work "src\p$t.txt")) | Out-Null
        [System.Windows.Forms.Clipboard]::SetFileDropList($files)
        Start-Sleep -Milliseconds 500
        if (-not [W]::IsForeground($hwnd)) { Fail "Explorer lost the foreground before pasting into $t" }
        [W]::Chord(@($VK_CONTROL), [byte][char]'V')

        $name = "p$t.txt"
        $landed = @()
        $deadline = (Get-Date).AddSeconds(30)
        while ($landed.Count -eq 0 -and (Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 250
            $landed = @($tabs | Where-Object { Test-Path -LiteralPath (Join-Path $Work "$_\$name") })
        }
        Start-Sleep -Seconds 2   # a late duplicate would show up here
        $landed = @($tabs | Where-Object { Test-Path -LiteralPath (Join-Path $Work "$_\$name") })
        # the transfer log proves Ultracopier did the copy (an uncaught Ctrl+V is Explorer's own paste)
        $byUltracopier = $false
        $deadline = (Get-Date).AddSeconds(15)
        while (-not $byUltracopier -and (Get-Date) -lt $deadline) {
            if (Test-Path -LiteralPath $log) {
                $byUltracopier = [bool]((Get-Content -LiteralPath $log -Raw) -replace '/', '\' -match [regex]::Escape("\$t\$name"))
            }
            if (-not $byUltracopier) { Start-Sleep -Milliseconds 250 }
        }
        $ok = ($landed.Count -eq 1 -and $landed[0] -eq $t -and $byUltracopier)
        $result.cases += [ordered]@{ tab = $t; landed_in = $landed; by_ultracopier = $byUltracopier; ok = $ok }
    }
    $result.status = if (@($result.cases | Where-Object { -not $_.ok }).Count -eq 0) { 'pass' } else { 'fail' }
} catch {
    $result.detail = "$_"
} finally {
    try { if ($shell) { CloseExplorerWindows $shell } } catch { $result.detail += " | cleanup: $_" }
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Work 'result.json') -Encoding UTF8
}
