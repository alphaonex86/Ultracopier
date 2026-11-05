#!/usr/bin/env python3
"""Pin tools/to-pack/sub-script/macdeployqt_linux.py, the Linux-native macdeployqt of the macOS build.

3.1.0.7 shipped a .app that dyld aborted at launch on every Mac:
  "Library not loaded: @rpath/QtDBus.framework/Versions/A/QtDBus, Referenced from: QtGui"
The deploy only walked the exe's direct dependencies (the copied framework binary is a symlink, so
the recursion was skipped), and QtDBus is only a dependency of QtGui. Same script: the platform
plugins were pointed at @executable_path/../../Frameworks (one level too high), the build-host
Qt path stayed in the exe's LC_RPATH, and install_name_tool left every arm64 signature invalid
(Apple Silicon kills such a binary at launch) with no re-signing.

A tiny universal exe linking only QtGui + QtCore (exactly that transitive shape) is built with
osxcross and deployed; the bundle is then checked independently of the script's own verify():
QtDBus bundled, every @executable_path dependency present, no build-host path left anywhere,
every Mach-O signature valid. The signature check is the script's own page-hash checker
(rcodesign's verify rejects valid ad-hoc signatures, its own help calls it buggy); it is first shown
to flag an arm64 exe that install_name_tool modified without re-signing.
A second exe linked against a dylib that is not in the bundle must make the deploy FAIL.

Backend-independent; self-skips (PASS) when the osxcross prefix is absent
(UC_OSXCROSS_PREFIX, default /mnt/data/perso/progs/catchchallenger-osxcross).
UC_MACDEPLOY_SCRIPT overrides the script under test."""
import sys, os, pathlib, subprocess, tempfile, shutil, importlib.util, plistlib

_HERE = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _HERE)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H   # noqa: F401  (signature symmetry with the other cases)

_CHECKER = pathlib.Path(__file__).resolve().parents[5] / "tools" / "to-pack" / "sub-script" / "macdeployqt_linux.py"
_SCRIPT = os.environ.get("UC_MACDEPLOY_SCRIPT", str(_CHECKER))
_PREFIX = os.environ.get("UC_OSXCROSS_PREFIX", "/mnt/data/perso/progs/catchchallenger-osxcross")
_BIN = os.path.join(_PREFIX, "osxcross", "target", "bin")
_QT = os.path.join(_PREFIX, "qt6-macos", "6.5.3", "macos")
_RCODESIGN = os.path.join(_PREFIX, "bin", "rcodesign")
_TRIPLE = "x86_64-apple-darwin20.4"
_ARCHS = ("x86_64", "arm64")


