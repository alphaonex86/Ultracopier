// Standalone unit test for the operation-decomposition scheduler CORE (pipeline/OpScheduler.h) —
// STAGE 1a of the tiered-parallelism pipelining (pipeline/PIPELINING_DESIGN.md). It proves, at the
// plan level (no I/O, no Qt), the exact invariants ops_integrity.py / ops_dependency.py assert on the
// live engine: NO duplicate / NO forgotten op, and every hard FS dependency edge holds under ANY order
// the caller completes ready ops in — including the transitive rule that a FOLDER's date is set only
// after EVERY file under it (however deep) is closed. Also checks the tiered-pool classification.
//
// Built + run by cases/opscheduler_unit.py. Returns 0 iff every check passes.
#include "../../pipeline/OpScheduler.h"
#include <cstdio>
#include <vector>
#include <cstdint>

using namespace ucpipe;

static int g_fail = 0;
#define CHECK(cond, msg) do{ if(!(cond)){ std::printf("FAIL: %s (line %d)\n", msg, __LINE__); g_fail++; } }while(0)

// A small nested tree:  root/ {a.txt(6), big.iso(4MiB), mid.dat(8KiB), sub/ {b.bin(30KiB),
//                        deep/ {c.dat(2KiB)}}}
static std::vector<FsNode> makeTree()
{
    std::vector<FsNode> n;
    n.push_back({true,  -1, 0});                 // 0 root
    n.push_back({true,   0, 0});                 // 1 root/sub
    n.push_back({true,   1, 0});                 // 2 root/sub/deep
    n.push_back({false,  0, 6});                 // 3 root/a.txt        -> DataSmall
    n.push_back({false,  0, 4ULL*1024*1024});    // 4 root/big.iso      -> DataLarge
    n.push_back({false,  0, 8ULL*1024});         // 5 root/mid.dat      -> DataMedium
    n.push_back({false,  1, 30ULL*1024});        // 6 root/sub/b.bin    -> DataMedium
    n.push_back({false,  2, 2ULL*1024});         // 7 root/sub/deep/c.dat -> DataSmall
    return n;
}

// Drive a full drain, completing ready ops with one of two strategies, and validate afterwards.
// strategy 0 = complete ALL currently-runnable ops each round; strategy 1 = complete only ONE (the
// last) each round — a maximally different completion order that stresses the readiness engine and
// proves the edges hold order-independently. `pool` is a local worklist of ops that readyOps() has
// handed out (each op exactly once) but that are not yet completed. Fills time[] (completion order)
// and dispatch[] (# times each op was handed out — must end at exactly 1).
static bool drain(int strategy, std::vector<int> &time, std::vector<int> &dispatch)
{
    std::vector<FsNode> nodes = makeTree();
    OpScheduler s(nodes);
    const int M = (int)s.opCount();
    time.assign((size_t)M, -1);
    dispatch.assign((size_t)M, 0);
    std::vector<int> pool;
    int counter = 0, guard = 0;
    while (!s.allDone())
    {
        for (int id : s.readyOps())       // pull the ops that became ready since last round
        {
            dispatch[(size_t)id]++;
            pool.push_back(id);
        }
        if (pool.empty())                 // ready==empty AND nothing pending -> real deadlock
        {
            std::printf("FAIL: deadlock — no runnable ops but not all done (strategy %d)\n", strategy);
            return false;
        }
        if (strategy == 0)
        {
            std::vector<int> batch; batch.swap(pool);
            for (int id : batch) { s.complete(id); time[(size_t)id] = counter++; }
        }
        else
        {
            int id = pool.back(); pool.pop_back();
            s.complete(id); time[(size_t)id] = counter++;
        }
        if (++guard > 100000) { std::printf("FAIL: no termination (strategy %d)\n", strategy); return false; }
    }
    return true;
}

