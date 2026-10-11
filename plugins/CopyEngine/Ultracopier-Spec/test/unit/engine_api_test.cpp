// In-process driver of the REAL CopyEngine (PluginInterface_CopyEngine) -- the running-transfer
// API test. It instantiates the engine, configures it via the same setXxx() calls the factory
// uses (NO OptionInterface needed), runs a real copy through a live Qt event loop, and drives the
// API on the running transfer:
//   * plain copy  -> completes (actionInProgess Idle after active) and content is byte-correct.
//   * pause/resume -> realByteTransfered() HALTS while paused, then completes correctly on resume.
//   * puttoend     -> ONE inode thread + fileError=put-to-end: a file whose first read fails (shim
//                    flaky:<name>:1) is deferred to the end and copied on its one retry; the job
//                    reaches Idle with every file byte-identical. This is the put-to-end close
//                    handshake (WriteThread::stop(false) -> closed() -> putAtBottomAfterError) on the
//                    single thread that also copies everything else -- under TSan it is the only
//                    stable way to watch that handshake (the full app dies on the Qt registry CHECK).
//   * cancel       -> cancel() mid-transfer reaches canBeDeleted() (every transfer thread went Idle)
//                    and the partial destination is removed. A 3.1 regression left the engine
//                    waiting forever (leaked engine + threads, partial file kept).
//   * speedlimit   -> ONE inode thread, setSpeedLimitation(512 KB/s) for the whole copy: the copy of
//                    a known byte count must take at least the time the limit implies (a lower bound,
//                    so load can only make it pass more easily). The pipelined backends ignored the
//                    speed clock entirely (the UI offered a limit that did nothing).
//   * speedchange  -> ONE inode thread, a speed limit set BEFORE the copy (the limiter shrinks the
//                    transfer block below the option's blockSize) and REMOVED once the first bytes
//                    have landed: the transfer threads' blockSize grows while their I/O buffers are
//                    still the small ones. The pipelined backends must size every read to the
//                    buffer actually allocated (never the live blockSize) or the kernel writes past
//                    the heap block -> heap corruption / wrong bytes. Asserts a clean Idle + the
//                    caller diffs the content.
//
// I/O is slowed by the LD_PRELOAD shim (UC_FS_SCENARIO=slow:<ms>) so the copy is long enough to
// catch it mid-flight and pause it. Async backend only (libc I/O; io_uring bypasses libc).
//
// argv: <src> <dest> [pause|cancel|puttoend|speedchange|speedlimit]
#include "CopyEngine.h"
#include "../../../interface/FacilityInterface.h"
#include "../../../interface/PluginInterface_CopyEngine.h"
#include <QApplication>   // CopyEngine builds QWidgets (options + collision dialogs) -> needs QApplication
#include <QElapsedTimer>
#include <QEventLoop>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>

