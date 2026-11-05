# First-logon provisioning of the Ultracopier Windows 11 test VM (run elevated by autounattend.xml).
# Every step logs to C:\uc-setup.log; C:\uc-setup.done is written only when all steps succeeded.
Start-Transcript -Path C:\uc-setup.log -Append | Out-Null
$failed = @()
function Step($name, [scriptblock]$body) {
    try { & $body; Write-Output "OK   $name" }
    catch { Write-Output "FAIL $name : $_"; $script:failed += $name }
}
function RegSet($path, $name, $value, $type = 'DWord') {
    if (-not (Test-Path $path)) { New-Item -Path $path -Force -ErrorAction Stop | Out-Null }
    New-ItemProperty -Path $path -Name $name -Value $value -PropertyType $type -Force -ErrorAction Stop | Out-Null
}
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

# sshd first: the OpenSSH capability is fetched through Windows Update, which is disabled below
Step 'openssh server' {
    if (-not (Get-Service sshd -ErrorAction SilentlyContinue)) {
        Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0 -ErrorAction Stop | Out-Null
    }
    Set-Service sshd -StartupType Automatic -ErrorAction Stop
    Start-Service sshd -ErrorAction Stop
}
Step 'ssh key' {
    $ak = 'C:\ProgramData\ssh\administrators_authorized_keys'
    Copy-Item -LiteralPath (Join-Path $here 'authorized_keys') -Destination $ak -Force -ErrorAction Stop
    & icacls.exe $ak /inheritance:r /grant 'Administrators:F' /grant 'SYSTEM:F' | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "icacls rc=$LASTEXITCODE" }
}
Step 'firewall ssh' {
    if (-not (Get-NetFirewallRule -Name 'uc-sshd' -ErrorAction SilentlyContinue)) {
        New-NetFirewallRule -Name 'uc-sshd' -DisplayName 'sshd (uc test)' -Direction Inbound -Protocol TCP -LocalPort 22 -Action Allow -ErrorAction Stop | Out-Null
    }
}

# permanent auto-logon: the GUI tests need an unlocked interactive desktop after every boot
Step 'autologon' {
    $wl = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'
    RegSet $wl 'AutoAdminLogon' '1' 'String'
    RegSet $wl 'DefaultUserName' 'uc' 'String'
    RegSet $wl 'DefaultPassword' 'uc' 'String'
    Remove-ItemProperty -Path $wl -Name 'AutoLogonCount' -ErrorAction SilentlyContinue
}
Step 'no sleep / lock / screensaver' {
    foreach ($a in @('monitor-timeout-ac', 'standby-timeout-ac', 'hibernate-timeout-ac')) {
        & powercfg.exe /change $a 0
        if ($LASTEXITCODE -ne 0) { throw "powercfg $a rc=$LASTEXITCODE" }
    }
    RegSet 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\Personalization' 'NoLockScreen' 1
    RegSet 'HKCU:\Control Panel\Desktop' 'ScreenSaveActive' '0' 'String'
}
# no UAC prompt: a secure-desktop prompt would block a headless run
Step 'uac no prompt' {
    $p = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System'
    RegSet $p 'ConsentPromptBehaviorAdmin' 0
    RegSet $p 'PromptOnSecureDesktop' 0
}
# no first-run popups stealing the foreground from the tests
Step 'no popups' {
    RegSet 'HKCU:\Software\Microsoft\Windows\CurrentVersion\UserProfileEngagement' 'ScoobeSystemSettingEnabled' 0
    RegSet 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\CloudContent' 'DisableWindowsConsumerFeatures' 1
    RegSet 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\OneDrive' 'DisableFileSyncNGSC' 1
    Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'OneDriveSetup' -ErrorAction SilentlyContinue
}

# Windows Update off: no download, no install, no reboot during a test run
Step 'windows update policy' {
    $wu = 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate'
    RegSet "$wu\AU" 'NoAutoUpdate' 1
    RegSet "$wu\AU" 'AUOptions' 1
    RegSet "$wu\AU" 'NoAutoRebootWithLoggedOnUsers' 1
    RegSet $wu 'DisableWindowsUpdateAccess' 1
    RegSet $wu 'DoNotConnectToWindowsUpdateInternetLocations' 1
    RegSet 'HKLM:\SOFTWARE\Policies\Microsoft\WindowsStore' 'AutoDownload' 2
    RegSet 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\DeliveryOptimization' 'DODownloadMode' 0
}
Step 'windows update services' {
    Stop-Service wuauserv -Force -ErrorAction SilentlyContinue
    Set-Service wuauserv -StartupType Disabled -ErrorAction Stop
    # UsoSvc/WaaSMedicSvc refuse Set-Service and may be TrustedInstaller-only: best effort, the policies above already block updates
    foreach ($s in @('UsoSvc', 'WaaSMedicSvc')) {
        try { RegSet "HKLM:\SYSTEM\CurrentControlSet\Services\$s" 'Start' 4 }
        catch { Write-Output "WARN cannot disable $s : $_" }
    }
}
Step 'windows update tasks' {
    Get-ScheduledTask -TaskPath '\Microsoft\Windows\WindowsUpdate\*' -ErrorAction SilentlyContinue |
        Disable-ScheduledTask -ErrorAction SilentlyContinue | Out-Null
}

if ($failed.Count -eq 0) {
    New-Item -ItemType File -Path C:\uc-setup.done -Force | Out-Null
    Write-Output 'SETUP COMPLETE'
} else {
    Write-Output ('SETUP FAILED: ' + ($failed -join ', '))
}
Stop-Transcript | Out-Null
