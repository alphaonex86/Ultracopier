#!/usr/bin/env python3
"""
macdeployqt_linux.py -- Linux-native macdeployqt equivalent for osxcross.

Qt's own macdeployqt ships only as a Mach-O binary, so it can't run on a Linux
build host. After the osxcross cmake build produces Ultracopier.app this:

  1. Copies the Qt plugin categories the app may dlopen into Contents/PlugIns/.
  2. Walks every Mach-O in the bundle (exe, plugins, and each framework it
     copies, recursively: QtGui -> QtDBus is only a transitive dependency) and
     copies each referenced Qt framework into Contents/Frameworks/.
  3. Rewrites every Qt dependency to @executable_path/../Frameworks/..., sets
     each framework's own id the same way, and deletes absolute (build host)
     LC_RPATH entries.
  4. Writes Contents/Resources/qt.conf so QtCore finds the bundled plugins.
  5. Ad-hoc signs the bundle with rcodesign: install_name_tool invalidates the
     arm64 signatures and Apple Silicon kills a binary with a broken one.
  6. Verifies that every dependency of every Mach-O resolves inside the bundle
     or to the system and that every Mach-O signature is valid; any miss fails
     the deploy instead of shipping a .app that dies at launch.

Uses osxcross's Linux-native <triple>-otool / <triple>-install_name_tool and
rcodesign (cargo install apple-codesign).

Usage:
  python3 macdeployqt_linux.py --app Ultracopier.app \
      --qt /path/qt6-macos/6.5.3/macos \
      --osxcross-bin /path/osxcross/target/bin --triple x86_64-apple-darwin20.4 \
      --rcodesign /path/bin/rcodesign
"""

import argparse
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys


