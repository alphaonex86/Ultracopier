#!/bin/bash
# Windows 11 test VM: q35 + Secure Boot (OVMF, MS keys) + swtpm TPM2, user-mode net with ssh on 127.0.0.1:$SSH_PORT.
#   vm.sh install   create the disk and run the unattended install (headless, ends powered off)
#   vm.sh run       boot with a GTK window (desktop use)
#   vm.sh start     boot headless in the background (VNC on 127.0.0.1:$VNC_DISPLAY)
#   vm.sh stop      ACPI shutdown, forced quit after a bounded wait
#   vm.sh ssh ...   ssh into the guest with the VM's own key
#   vm.sh shot F    save a screenshot of the guest screen to F (.png)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DIR="${UC_WIN11_DIR:-/mnt/data/perso/pc-virtuel/windows-11-64}"
ISO="${UC_WIN11_ISO:-/mnt/data/read/iso/windows/Win11_24H2_EnglishInternational_x64.iso}"
SSH_PORT="${UC_WIN11_SSH_PORT:-2222}"
VNC_DISPLAY="${UC_WIN11_VNC:-11}"
OVMF_CODE=/usr/share/edk2/OvmfX64/OVMF_CODE_4M.secboot.qcow2
OVMF_VARS=/usr/share/edk2/OvmfX64/OVMF_VARS_4M.secboot.qcow2   # MS keys enrolled
MON="$DIR/monitor.sock"
PIDFILE="$DIR/qemu.pid"

