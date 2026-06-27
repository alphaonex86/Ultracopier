// In-process, FS-free unit test of the UI<->copy-list RECONCILIATION + reorder logic
// (ListThread::moveItemsOnTop/Up/Down/OnBottom/removeItems in ListThreadListChange.cpp), driven
// directly on a synthetic transfer list. No real copy, no Qt event loop, no disk -> runs in ms,
// and a multi-million-entry SCALE pass proves C++ chews the whole list/reorder pipeline quickly
// (the user's point). Expected orders below are derived from the engine source, not guessed.
//
// The reorder API + actionToDoListTransfer + pathTree + rebuildActionToDoTransferIndex are all
// public on ListThread, so only a tiny FacilityInterface stub is needed (for the ctor).
#include "ListThread.h"
#include "../../../interface/FacilityInterface.h"
#include <QCoreApplication>
#include <cstdio>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

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

static int g_checks = 0, g_fail = 0;
static void check(bool cond, const std::string &msg) {
    ++g_checks;
    if (!cond) { ++g_fail; std::fprintf(stderr, "  FAIL: %s\n", msg.c_str()); }
}

// Build a fresh transfer list with ids 1..n. Distinct interned paths when `distinctPaths`
// (small correctness runs, also exercises PathTree), else one shared node (huge scale runs).
static void populate(ListThread &lt, uint64_t n, bool distinctPaths) {
    lt.actionToDoListTransfer.clear();
    uint32_t shared = distinctPaths ? 0u : lt.pathTree.intern(TransferThread::stringToInternalString("/src/shared.dat"));
    for (uint64_t i = 1; i <= n; ++i) {
        auto a = std::unique_ptr<ListThread::ActionToDoTransfer>(new ListThread::ActionToDoTransfer());
        a->id = i;
        a->size = 100;
        if (distinctPaths) {
            const std::string p = "/src/dir" + std::to_string(i % 7) + "/file" + std::to_string(i) + ".dat";
            a->srcNode = a->dstNode = lt.pathTree.intern(TransferThread::stringToInternalString(p));
        } else {
            a->srcNode = a->dstNode = shared;
        }
        a->mode = Ultracopier::Copy;
        a->isRunning = false;
        a->removed = false;
        lt.actionToDoListTransfer.push_back(std::move(a));
    }
    lt.rebuildActionToDoTransferIndex();
}

// Current live order (skipping tombstones), as a vector of ids.
static std::vector<uint64_t> order(ListThread &lt) {
    std::vector<uint64_t> v;
    v.reserve(lt.actionToDoListTransfer.size());
    for (const auto &a : lt.actionToDoListTransfer)
        if (a && !a->removed) v.push_back(a->id);
    return v;
}

static std::string fmt(const std::vector<uint64_t> &v) {
    std::string s = "[";
    for (size_t i = 0; i < v.size(); ++i) { if (i) s += ","; s += std::to_string(v[i]); }
    return s + "]";
}