def run(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        sys.stderr.write(f"[macdeployqt_linux] FAIL: {' '.join(cmd)}\n{p.stdout}")
        sys.exit(p.returncode if p.returncode > 0 else 1)
    return p.stdout


_LIB_RE = re.compile(r"^\s+(\S+)\s+\(compatibility")
_RPATH_RE = re.compile(r"^\s+path\s+(\S+)\s+\(offset")


def list_deps(otool, target):
    """Install names otool -L reports (both slices of a fat binary, deduplicated)."""
    deps = []
    for line in run([otool, "-L", target]).splitlines():
        m = _LIB_RE.match(line)
        if m and m.group(1) not in deps:
            deps.append(m.group(1))
    return deps


def list_rpaths(otool, target):
    rpaths = []
    in_rpath = False
    for line in run([otool, "-l", target]).splitlines():
        if line.strip().startswith("cmd "):
            in_rpath = line.strip() == "cmd LC_RPATH"
        elif in_rpath:
            m = _RPATH_RE.match(line)
            if m and m.group(1) not in rpaths:
                rpaths.append(m.group(1))
    return rpaths


_MACHO_MAGICS = (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca")


def is_macho(path):
    with open(path, "rb") as f:
        return f.read(4) in _MACHO_MAGICS


def macho_files(app):
    found = []
    for root, dirs, files in os.walk(app):
        for n in files:
            p = os.path.join(root, n)
            if os.path.isfile(p) and not os.path.islink(p) and is_macho(p):
                found.append(p)
    return found


_CPU_TYPE_ARM64 = 0x0100000c
_HASH_NAMES = {1: "sha1", 2: "sha256", 3: "sha256"}   # CS_HASHTYPE_SHA1 / SHA256 / SHA256_TRUNCATED


def macho_slices(data):
    """[(cputype, offset)] of a thin or fat 64-bit Mach-O."""
    if data[:4] == b"\xca\xfe\xba\xbe":
        count = struct.unpack_from(">I", data, 4)[0]
        return [struct.unpack_from(">iiI", data, 8 + 20 * i)[0::2] for i in range(count)]
    return [(struct.unpack_from("<i", data, 4)[0], 0)]


def slice_signature_problem(data, cputype, off):
    """None when the slice's CodeDirectory page hashes match its content (what the kernel checks)."""
    if data[off:off + 4] != b"\xcf\xfa\xed\xfe":
        return "not a 64-bit Mach-O slice"
    ncmds = struct.unpack_from("<I", data, off + 16)[0]
    cmd_off = off + 32
    sig_off = None
    for i in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, cmd_off)
        if cmd == 0x1d:   # LC_CODE_SIGNATURE
            sig_off = off + struct.unpack_from("<I", data, cmd_off + 8)[0]
        cmd_off += cmdsize
    if sig_off is None:
        return "arm64 slice not signed" if cputype == _CPU_TYPE_ARM64 else None
    magic, length, count = struct.unpack_from(">III", data, sig_off)
    if magic != 0xfade0cc0:
        return "corrupt code signature SuperBlob"
    cd = None
    for i in range(count):
        blob_type, blob_off = struct.unpack_from(">II", data, sig_off + 12 + 8 * i)
        if blob_type == 0:
            cd = sig_off + blob_off
    if cd is None or struct.unpack_from(">I", data, cd)[0] != 0xfade0c02:
        return "no CodeDirectory in the code signature"
    (hash_offset, n_code_slots, code_limit, hash_size, hash_type,
     page_shift) = (struct.unpack_from(">I", data, cd + 16)[0],) + struct.unpack_from(">IIBBxB", data, cd + 28)
    if hash_type not in _HASH_NAMES:
        return f"unknown code signature hash type {hash_type}"
    page = 1 << page_shift if page_shift else code_limit
    for k in range(n_code_slots):
        start = k * page
        digest = hashlib.new(_HASH_NAMES[hash_type], data[off + start:off + min(start + page, code_limit)]).digest()
        recorded = data[cd + hash_offset + k * hash_size:cd + hash_offset + (k + 1) * hash_size]
        if digest[:hash_size] != recorded:
            return f"code signature page {k} does not match the content"
    return None


def signature_problems(path):
    with open(path, "rb") as f:
        data = f.read()
    problems = []
    for cputype, off in macho_slices(data):
        problem = slice_signature_problem(data, cputype, off)
        if problem is not None:
            problems.append(f"{path}: cputype {cputype:#x}: {problem}")
    return problems


def framework_tail(dep, qt_lib_prefix):
    """'QtCore.framework/Versions/A/QtCore' for a Qt framework dependency, else None."""
    if not (dep.startswith(qt_lib_prefix) or dep.startswith("@rpath/Qt")):
        return None
    mi = dep.find(".framework/")
    if mi < 0:
        return None
    return dep[dep.rfind("/", 0, mi) + 1:]


# Plugin categories Ultracopier can dlopen (no sqldrivers / multimedia: unused).
_PLUGIN_CATEGORIES = ("platforms", "imageformats", "iconengines", "tls",
                      "networkinformation", "styles", "generic")


def copy_plugins(app, qt_prefix):
    src_plugins = os.path.join(qt_prefix, "plugins")
    dst_plugins = os.path.join(app, "Contents", "PlugIns")
    if not os.path.isdir(os.path.join(src_plugins, "platforms")):
        sys.stderr.write(f"[macdeployqt_linux] no platforms plugins under {src_plugins}\n")
        sys.exit(1)
    for cat in _PLUGIN_CATEGORIES:
        src_cat = os.path.join(src_plugins, cat)
        if os.path.isdir(src_cat):
            dst_cat = os.path.join(dst_plugins, cat)
            if os.path.exists(dst_cat):
                shutil.rmtree(dst_cat)
            shutil.copytree(src_cat, dst_cat, symlinks=True)


def deploy(app, qt_prefix, otool, install_name_tool):
    fw_dst = os.path.join(app, "Contents", "Frameworks")
    os.makedirs(fw_dst, exist_ok=True)
    qt_lib_prefix = os.path.join(qt_prefix, "lib") + "/"
    todo = macho_files(app)
    copied = set()
    while todo:
        target = todo.pop()
        for dep in list_deps(otool, target):
            tail = framework_tail(dep, qt_lib_prefix)
            if tail is not None:
                fw_name = tail[:tail.find("/")]
                if fw_name not in copied:
                    src_fw = os.path.join(qt_prefix, "lib", fw_name)
                    if not os.path.isdir(src_fw):
                        sys.stderr.write(f"[macdeployqt_linux] {target} needs {dep}, "
                                         f"but {src_fw} does not exist\n")
                        sys.exit(1)
                    dst = os.path.join(fw_dst, fw_name)
                    if os.path.exists(dst):
                        shutil.rmtree(dst)
                    shutil.copytree(src_fw, dst, symlinks=True, ignore=shutil.ignore_patterns("Headers"))
                    copied.add(fw_name)
                    # The real file, not the top-level symlink, else the walk never recurses.
                    todo.append(os.path.join(fw_dst, tail))
                new_name = "@executable_path/../Frameworks/" + tail
                if dep != new_name:
                    run([install_name_tool, "-change", dep, new_name, target])
        for rpath in list_rpaths(otool, target):
            if rpath.startswith("/"):
                run([install_name_tool, "-delete_rpath", rpath, target])
    for fw in copied:
        short = fw[:-len(".framework")]
        fw_bin = os.path.join(fw_dst, fw, "Versions", "A", short)
        if not os.path.isfile(fw_bin):
            sys.stderr.write(f"[macdeployqt_linux] unexpected framework layout: {fw_bin} missing\n")
            sys.exit(1)
        run([install_name_tool, "-id",
             "@executable_path/../Frameworks/" + fw + "/Versions/A/" + short, fw_bin])


def write_qt_conf(app):
    res = os.path.join(app, "Contents", "Resources")
    os.makedirs(res, exist_ok=True)
    with open(os.path.join(res, "qt.conf"), "w") as f:
        f.write("[Paths]\nPlugins = PlugIns\n")


_SYSTEM_PREFIXES = ("/usr/lib/", "/System/Library/")


def verify(app, otool):
    """Return the list of problems: anything dyld or the kernel would refuse on a user's Mac."""
    exe_dir = os.path.join(app, "Contents", "MacOS")
    problems = []
    for target in macho_files(app):
        for dep in list_deps(otool, target):
            if dep.startswith("@executable_path/"):
                if not os.path.isfile(os.path.join(exe_dir, dep[len("@executable_path/"):])):
                    problems.append(f"{target}: {dep} not in bundle")
            elif dep.startswith("@loader_path/"):
                if not os.path.isfile(os.path.join(os.path.dirname(target), dep[len("@loader_path/"):])):
                    problems.append(f"{target}: {dep} not in bundle")
            elif not dep.startswith(_SYSTEM_PREFIXES):
                problems.append(f"{target}: unresolved dependency {dep}")
        for rpath in list_rpaths(otool, target):
            if rpath.startswith("/"):
                problems.append(f"{target}: absolute rpath {rpath}")
        problems += signature_problems(target)
    return problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--app", required=True, help="Path to the .app bundle to deploy.")
    ap.add_argument("--qt", required=True, help="Qt for macOS install prefix on the build host.")
    ap.add_argument("--osxcross-bin", required=True, help="osxcross target/bin dir.")
    ap.add_argument("--triple", required=True, help="Tool name prefix, e.g. x86_64-apple-darwin20.4.")
    ap.add_argument("--rcodesign", required=True, help="rcodesign binary (ad-hoc signing).")
    args = ap.parse_args()
    otool = os.path.join(args.osxcross_bin, f"{args.triple}-otool")
    install_name_tool = os.path.join(args.osxcross_bin, f"{args.triple}-install_name_tool")
    for p in (otool, install_name_tool, args.rcodesign):
        if not os.path.isfile(p):
            sys.stderr.write(f"[macdeployqt_linux] missing tool: {p}\n")
            return 1
    if not os.path.isdir(os.path.join(args.app, "Contents", "MacOS")):
        sys.stderr.write(f"[macdeployqt_linux] not an app bundle: {args.app}\n")
        return 1
    if not os.path.isdir(args.qt):
        sys.stderr.write(f"[macdeployqt_linux] Qt prefix missing: {args.qt}\n")
        return 1
    copy_plugins(args.app, args.qt)
    deploy(args.app, args.qt, otool, install_name_tool)
    write_qt_conf(args.app)
    run([args.rcodesign, "sign", args.app])
    problems = verify(args.app, otool)
    for p in problems:
        sys.stderr.write(f"[macdeployqt_linux] {p}\n")
    if problems:
        sys.stderr.write(f"[macdeployqt_linux] FAIL: {len(problems)} problem(s) in {args.app}\n")
        return 1
    print(f"[macdeployqt_linux] OK: {args.app}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