def _load_checker():
    spec = importlib.util.spec_from_file_location("macdeployqt_linux", _CHECKER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _tool(name, arch="x86_64"):
    return os.path.join(_BIN, f"{arch}-apple-darwin20.4-{name}")


def _cc(arch, args):
    r = subprocess.run([_tool("clang", arch), "-fuse-ld=lld", "-mmacosx-version-min=11.0",
                        "-Wl,-headerpad_max_install_names"] + args, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"    [mac_bundle_deploy] {arch} clang FAILED:\n{r.stderr[-2000:]}")
    return r.returncode == 0


def _make_app(work, name, extra_libs):
    """Universal <name>.app whose exe links QtGui + QtCore (+ extra_libs), CMake-like rpath."""
    main_c = work / "main.c"
    main_c.write_text("int main(void){return 0;}\n")
    slices = []
    for arch in _ARCHS:
        out = work / f"{name}-{arch}"
        if not _cc(arch, [str(main_c), "-o", str(out), "-F" + _QT + "/lib", "-framework", "QtGui",
                          "-framework", "QtCore", "-Wl,-rpath," + _QT + "/lib"] + extra_libs(arch)):
            return None
        slices.append(str(out))
    app = work / f"{name}.app"
    (app / "Contents" / "MacOS").mkdir(parents=True)
    with open(app / "Contents" / "Info.plist", "wb") as f:
        plistlib.dump({"CFBundleExecutable": name, "CFBundleIdentifier": f"test.ultracopier.{name}",
                       "CFBundlePackageType": "APPL"}, f)
    r = subprocess.run([_tool("lipo"), "-create"] + slices + ["-output", str(app / "Contents" / "MacOS" / name)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(f"    [mac_bundle_deploy] lipo FAILED:\n{r.stderr}")
        return None
    return app


def _deploy(app):
    return subprocess.run([sys.executable, "-I", _SCRIPT, "--app", str(app), "--qt", _QT,
                           "--osxcross-bin", _BIN, "--triple", _TRIPLE, "--rcodesign", _RCODESIGN],
                          capture_output=True, text=True, timeout=600)


def _otool(flag, path):
    return subprocess.run([_tool("otool"), flag, str(path)], capture_output=True, text=True, check=True).stdout


def _bundle_problems(app, checker):
    exe_dir = app / "Contents" / "MacOS"
    problems = []
    if not (app / "Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus").is_file():
        problems.append("QtDBus.framework (transitive dependency of QtGui) not bundled")
    if not (app / "Contents/PlugIns/platforms/libqcocoa.dylib").is_file():
        problems.append("platforms/libqcocoa.dylib not bundled")
    for p in app.rglob("*"):
        if p.is_file() and not p.is_symlink() and p.read_bytes()[:4] in (
                b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe"):
            problems += checker.signature_problems(str(p))
            if _PREFIX in _otool("-l", p):
                problems.append(f"{p}: build-host path {_PREFIX} still in the load commands")
            for line in _otool("-L", p).splitlines():
                dep = line.strip().split(" ")[0]
                if dep.startswith("@executable_path/") and not (exe_dir / dep[len("@executable_path/"):]).is_file():
                    problems.append(f"{p}: {dep} missing from the bundle")
                if dep.startswith("@rpath/Qt"):
                    problems.append(f"{p}: {dep} left as @rpath")
    return list(dict.fromkeys(problems))   # a fat binary lists every dependency once per slice


def run(backends=None, memcheck=H.NONE) -> bool:
    if not (os.path.isfile(_tool("clang")) and os.path.isfile(_tool("clang", "arm64")) and os.path.isdir(_QT) and os.path.isfile(_RCODESIGN)):
        print(f"    [mac_bundle_deploy] SKIP (osxcross + Qt for macOS + rcodesign not found under {_PREFIX})")
        return True
    work = pathlib.Path(tempfile.mkdtemp(prefix="uc-mac-deploy-"))
    try:
        ok = True
        checker = _load_checker()
        app = _make_app(work, "Good", lambda arch: [])
        if app is None:
            return False
        unsigned = work / "unsigned-after-install_name_tool"
        shutil.copy(app / "Contents" / "MacOS" / "Good", unsigned)
        subprocess.run([_tool("install_name_tool"), "-change", "@rpath/QtCore.framework/Versions/A/QtCore",
                        "@executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore", str(unsigned)],
                       capture_output=True, check=True)
        if not checker.signature_problems(str(unsigned)):
            print("    [mac_bundle_deploy] FAIL: signature checker accepted an arm64 slice modified after signing")
            ok = False
        r = _deploy(app)
        if r.returncode != 0:
            print(f"    [mac_bundle_deploy] deploy of a valid app FAILED (exit {r.returncode}):\n{r.stderr[-3000:]}")
            ok = False
        for p in _bundle_problems(app, checker):
            print(f"    [mac_bundle_deploy] FAIL: {p}")
            ok = False

        missing_dir = work / "missing"
        missing_dir.mkdir()
        missing_c = missing_dir / "missing.c"
        missing_c.write_text("int uc_missing(void){return 1;}\n")

        def missing_lib(arch):
            out = missing_dir / f"libmissing-{arch}.dylib"
            _cc(arch, ["-dynamiclib", str(missing_c), "-o", str(out),
                       "-install_name", "/opt/uc-missing/libmissing.dylib"])
            return [str(out)]
        bad = _make_app(work, "Bad", missing_lib)
        if bad is None:
            return False
        r = _deploy(bad)
        if r.returncode == 0:
            print("    [mac_bundle_deploy] FAIL: deploy accepted an exe whose /opt/uc-missing/libmissing.dylib "
                  "is not in the bundle (would abort at launch on the user's Mac)")
            ok = False
        elif "libmissing.dylib" not in r.stderr:
            print(f"    [mac_bundle_deploy] FAIL: deploy failed without naming the missing dylib:\n{r.stderr[-2000:]}")
            ok = False
        return ok
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
