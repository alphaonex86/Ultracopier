#!/usr/bin/env python3
r"""The Dolphin patch's OWN paste function, executed against a real Ultracopier.

`file-manager/dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch` replaces
`DolphinView::pasteToUrl()`: Ctrl+V goes to Ultracopier over the catchcopy socket, and -- since the
protocol-refusal fix -- falls back to Dolphin's own copier when Ultracopier REFUSES the job. That
function is the actual product of the patch and nothing here ever compiled it, let alone ran it.

This case does both, without a Dolphin checkout:

  * the patched `pasteToUrl()` (plus its two helpers) is taken VERBATIM out of the .patch file --
    the patch stays the single source of truth, so the test cannot drift from what ships;
  * it is compiled against `test/fmshim/dolphin_api_stub.h`, a PINNED copy of the Dolphin/KIO API
    it uses (Dolphin **26.04.x** + KF6 KIO). Pinned on purpose: if Dolphin renames one of those
    members the case fails to build, which is the signal that the patch needs revisiting;
  * everything the function reaches out to goes through a pure-virtual seam
    (`DolphinTest::PasteEnvironment`) that the driver OVERRIDES -- the project's virtual/override
    test-seam idiom. The override IS the "file manager's own copier": it really copies the local
    sources, so the fallback is verified down to the bytes on disk.

The two questions asked, both against a live Ultracopier:

  1. NORMAL case -- a local paste: Ultracopier must take the job (the internal copier must NOT run)
     and the file must really land in the destination, copied BY ULTRACOPIER.
  2. REFUSED case -- a paste whose list contains an `sftp://` url (no engine for it without KIO):
     Ultracopier must refuse, Dolphin's own copier must take over with the SAME sources and
     destination, and the local file must still land -- the paste is never lost.
  3. A cut (`mv`) that Ultracopier accepts must clear the clipboard, and a REFUSED cut must NOT --
     the fallback still needs it.

The patch hardcodes the socket name `advanced-copier-<uid>` (that name is part of what is under
test, so it is not parameterised): the run is isolated with a private `$TMPDIR`, which is where Qt
puts the socket, so it can never touch the operator's own Ultracopier."""
import sys, os, re, time, pathlib, shutil, subprocess

_CASES_DIR = str(pathlib.Path(__file__).resolve().parent)
sys.path[:] = [p for p in sys.path if p not in ("", _CASES_DIR)]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from lib import harness as H
from lib import casekit as K
from lib import uclaunch as U

_TEST_DIR = pathlib.Path(__file__).resolve().parents[1]
_FMSHIM = _TEST_DIR / "fmshim"
_PATCH = (_TEST_DIR.parents[3] / "file-manager"
          / "dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch")
# One fixed private TMPDIR for this lane: the socket name is the shipping one, so the directory is
# what keeps the run away from the operator's session -- and a stray from a previous run is found
# again here instead of lingering unnoticed.
_LANE_TMP = pathlib.Path("/tmp") / f"uc-dolphin-paste-lane-{os.getuid()}"

_PATCH_INCLUDES = """/* GENERATED -- do not edit. The body below is copied verbatim from
 * file-manager/dolphin-0002-add-ultracopier-advanced-copier-support-to-paste.patch by
 * test/cases/dolphin_paste_patch.py. Only these includes are added: in Dolphin they are already
 * in dolphinview.cpp (the patch adds QDataStream/QLocalSocket/unistd.h to that list). */
#include "dolphin_api_stub.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDataStream>
#include <QLocalSocket>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QUrl>
#include <unistd.h>

"""

_HELPERS = re.compile(r'^static (?:void|bool) '
                      r'(?:sendRawOrderList|ultracopierRefusedOrder)\(.*?^\}$', re.S | re.M)
_PASTE_START = re.compile(r'^void DolphinView::pasteToUrl\(const QUrl &url\)$', re.M)
# The patch's last hunk STOPS inside pasteToUrl() (its final context line is
# `m_markFirstNewlySelectedItemAsCurrent = true;`), so the tail of the function is Dolphin's own
# code and simply is not in the .patch. It is pinned here for the same Dolphin version as the API
# stub -- if Dolphin changes these two connects, this case stops compiling, which is the point.
_PASTE_TAIL_DOLPHIN_26_04 = """
    connect(job, &KIO::PasteJob::itemCreated, this, &DolphinView::slotItemCreated);
    connect(job, &KJob::result, this, &DolphinView::slotPasteJobResult);
}
"""