// Validate a completed drain's time[]/dispatch[] against every invariant.
static void validate(int strategy)
{
    std::vector<FsNode> nodes = makeTree();
    OpScheduler s(nodes);      // a fresh identical scheduler just to read the op ids / structure
    std::vector<int> time, dispatch;
    if (!drain(strategy, time, dispatch)) { g_fail++; return; }
    const int M = (int)s.opCount();

    // 1. NO duplicate / NO forgotten: each op handed out exactly once and completed exactly once.
    for (int i = 0; i < M; i++)
    {
        CHECK(dispatch[(size_t)i] == 1, "each op dispatched exactly once");
        CHECK(time[(size_t)i] >= 0, "each op completed (nothing forgotten)");
    }
    CHECK((int)s.opCount() == 3*2 + 5*4, "op count = dirs*2 + files*4 = 26");

    // 2. Every declared dependency EDGE respected: prereq completes strictly before its dependent.
    for (int p = 0; p < M; p++)
        for (int d : s.op(p).dependents)
            CHECK(time[(size_t)p] < time[(size_t)d], "dependency edge: prereq before dependent");

    // 3. Per-file chain: mkdir(parent) < open < data < close < setMetaFile.
    for (int n = 0; n < (int)nodes.size(); n++)
        if (!nodes[(size_t)n].isDir)
        {
            const int op = s.openOp(n), dt = s.dataOp(n), cl = s.closeOp(n), me = s.setMetaFileOp(n);
            CHECK(time[(size_t)op] < time[(size_t)dt], "open < data");
            CHECK(time[(size_t)dt] < time[(size_t)cl], "data < close");
            CHECK(time[(size_t)cl] < time[(size_t)me], "close < setMetaFile");
            const int par = nodes[(size_t)n].parent;
            if (par >= 0)
                CHECK(time[(size_t)s.mkdirOp(par)] < time[(size_t)op], "mkdir(parent) < open(file)");
        }

    // 4. THE folder-date rule (transitive): a directory's SetMetaDir runs AFTER every file anywhere
    //    under it is closed, and after every descendant subdir is finalized. Walk each file up to root.
    for (int n = 0; n < (int)nodes.size(); n++)
        if (!nodes[(size_t)n].isDir)
        {
            int anc = nodes[(size_t)n].parent;
            while (anc >= 0)
            {
                CHECK(time[(size_t)s.closeOp(n)] < time[(size_t)s.setMetaDirOp(anc)],
                      "folder date set only after EVERY contained file (incl. deep) is closed");
                anc = nodes[(size_t)anc].parent;
            }
        }
    // nested dirs are created top-down.
    for (int n = 0; n < (int)nodes.size(); n++)
        if (nodes[(size_t)n].isDir && nodes[(size_t)n].parent >= 0)
            CHECK(time[(size_t)s.mkdirOp(nodes[(size_t)n].parent)] < time[(size_t)s.mkdirOp(n)],
                  "mkdir(parent dir) < mkdir(child dir)");
}

// Drive the single-inode-thread stage-1b.1 pick (nextReadyByEngineId) and prove the tie-break: at EVERY
// step the op chosen has the LOWEST node engineId among all currently-ready ops (whatever the engineId
// assignment), edges still hold, nothing dup/forgotten. `assign` picks the engineId scheme.
static void validateEngineId(int assign)
{
    std::vector<FsNode> nodes = makeTree();
    for (size_t i = 0; i < nodes.size(); i++)
        nodes[(size_t)i].engineId = (assign == 0) ? (uint64_t)i                    // ascending = node order
                                                   : (uint64_t)(1000 - 7 * i);     // scrambled/descending
    OpScheduler s(nodes);
    const int M = (int)s.opCount();
    std::vector<int> time((size_t)M, -1), dispatch((size_t)M, 0);
    int counter = 0, guard = 0;
    while (!s.allDone())
    {
        // currently-runnable ops, computed independently BEFORE the pick (public Op fields).
        uint64_t minEid = ~0ULL;
        bool any = false;
        for (int i = 0; i < M; i++)
        {
            const OpScheduler::Op &o = s.op(i);
            if (!o.completed && !o.dispatched && o.unmet == 0)
            {
                any = true;
                const uint64_t e = nodes[(size_t)o.node].engineId;
                if (e < minEid) minEid = e;
            }
        }
        if (!any) { CHECK(false, "engineId drive: deadlock"); return; }
        int chosen = s.nextReadyByEngineId();
        CHECK(chosen >= 0, "engineId: a ready op was returned");
        dispatch[(size_t)chosen]++;
        CHECK(nodes[(size_t)s.op(chosen).node].engineId == minEid,
              "engineId tie-break: the LOWEST-engineId ready op is chosen");
        s.complete(chosen); time[(size_t)chosen] = counter++;
        if (++guard > 100000) { CHECK(false, "engineId: no termination"); return; }
    }
    for (int i = 0; i < M; i++)
    {
        CHECK(dispatch[(size_t)i] == 1, "engineId: each op dispatched exactly once");
        CHECK(time[(size_t)i] >= 0, "engineId: each op completed");
    }
    for (int p = 0; p < M; p++)
        for (int d : s.op(p).dependents)
            CHECK(time[(size_t)p] < time[(size_t)d], "engineId drive: every dependency edge still respected");
}

