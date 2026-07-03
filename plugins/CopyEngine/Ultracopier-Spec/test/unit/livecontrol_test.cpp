// In-process driver of the REAL CopyEngine exercising the LIVE-CONTROL surface (the theme's
// transfer-list buttons) on a RUNNING copy: skip(id) of the currently-transferring item,
// removeItems([id]) of a still-queued item, and moveItemsOnTop([id]) reordering a queued item --
// the exact calls PluginInterface_CopyEngine exposes and the Oxygen theme wires to its buttons.
//
// This is the use-after-free / dangling-running-item / pump-never-repumps SAFETY class (coverage
// gap 20): skip(id) on a running entry must cancel its in-flight reader/writer and tombstone an
// ActionToDoTransfer a transfer thread still references, without a crash, a hang, or losing an
// untouched sibling. I/O is slowed by the LD_PRELOAD shim (UC_FS_SCENARIO=slow:<ms>) so the first
// file is still in flight when we act; item IDs are learned from the engine's own newActionOnList
// signal (AddingItem), so we act on REAL ids, not guessed ones. Async backend only (libc I/O).
//
// argv: <src> <dest>       (src must hold several files, sorted names 00..NN)
// exit 0 == completed/canDelete with no crash; the harness case adds the ASan/TSan + content gate.
#include "CopyEngine.h"
#include "../../../interface/FacilityInterface.h"
#include "../../../interface/PluginInterface_CopyEngine.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>

static uintmax_t treeBytes(const std::string &root) {
    uintmax_t t = 0; std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
        if (it->is_regular_file(ec)) { auto s = it->file_size(ec); if (!ec) t += s; }
    return t;
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
    e.setFileCollision(2); e.setFolderCollision(1);
    e.setFileError(1); e.setFolderError(1);
    e.setInodeThreads(16); e.setParallelizeIfSmallerThan(128);
    e.setCoalesceSourceStat(true); e.setRightTransfer(false); e.setKeepDate(false);
    e.setMkFullPath(true); e.setCheckDestinationFolderExists(false); e.setMoveTheWholeFolder(false);
    e.setFollowTheStrictOrder(false); e.setDeletePartiallyTransferredFiles(false);
    e.setRenameTheOriginalDestination(false); e.setChecksum(false); e.setCheckDiskSpace(false);
    e.setBuffer(false); e.set_setFilters({}, {}, {}, {}); e.setRenamingRules("", "");
}

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);   // unbuffered so a hang cannot hide the trace
    QApplication app(argc, argv);
    if (argc < 3) { std::fprintf(stderr, "usage: <src> <dest>\n"); return 2; }
    const std::string src = argv[1], dest = argv[2];
    const std::string mode = (argc > 3) ? argv[3] : "all";   // all|skip|remove|move

    StubFacility facility;
    CopyEngine engine(&facility);
    engine.connectTheSignalsSlots();
    configure(engine);

    bool sawActive = false, done = false, canDelete = false;
    std::vector<uint64_t> ids;   // AddingItem ids in arrival (== scan/transfer) order
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

    const uintmax_t srcTotal = treeBytes(src);
    engine.newCopy(std::vector<std::string>{src}, dest);
    std::printf("STARTED newCopy srcTotal=%llu\n", (unsigned long long)srcTotal);

    QElapsedTimer t; t.start();
    bool acted = false; long long lastBeat = -1;
    while (!done && !canDelete && t.elapsed() < 120000) {
        app.processEvents(QEventLoop::AllEvents, 20);
        if (t.elapsed() / 1000 != lastBeat) { lastBeat = t.elapsed() / 1000;
            if (lastBeat % 2 == 0) std::printf("beat t=%llds sawActive=%d ids=%zu destB=%llu acted=%d\n",
                (long long)lastBeat, sawActive?1:0, ids.size(), (unsigned long long)treeBytes(dest), acted?1:0); }
        // Act ONCE, mid-flight: first file transferring (sawActive + some bytes) and we know >=4 ids.
        if (!acted && sawActive && ids.size() >= 4 && treeBytes(dest) > 0) {
            const uint64_t running = ids.front();        // first added == first transferred
            const uint64_t queuedRemove = ids.back();    // last == still queued
            const uint64_t queuedReorder = ids[ids.size() / 2];
            std::printf("ACT ids=%zu skip(running=%llu) remove(queued=%llu) moveTop(queued=%llu)\n",
                        ids.size(), (unsigned long long)running,
                        (unsigned long long)queuedRemove, (unsigned long long)queuedReorder);
            const bool doMove   = (mode == "all" || mode == "move"   || mode == "removemove");
            const bool doRemove = (mode == "all" || mode == "remove" || mode == "removemove");
            const bool doSkip   = (mode == "all" || mode == "skip");
            if (doMove)   engine.moveItemsOnTop(std::vector<uint64_t>{queuedReorder});
            if (doRemove) engine.removeItems(std::vector<uint64_t>{queuedRemove});
            if (doSkip)   engine.skip(running);   // KNOWN HANG on a running item (finding-skip-running-hangs)
            acted = true;
        }
    }
    std::printf("DONE done=%d canDelete=%d acted=%d destBytes=%llu/%llu ids=%zu\n",
                done ? 1 : 0, canDelete ? 1 : 0, acted ? 1 : 0,
                (unsigned long long)treeBytes(dest), (unsigned long long)srcTotal, ids.size());
    // Success == the engine reached a clean terminal state (no hang, no crash) AFTER we mutated the
    // running list. If we never got enough ids to act, that is a setup problem -> non-zero.
    return ((done || canDelete) && acted) ? 0 : 1;
}
