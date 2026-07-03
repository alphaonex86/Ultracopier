// In-process reproducer for the #9 realMove data-loss (adversarial-review finding, 2026-07-02):
// a SAME-DRIVE MOVE whose atomic rename() FAILS (non-EXDEV, e.g. a stuck mount) while an interactive
// SKIP is in flight must leave the user's PRE-EXISTING destination UNTOUCHED -- a failed rename made
// NO partial of ours, so the engine must never unlink the destination.
//
// Rig: fileCollision=Overwrite onto a dest file pre-seeded with distinct user bytes; the LD_PRELOAD
// shim's `renamefail:<dstname>` blocks rename() ~250ms then returns EACCES, giving this driver a window
// to engine.skip(runningId) mid-rename. After the job settles, the destination MUST still hold the
// pre-seed bytes (the move failed + was skipped -> the source stays, the dest is untouched).
//
// argv: <src_file> <dest_dir>     (src_file is moved INTO dest_dir; dest_dir/<basename> is pre-seeded)
// exit 0 == destination survived with the pre-seed bytes (fix present); 1 == destination lost/altered.
#include "CopyEngine.h"
#include "../../../interface/FacilityInterface.h"
#include "../../../interface/PluginInterface_CopyEngine.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

static std::string readAll(const std::string &p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

class StubFacility : public FacilityInterface {
public:
    void retranslate() override {}
    std::string sizeToString(const double &) const override { return std::string(); }
    std::string sizeUnitToString(const Ultracopier::SizeUnit &) const override { return std::string(); }
    std::string translateText(const std::string &t) const override { return t; }
    std::string speedToString(const double &) const override { return std::string(); }
    Ultracopier::TimeDecomposition secondsToTimeDecomposition(const uint32_t &) const override { return Ultracopier::TimeDecomposition(); }
    bool haveFunctionality(const std::string &) const override { return false; }
    std::string callFunctionality(const std::string &, const std::vector<std::string> &) override { return std::string(); }
    std::string simplifiedRemainingTime(const uint32_t &) const override { return std::string(); }
    std::string ultimateUrl() const override { return std::string(); }
    std::string softwareName() const override { return std::string("test"); }
    bool isUltimate() const override { return false; }
    bool isIllegal() const override { return false; }
    void *prepareOpusAudio(const std::string &, QBuffer &) const override { return nullptr; }
};

static void configure(CopyEngine &e) {
    e.setAutoStart(true);
    e.setFileCollision(2); e.setFolderCollision(1);   // 2 = Overwrite
    e.setFileError(1); e.setFolderError(1);
    e.setInodeThreads(16); e.setParallelizeIfSmallerThan(128);
    e.setCoalesceSourceStat(true); e.setRightTransfer(false); e.setKeepDate(false);
    e.setMkFullPath(true); e.setCheckDestinationFolderExists(false); e.setMoveTheWholeFolder(false);
    e.setFollowTheStrictOrder(false); e.setDeletePartiallyTransferredFiles(false);
    e.setRenameTheOriginalDestination(false); e.setChecksum(false); e.setCheckDiskSpace(false);
    e.setBuffer(false); e.set_setFilters({}, {}, {}, {}); e.setRenamingRules("", "");
}

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    QApplication app(argc, argv);
    if (argc < 3) { std::fprintf(stderr, "usage: <src_file> <dest_dir>\n"); return 2; }
    const std::string srcFile = argv[1], destDir = argv[2];
    const std::string base = std::filesystem::path(srcFile).filename().string();
    const std::string destFile = (std::filesystem::path(destDir) / base).string();
    const std::string preseed = readAll(destFile);   // the user's pre-existing dest content
    if (preseed.empty()) { std::fprintf(stderr, "dest not pre-seeded: %s\n", destFile.c_str()); return 2; }

    StubFacility facility;
    CopyEngine engine(&facility);
    engine.connectTheSignalsSlots();
    configure(engine);

    bool sawActive = false, done = false, canDelete = false;
    std::vector<uint64_t> ids;
    QObject::connect(&engine, &PluginInterface_CopyEngine::actionInProgess,
        [&](const Ultracopier::EngineActionInProgress &a) {
            if (a != Ultracopier::Idle) sawActive = true;
            if (a == Ultracopier::Idle && sawActive) done = true;
        });
    QObject::connect(&engine, &PluginInterface_CopyEngine::canBeDeleted, [&]() { canDelete = true; });
    QObject::connect(&engine, &PluginInterface_CopyEngine::newActionOnList,
        [&](const std::vector<Ultracopier::ReturnActionOnCopyList> &acts) {
            for (const auto &a : acts)
                if (a.type == Ultracopier::AddingItem) ids.push_back(a.addAction.id);
        });

    engine.newMove(std::vector<std::string>{srcFile}, destDir);
    std::printf("STARTED newMove %s -> %s\n", srcFile.c_str(), destDir.c_str());

    QElapsedTimer t; t.start();
    bool acted = false;
    qint64 activeSince = -1;
    // CRITICAL timing: the skip's realMove branch signals completion (Idle) IMMEDIATELY, but the
    // transfer thread is STILL blocked in the 800ms rename() and only does its site-C teardown (the
    // buggy unlink) when rename RETURNS. So we must NOT read the destination the moment the engine
    // says "done" -- we must keep pumping events until the transfer thread has definitely finished its
    // teardown (well past the 800ms block). Run until >=3000ms AND terminal, so the unlink (if any)
    // has already happened before we check.
    while (t.elapsed() < 3000 || (!done && !canDelete && t.elapsed() < 60000)) {
        app.processEvents(QEventLoop::AllEvents, 10);
        if (activeSince < 0 && sawActive && !ids.empty())
            activeSince = t.elapsed();
        // Skip ~300ms AFTER the job goes active, so the transfer thread has entered the (800ms-blocked)
        // rename() and transfer_stat==Transfer -- the exact state that reaches the site-C teardown. An
        // immediate skip lands pre-rename and aborts the file cleanly (never exercising site C).
        if (!acted && activeSince >= 0 && t.elapsed() - activeSince >= 300) {
            const uint64_t running = ids.front();
            std::printf("ACT skip(running=%llu) at t=%lldms\n", (unsigned long long)running, (long long)t.elapsed());
            engine.skip(running);
            acted = true;
        }
    }

    const std::string after = readAll(destFile);
    const bool destPresent = std::filesystem::exists(destFile);
    const bool survived = destPresent && (after == preseed);
    std::printf("DONE done=%d canDelete=%d acted=%d destPresent=%d survived=%d (%zu->%zu bytes)\n",
                done ? 1 : 0, canDelete ? 1 : 0, acted ? 1 : 0, destPresent ? 1 : 0,
                survived ? 1 : 0, preseed.size(), after.size());
    if (!acted) { std::fprintf(stderr, "never caught the move in flight to skip it\n"); return 2; }
    return survived ? 0 : 1;
}