def _new_side(patch_text: str) -> str:
    """The post-patch content of every hunk (context + added lines) = what Dolphin compiles."""
    out, in_hunk = [], False
    for line in patch_text.split("\n"):
        if line.startswith("@@"):
            in_hunk = True
            continue
        if line.startswith("diff ") or line.startswith("--- ") or line.startswith("+++ "):
            in_hunk = False
            continue
        if not in_hunk:
            continue
        if line.startswith("+"):
            out.append(line[1:])
        elif line.startswith(" ") or line == "":
            out.append(line[1:] if line else "")
    return "\n".join(out)


def _extract(build: pathlib.Path) -> str:
    text = _new_side(_PATCH.read_text())
    helpers = _HELPERS.findall(text)
    m = _PASTE_START.search(text)
    if len(helpers) != 2 or m is None:
        raise RuntimeError(f"could not extract the patched code "
                           f"(helpers={len(helpers)}, pasteToUrl={m is not None})")
    body = text[m.start():].rstrip()
    if "KIO::paste(" not in body:
        raise RuntimeError("the extracted pasteToUrl() does not reach the KIO fallback")
    gen = build / "dolphin_paste_patched.cpp"
    gen.write_text(_PATCH_INCLUDES + "\n\n".join(helpers) + "\n\n"
                   + body + _PASTE_TAIL_DOLPHIN_26_04)
    return str(gen)


def _build(build: pathlib.Path) -> str:
    build.mkdir(parents=True, exist_ok=True)
    generated = _extract(build)
    qmake = shutil.which("qmake6") or shutil.which("qmake")
    if qmake is None:
        return ""
    r = subprocess.run([qmake, "-o", "Makefile", str(_FMSHIM / "dolphin_paste.pro"),
                        "-spec", "linux-g++", "CONFIG+=release", f"SOURCES+={generated}"],
                       cwd=build, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"qmake failed:\n{r.stdout[-1500:]}\n{r.stderr[-1500:]}")
    r = subprocess.run(["make", f"-j{os.cpu_count()}"], cwd=build, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"build of the patched paste failed:\n{r.stdout[-2500:]}\n{r.stderr[-2500:]}")
    return str(build / "dolphin_paste_driver")


def _reap_lane():
    """Kill any ultracopier left over from this lane (matched by its private TMPDIR in the
    environ -- these instances carry the SHIPPING socket name, so they must never be matched by
    process name alone)."""
    marker = ("TMPDIR=" + str(_LANE_TMP)).encode()
    for entry in pathlib.Path("/proc").iterdir():
        if entry.name.isdigit():
            try:
                with open(entry / "environ", "rb") as f:
                    if marker not in f.read():
                        continue
                with open(entry / "cmdline", "rb") as f:
                    argv = [a for a in f.read().split(b"\0") if a]
                if any(os.path.basename(a) == b"ultracopier" for a in argv):
                    os.kill(int(entry.name), 9)
            except OSError:
                pass


def _paste(driver: str, dest: pathlib.Path, mode: str, sources) -> dict:
    """Run the patched pasteToUrl() once; returns what it did."""
    env = dict(os.environ, TMPDIR=str(_LANE_TMP), QT_QPA_PLATFORM="offscreen", DISPLAY="")
    env.pop("WAYLAND_DISPLAY", None)
    r = subprocess.run([driver, str(dest), mode] + list(sources), env=env,
                       capture_output=True, text=True, timeout=180)
    out = r.stdout
    return {
        "internal": "INTERNAL_COPY " in out,
        "internal_sources": int(re.search(r"INTERNAL_COPY (\d+)", out).group(1))
                            if "INTERNAL_COPY " in out else 0,
        "ultracopier": "ULTRACOPIER" in out,
        "clipboard_cleared": "CLIPBOARD cleared" in out,
        "raw": out.strip().replace("\n", " | "),
        "stderr": r.stderr[-400:],
    }