static void expect_order(ListThread &lt, const std::vector<uint64_t> &want, const std::string &label) {
    const std::vector<uint64_t> got = order(lt);
    check(got == want, label + ": got " + fmt(got) + " want " + fmt(want));
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);   // QObject machinery only; never exec()'d
    StubFacility facility;

    // ---- correctness: each reorder op produces the exact order the engine source defines ----
    {
        ListThread lt(&facility);
        populate(lt, 6, /*distinctPaths*/true);
        expect_order(lt, {1,2,3,4,5,6}, "initial");
        // resolve() round-trips a known node (tree storage / display path derivation)
        check(TransferThread::internalStringTostring(lt.pathTree.resolve(lt.actionToDoListTransfer.at(2)->srcNode))
              == "/src/dir3/file3.dat", "pathTree.resolve round-trip");
        lt.moveItemsOnTop({3,5});
        expect_order(lt, {3,5,1,2,4,6}, "moveItemsOnTop({3,5})");
    }
    { ListThread lt(&facility); populate(lt, 6, true);
      lt.moveItemsOnBottom({2,4}); expect_order(lt, {1,3,5,6,2,4}, "moveItemsOnBottom({2,4})"); }
    { ListThread lt(&facility); populate(lt, 6, true);
      lt.moveItemsUp({3,5});      expect_order(lt, {1,3,2,5,4,6}, "moveItemsUp({3,5})"); }
    { ListThread lt(&facility); populate(lt, 6, true);
      lt.moveItemsDown({2,4});    expect_order(lt, {1,3,2,5,4,6}, "moveItemsDown({2,4})"); }
    { ListThread lt(&facility); populate(lt, 6, true);
      lt.removeItems({2,4});      expect_order(lt, {1,3,5,6}, "removeItems({2,4}) tombstones"); }
    // idempotence / no-op edges
    { ListThread lt(&facility); populate(lt, 1, true);
      lt.moveItemsOnTop({1}); lt.moveItemsUp({1}); expect_order(lt, {1}, "single-item no-op"); }
    { ListThread lt(&facility); populate(lt, 6, true);
      lt.moveItemsOnTop({1});     expect_order(lt, {1,2,3,4,5,6}, "moveOnTop of already-first is no-op"); }

    // ---- engine->UI change-list contract (newActionOnList): the engine is authoritative and
    //      tells the model ID/position-based MoveItem/RemoveItem deltas -- never raw pointers.
    //      newActionOnList is emitted synchronously by sendActionDone() (direct connection, no
    //      event loop), so a connected lambda captures the exact deltas. ----
    {
        ListThread lt(&facility); populate(lt, 6, true);
        std::vector<Ultracopier::ReturnActionOnCopyList> captured;
        QObject::connect(&lt, &ListThread::newActionOnList,
            [&captured](const std::vector<Ultracopier::ReturnActionOnCopyList> &a){
                captured.insert(captured.end(), a.begin(), a.end()); });

        // skip(id) of a not-running entry -> ONE RemoveItem delta (moveAt==1 marks "skip", not a
        // plain remove), carrying the skipped id + its live position; and it drops from the order.
        lt.skip(3);
        lt.sendActionDone();
        check(captured.size()==1 && captured[0].type==Ultracopier::RemoveItem
              && captured[0].userAction.moveAt==1 && captured[0].addAction.id==3
              && captured[0].userAction.position==2,
              "skip(3) -> one RemoveItem{id=3,pos=2,skip} delta");
        expect_order(lt, {1,2,4,5,6}, "skip(3) removes id 3 from the live order");

        // skipInternal of a missing id -> false, and emits NO delta (engine stays consistent).
        captured.clear();
        check(lt.skipInternal(999)==false, "skipInternal(missing id) returns false");
        lt.sendActionDone();
        check(captured.empty(), "skipInternal(missing id) emits no change-list delta");
    }
    {
        ListThread lt(&facility); populate(lt, 6, true);
        std::vector<Ultracopier::ReturnActionOnCopyList> captured;
        QObject::connect(&lt, &ListThread::newActionOnList,
            [&captured](const std::vector<Ultracopier::ReturnActionOnCopyList> &a){
                captured.insert(captured.end(), a.begin(), a.end()); });
        // moveItemsOnTop({3,5}) -> two MoveItem deltas (ids 3 then 5 -> positions 0 then 1), and
        // the order matches. The deltas reference ids (addAction.id), never pointers.
        lt.moveItemsOnTop({3,5});
        lt.sendActionDone();
        check(captured.size()==2
              && captured[0].type==Ultracopier::MoveItem && captured[0].addAction.id==3 && captured[0].userAction.moveAt==0
              && captured[1].type==Ultracopier::MoveItem && captured[1].addAction.id==5 && captured[1].userAction.moveAt==1,
              "moveItemsOnTop({3,5}) -> MoveItem{3->0},MoveItem{5->1} deltas");
        expect_order(lt, {3,5,1,2,4,6}, "moveItemsOnTop({3,5}) order matches the deltas");
    }

    // ---- pause/resume state contract + idempotency (isInPause signal). Driven on an IDLE engine
    //      (no queued transfers / inode work) so it exercises ONLY the pause state machine,
    //      FS-free: pause->true, double-pause is a no-op, resume->false, double-resume is a no-op. ----
    {
        ListThread lt(&facility);
        std::vector<bool> ev;
        QObject::connect(&lt, &ListThread::isInPause, [&ev](const bool &p){ ev.push_back(p); });
        lt.pause();
        check(ev.size()==1 && ev[0]==true,  "pause() -> isInPause(true)");
        lt.pause();                                   // already paused
        check(ev.size()==1,                 "pause() while paused is a no-op (no duplicate signal)");
        lt.resume();
        check(ev.size()==2 && ev[1]==false, "resume() -> isInPause(false)");
        lt.resume();                                  // already running
        check(ev.size()==2,                 "resume() while running is a no-op");
    }

    // (exportTransferList/importTransferList are intentionally NOT unit-tested here: they go
    //  through a QUEUED signal to a slot on the moveToThread(this) ListThread, so they only run
    //  with the thread's event loop started -- which pulls in real thread lifecycle and fights this
    //  FS-free design. They are better covered via the started-thread engine_api driver with a
    //  QFileDialog hook; tracked as remaining #14 work.)

    // ---- SCALE: a multi-million-entry list, reordered/removed in milliseconds ----
    {
        const uint64_t N = 2000000;   // 2M entries
        ListThread lt(&facility);
        auto t0 = std::chrono::steady_clock::now();
        populate(lt, N, /*distinctPaths*/false);   // shared node -> bounded memory
        auto t1 = std::chrono::steady_clock::now();
        // move a handful to the top + remove a handful: O(N) reindex, tiny per-op
        lt.moveItemsOnTop({N, N-1, N/2, 2, 7});
        lt.removeItems({3, 4, 5});
        auto t2 = std::chrono::steady_clock::now();
        auto ms = [](auto a, auto b){ return std::chrono::duration_cast<std::chrono::milliseconds>(b-a).count(); };
        const std::vector<uint64_t> got = order(lt);
        // top 5 are the moved ids in selection-encounter order (ascending index): 2,7,N/2,N-1,N
        check(got.size() == N - 3, "scale: 3 removed -> N-3 live (" + std::to_string(got.size()) + ")");
        check(got.size() >= 5 && got[0]==2 && got[1]==7 && got[2]==N/2 && got[3]==N-1 && got[4]==N,
              "scale: moved-to-top ids in encounter order");
        std::fprintf(stdout, "  [scale] N=%llu populate+index=%lldms reorder+remove=%lldms\n",
                     (unsigned long long)N, (long long)ms(t0,t1), (long long)ms(t1,t2));
        check(ms(t1,t2) < 5000, "scale: reorder+remove of 2M-entry list well under 5s");
    }

    std::fprintf(stdout, "== listreorder: %d checks, %d failure(s) ==\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