int main()
{
    // Pool classification per size (HDD-baseline thresholds).
    {
        std::vector<FsNode> nodes = makeTree();
        OpScheduler s(nodes);
        CHECK(s.classOf(s.dataOp(3)) == OpClass::DataSmall,  "6B -> DataSmall");
        CHECK(s.classOf(s.dataOp(7)) == OpClass::DataSmall,  "2KiB -> DataSmall");
        CHECK(s.classOf(s.dataOp(5)) == OpClass::DataMedium, "8KiB -> DataMedium");
        CHECK(s.classOf(s.dataOp(6)) == OpClass::DataMedium, "30KiB -> DataMedium");
        CHECK(s.classOf(s.dataOp(4)) == OpClass::DataLarge,  "4MiB -> DataLarge");
        CHECK(s.classOf(s.mkdirOp(0)) == OpClass::Inode,     "mkdir -> Inode pool");
        CHECK(s.classOf(s.closeOp(3)) == OpClass::Inode,     "close -> Inode pool");
        CHECK(s.classOf(s.setMetaDirOp(1)) == OpClass::Inode,"setMetaDir -> Inode pool");
    }

    // Two very different completion orders must BOTH satisfy every invariant (order-independence).
    validate(0);
    validate(1);

    // stage-1b.1 single-thread pick: lowest-engineId ready op, under both an ascending and a scrambled
    // engineId assignment (proves the pick follows engineId, not FsNode index).
    validateEngineId(0);
    validateEngineId(1);

    // Degenerate trees: empty, a single file, a childless dir — no crash, all invariants trivially hold.
    {
        std::vector<FsNode> empty;
        OpScheduler s(empty);
        CHECK(s.opCount() == 0, "empty tree -> 0 ops");
        CHECK(s.allDone(), "empty tree already done");
        CHECK(s.readyOps().empty(), "empty tree no ready ops");
    }
    {
        std::vector<FsNode> one; one.push_back({false, -1, 100});   // a lone root file
        OpScheduler s(one);
        int done = 0, guard = 0;
        while (!s.allDone() && ++guard < 100)
            for (int id : s.readyOps()) { s.complete(id); done++; }
        CHECK(done == 4, "lone file -> open/data/close/setMeta");
        CHECK(s.allDone(), "lone file drained");
    }
    {
        std::vector<FsNode> d; d.push_back({true, -1, 0});          // a lone empty dir
        OpScheduler s(d);
        // mkdir must come before setMetaDir even with no children.
        std::vector<int> r1 = s.readyOps();
        CHECK(r1.size() == 1 && s.op(r1[0]).kind == OpKind::Mkdir, "empty dir: mkdir ready first");
        s.complete(r1[0]);
        std::vector<int> r2 = s.readyOps();
        CHECK(r2.size() == 1 && s.op(r2[0]).kind == OpKind::SetMetaDir, "empty dir: setMetaDir after mkdir");
        s.complete(r2[0]);
        CHECK(s.allDone(), "empty dir drained");
    }

    if (g_fail == 0) std::printf("opscheduler_test: ALL PASS\n");
    else             std::printf("opscheduler_test: %d FAILURE(S)\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