def run(backends=None, memcheck=H.NONE) -> bool:
    if memcheck == H.TSAN:
        print("    [dolphin_paste_patch] SKIP under TSan (the full app dies in Qt's threadpool "
              "under TSan; the thread gate is cases/tsan_engine_api.py)")
        return True
    bes = [b for b in (backends or [H.ASYNC]) if b in (H.ASYNC, H.IO_URING)]
    if not bes:
        print("    [dolphin_paste_patch] SKIP (async/io_uring only)")
        return True
    backend = H.ASYNC if H.ASYNC in bes else bes[0]

    rows = []
    root = pathlib.Path(K.fresh_dest("dolphin_paste"))
    build = pathlib.Path(H.SOURCES) / "build" / "test-dolphin-paste"
    _reap_lane()
    try:
        driver = _build(build)
        if not driver:
            print("    [dolphin_paste_patch] SKIP (no qmake to build the patched paste)")
            return True

        src_dir = root / "src"
        src_dir.mkdir(parents=True, exist_ok=True)
        payload = b"dolphin-paste-payload\n"

        with U.Instance("dolphin", backend=backend, memcheck=memcheck,
                        socket_suffix=False, tmpdir=_LANE_TMP) as inst:
            if not inst.wait_listening("catchcopy", 60):
                print("    [dolphin_paste_patch] FAIL: Ultracopier never listened on the "
                      "shipping catchcopy socket")
                return False

            # 1. NORMAL: a local paste -- Ultracopier takes it, the internal copier must not run
            f_ok = src_dir / "accepted.txt"
            f_ok.write_bytes(payload)
            d_ok = root / "dest_accepted"
            d_ok.mkdir(exist_ok=True)
            res = _paste(driver, d_ok, "cp", [f"file://{f_ok}"])
            rows.append(("normal paste: Ultracopier takes the job (no internal copy)",
                         res["ultracopier"] and not res["internal"], res["raw"]))
            landed = inst.wait_until(lambda: (d_ok / "accepted.txt").exists(), 60)
            rows.append(("normal paste: Ultracopier really copied the file",
                         landed and (d_ok / "accepted.txt").read_bytes() == payload
                         if landed else False,
                         f"landed={landed} dest={sorted(os.listdir(d_ok))}"))

            # 2. REFUSED: an sftp url in the list -- Dolphin's own copier must take over and the
            #    local file must still be copied (the paste is never lost)
            f_local = src_dir / "fallback.txt"
            f_local.write_bytes(payload)
            d_ref = root / "dest_refused"
            d_ref.mkdir(exist_ok=True)
            res = _paste(driver, d_ref, "cp",
                         [f"file://{f_local}", "sftp://root@127.0.0.1/XXX"])
            rows.append(("refused paste: the file manager's own copier takes over",
                         res["internal"] and not res["ultracopier"], res["raw"]))
            rows.append(("refused paste: it gets the WHOLE source list (local + sftp)",
                         res["internal_sources"] == 2, f"sources={res['internal_sources']}"))
            got = (d_ref / "fallback.txt")
            rows.append(("refused paste: the internal copy really wrote the file",
                         got.exists() and got.read_bytes() == payload,
                         f"dest={sorted(os.listdir(d_ref))}"))

            # 3. a CUT: the clipboard is cleared only when Ultracopier accepted
            f_cut = src_dir / "cut.txt"
            f_cut.write_bytes(payload)
            d_cut = root / "dest_cut"
            d_cut.mkdir(exist_ok=True)
            res = _paste(driver, d_cut, "cut", [f"file://{f_cut}"])
            rows.append(("accepted cut: the clipboard is cleared",
                         res["ultracopier"] and res["clipboard_cleared"], res["raw"]))
            inst.wait_until(lambda: (d_cut / "cut.txt").exists(), 60)

            f_cut2 = src_dir / "cut_refused.txt"
            f_cut2.write_bytes(payload)
            d_cut2 = root / "dest_cut_refused"
            d_cut2.mkdir(exist_ok=True)
            res = _paste(driver, d_cut2, "cut",
                         [f"file://{f_cut2}", "sftp://root@127.0.0.1/XXX"])
            rows.append(("refused cut: the clipboard is KEPT (the fallback still needs it)",
                         res["internal"] and not res["clipboard_cleared"], res["raw"]))

            rows.append(("Ultracopier survived every paste", inst.alive(), ""))
    except Exception as e:
        print(f"    [dolphin_paste_patch] EXCEPTION: {e}")
        return False
    finally:
        _reap_lane()
        H._kill_all_ultracopier()
        shutil.rmtree(root, ignore_errors=True)

    ok = all(r[1] for r in rows)
    for name, good, detail in rows:
        print(f"      [{'PASS' if good else 'FAIL'}] {name}{('  -- ' + detail) if detail else ''}")
    print(f"    [dolphin_paste_patch] {'PASS' if ok else 'FAIL'}  ({len(rows)} checks on {backend})")
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