// Total bytes currently present under a directory tree (what has actually been written to dest).
static uintmax_t treeBytes(const std::string &root) {
    uintmax_t t = 0;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file(ec)) { auto s = it->file_size(ec); if (!ec) t += s; }
    }
    return t;
}
static void pumpFor(QApplication &app, int ms) {
    QElapsedTimer t; t.start();
    while (t.elapsed() < ms) app.processEvents(QEventLoop::AllEvents, 20);
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

static void configure(CopyEngine &e, int inodeThreads) {
    e.setAutoStart(true);
    e.setFileCollision(2);    // Overwrite
    e.setFolderCollision(1);  // Merge
    e.setFileError(1);        // Skip
    e.setFolderError(1);      // Skip
    e.setInodeThreads(inodeThreads);
    e.setParallelizeIfSmallerThan(128);
    e.setCoalesceSourceStat(true);
    e.setRightTransfer(false);
    e.setKeepDate(false);
    e.setMkFullPath(true);
    e.setCheckDestinationFolderExists(false);
    e.setMoveTheWholeFolder(false);
    e.setFollowTheStrictOrder(false);
    e.setDeletePartiallyTransferredFiles(std::getenv("UC_TEST_KEEP_PARTIALS") == nullptr);   // shipping default: delete
    e.setRenameTheOriginalDestination(false);
    e.setChecksum(false);
    e.setCheckDiskSpace(false);
    e.setBuffer(false);
    e.set_setFilters({}, {}, {}, {});
    e.setRenamingRules("", "");
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);   // run under the offscreen QPA (set via env)
    if (argc < 3) { std::fprintf(stderr, "usage: <src> <dest> [pause|cancel]\n"); return 2; }
    const std::string src = argv[1], dest = argv[2];
    const bool doPause = (argc > 3 && std::string(argv[3]) == "pause");
    const bool doCancel = (argc > 3 && std::string(argv[3]) == "cancel");
    const bool doPutToEnd = (argc > 3 && std::string(argv[3]) == "puttoend");
    const bool doSpeedChange = (argc > 3 && std::string(argv[3]) == "speedchange");
    const bool doSpeedLimit = (argc > 3 && std::string(argv[3]) == "speedlimit");

    StubFacility facility;
    CopyEngine engine(&facility);
    engine.connectTheSignalsSlots();
    // put-to-end: ONE inode thread from the start (the deferred file comes back to the same thread);
    // sizing to 16 then shrinking would tear 15 threads down right before the scan thread starts,
    // which trips libtsan's thread-registry CHECK on pthread_t reuse (Qt detached threads)
    configure(engine, (doPutToEnd || doSpeedChange || doSpeedLimit) ? 1 : 16);
    if (doPutToEnd)
        engine.setFileError(2);      // put to the end of the list (one deferred retry)
    if (doSpeedChange)
        engine.setSpeedLimitation(256);    // 256 KB/s: interval > MAXTIMERINTERVAL -> the limiter shrinks the 64 KB block to ~25 KB
    if (doSpeedLimit)
        engine.setSpeedLimitation(512);    // 512 KB/s for the whole copy (one ~51 KB block per 100 ms tick)

    bool sawActive = false, done = false, canDelete = false;
    QObject::connect(&engine, &PluginInterface_CopyEngine::actionInProgess,
        [&](const Ultracopier::EngineActionInProgress &a) {
            if (a != Ultracopier::Idle) sawActive = true;
            if (a == Ultracopier::Idle && sawActive) done = true;
        });
    QObject::connect(&engine, &PluginInterface_CopyEngine::canBeDeleted,
        [&]() { canDelete = true; });

    const uintmax_t srcTotal = treeBytes(src);
    bool isPausedSig = false;
    QObject::connect(&engine, &PluginInterface_CopyEngine::isInPause,
        [&](const bool &p) { isPausedSig = p; });

    engine.newCopy(std::vector<std::string>{src}, dest);

    QElapsedTimer t; t.start();
    bool paused = false, pauseReported = false, cancelled = false, speedLifted = false;
    qint64 cancelAt = 0;
    // after a cancel only canBeDeleted() ends the wait (Idle alone is not the engine's teardown signal)
    while (!(done && !doCancel) && !canDelete && t.elapsed() < 120000) {
        app.processEvents(QEventLoop::AllEvents, 30);
        const uintmax_t d = treeBytes(dest);
        if (doCancel && !cancelled && sawActive && d > srcTotal / 10 && d < (srcTotal * 7) / 10) {
            engine.cancel(); cancelled = true; cancelAt = t.elapsed();
            std::printf("CANCEL at destBytes=%llu/%llu\n", (unsigned long long)d, (unsigned long long)srcTotal);
        }
        // Lift the speed limit as soon as the copy is in flight: the buffers were sized under the limit.
        if (doSpeedChange && !speedLifted && sawActive && d > 0) {
            engine.setSpeedLimitation(0); speedLifted = true;
            std::printf("SPEEDLIFT at destBytes=%llu/%llu\n", (unsigned long long)d, (unsigned long long)srcTotal);
        }
        if (cancelled && t.elapsed() - cancelAt > 20000) {
            std::printf("CANCEL-TIMEOUT canBeDeleted never came in 20s\n");
            break;
        }
        // Pause mid-stream: some bytes written, but not yet all (catch it in flight).
        if (doPause && !paused && sawActive && d > srcTotal / 10 && d < (srcTotal * 7) / 10) {
            engine.pause(); paused = true;
        }
        if (paused && !pauseReported) {
            // Let the queued pause reach the worker + the in-flight pipeline/current file drain,
            // THEN check that dest progress has truly plateaued over a quiet window.
            pumpFor(app, 1500);
            const uintmax_t b1 = treeBytes(dest);
            pumpFor(app, 1500);
            const uintmax_t b2 = treeBytes(dest);
            std::printf("PAUSE isInPause=%d drained=%llu plateau=%llu halted=%d (srcTotal=%llu)\n",
                        isPausedSig ? 1 : 0, (unsigned long long)b1, (unsigned long long)b2,
                        (b1 == b2 && b2 < srcTotal) ? 1 : 0, (unsigned long long)srcTotal);
            pauseReported = true;
            engine.resume();
        }
    }
    std::printf("DONE done=%d canDelete=%d destBytes=%llu/%llu paused=%d cancelled=%d speedLifted=%d elapsedMs=%lld\n",
                done ? 1 : 0, canDelete ? 1 : 0,
                (unsigned long long)treeBytes(dest), (unsigned long long)srcTotal, paused ? 1 : 0,
                cancelled ? 1 : 0, speedLifted ? 1 : 0, (long long)t.elapsed());
    if (doCancel)
        return canDelete ? 0 : 1;
    return (done || canDelete) ? 0 : 1;
}