die() { echo "vm.sh: $*" >&2; exit 1; }
running() { [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; }
monitor() { echo "$1" | socat - "UNIX-CONNECT:$MON" >/dev/null 2>&1; }
ssh_guest() {
    ssh -i "$DIR/ssh/id_ed25519" -p "$SSH_PORT" -o BatchMode=yes -o ConnectTimeout=10 \
        -o StrictHostKeyChecking=accept-new -o UserKnownHostsFile="$DIR/ssh/known_hosts" uc@127.0.0.1 "$@"
}

start_tpm() {
    mkdir -p "$DIR/tpm" || die "cannot create $DIR/tpm"
    rm -f "$DIR/tpm/sock"
    swtpm socket --tpm2 --tpmstate dir="$DIR/tpm" --ctrl type=unixio,path="$DIR/tpm/sock" \
        --terminate --daemon || die "swtpm failed to start"
    local i=0
    while [ ! -S "$DIR/tpm/sock" ]; do
        [ $i -ge 50 ] && die "swtpm socket never appeared"
        sleep 0.1; i=$((i+1))
    done
}

qemu_args() {
    QEMU_ARGS=(-name win11 -machine q35,smm=on,accel=kvm \
        -cpu host,hv_relaxed,hv_vapic,hv_spinlocks=0x1fff,hv_time -smp 8 -m 8192 \
        -global driver=cfi.pflash01,property=secure,value=on \
        -drive if=pflash,format=qcow2,unit=0,readonly=on,file="$OVMF_CODE" \
        -drive if=pflash,format=qcow2,unit=1,file="$DIR/nvram.qcow2" \
        -chardev socket,id=chrtpm,path="$DIR/tpm/sock" -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0 \
        -drive file="$DIR/hdd.qcow2",if=none,id=hd,format=qcow2,aio=io_uring -device ide-hd,drive=hd,bus=ide.0,bootindex=0 \
        -nic user,model=e1000e,hostfwd=tcp:127.0.0.1:$SSH_PORT-:22 \
        -usb -device usb-tablet -vga virtio \
        -monitor unix:"$MON",server,nowait)
}

wait_exit() {   # $1 = seconds
    local i=0
    while running; do
        [ $i -ge "$1" ] && return 1
        sleep 1; i=$((i+1))
    done
    return 0
}

cmd_install() {
    [ -e "$DIR/hdd.qcow2" ] && die "$DIR/hdd.qcow2 already exists (installed VM): refusing to overwrite"
    [ -f "$ISO" ] || die "ISO not found: $ISO"
    mkdir -p "$DIR/ssh" "$DIR/unattend" || die "cannot create $DIR"
    [ -f "$DIR/ssh/id_ed25519" ] || ssh-keygen -q -t ed25519 -N "" -C uc-win11-vm -f "$DIR/ssh/id_ed25519" || die "ssh-keygen failed"
    cp "$HERE/autounattend.xml" "$HERE/setup.ps1" "$DIR/unattend/" || die "copy unattend files failed"
    cp "$DIR/ssh/id_ed25519.pub" "$DIR/unattend/authorized_keys" || die "copy pubkey failed"
    mkisofs -quiet -J -R -V UNATTEND -o "$DIR/unattend.iso" "$DIR/unattend" || die "mkisofs failed"
    cp "$OVMF_VARS" "$DIR/nvram.qcow2" || die "copy OVMF vars failed"
    qemu-img create -q -f qcow2 "$DIR/hdd.qcow2" 80G || die "qemu-img failed"
    start_tpm
    qemu_args
    qemu-system-x86_64 "${QEMU_ARGS[@]}" \
        -drive file="$ISO",if=none,id=cd0,media=cdrom,readonly=on -device ide-cd,drive=cd0,bus=ide.1,bootindex=1 \
        -drive file="$DIR/unattend.iso",if=none,id=cd1,media=cdrom,readonly=on -device ide-cd,drive=cd1,bus=ide.2 \
        -display none -vnc 127.0.0.1:"$VNC_DISPLAY" -daemonize -pidfile "$PIDFILE" || die "qemu failed to start"
    # answer the ISO's "Press any key to boot from CD or DVD" prompt
    local i=0
    while [ $i -lt 40 ]; do monitor "sendkey ret"; sleep 0.5; i=$((i+1)); done
    echo "install running (VNC 127.0.0.1:$((5900+VNC_DISPLAY))), waiting for the first-logon setup to finish..."
    i=0
    until ssh_guest "if exist C:\\uc-setup.done echo SETUP_DONE" 2>/dev/null | grep -q SETUP_DONE; do
        running || die "qemu exited during the install"
        [ $i -ge 240 ] && die "install not finished after 2h (screenshot: vm.sh shot /tmp/x.png)"
        sleep 30; i=$((i+1))
    done
    echo "setup done, shutting down"
    cmd_stop
}

cmd_boot() {   # $1 = gui|headless
    running && die "already running (pid $(cat "$PIDFILE"))"
    [ -e "$DIR/hdd.qcow2" ] || die "not installed: run vm.sh install first"
    start_tpm
    qemu_args
    if [ "$1" = gui ]; then
        exec qemu-system-x86_64 "${QEMU_ARGS[@]}" -pidfile "$PIDFILE"
    fi
    qemu-system-x86_64 "${QEMU_ARGS[@]}" -display none -vnc 127.0.0.1:"$VNC_DISPLAY" -daemonize -pidfile "$PIDFILE" \
        || die "qemu failed to start"
}

cmd_stop() {
    running || { echo "not running"; return 0; }
    # Windows ignores the ACPI button while a session is logged on: ask the guest first
    if ! ssh_guest "shutdown /s /f /t 0" >/dev/null 2>&1; then
        echo "guest shutdown over ssh failed, trying ACPI" >&2
        monitor "system_powerdown"
    fi
    if ! wait_exit 180; then
        echo "no shutdown after 180s, forcing quit" >&2
        monitor "quit"
        wait_exit 20 || die "qemu still alive"
    fi
    rm -f "$PIDFILE"
}

cmd_shot() {
    running || die "not running"
    local ppm="$DIR/shot.ppm"
    rm -f "$ppm"
    monitor "screendump $ppm"
    sleep 1
    magick "$ppm" "$1" || die "magick failed"
    rm -f "$ppm"
}

case "${1:-}" in
    install) cmd_install ;;
    run)     cmd_boot gui ;;
    start)   cmd_boot headless ;;
    stop)    cmd_stop ;;
    ssh)     shift; ssh_guest "$@" ;;
    shot)    [ -n "${2:-}" ] || die "usage: vm.sh shot out.png"; cmd_shot "$2" ;;
    *)       die "usage: vm.sh install|run|start|stop|ssh ...|shot out.png" ;;
esac
