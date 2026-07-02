/** \file OpScheduler.h
\brief Operation-decomposition scheduler CORE for the pipelined (io_uring / IOCP) copy backends.
\author alpha_one_x86
\licence GPL3, see the file COPYING

This is STAGE 1a of the tiered-parallelism pipelining (see pipeline/PIPELINING_DESIGN.md): the pure
dependency logic, with NO I/O and NO Qt, so it is unit-testable in isolation (test/unit/opscheduler_test.cpp)
and can later be linked into the backends (stage 1b) unchanged. It is DELIBERATELY dependency-free
(only <vector>/<cstdint>) — same shape as ContiguousWatermark.h.

It decomposes a transfer subtree (a list of FsNode indices — NOT paths; paths are resolved lazily at
dispatch time, per the SCALE rule "memory must be O(in-flight ops), not O(paths)") into typed basic FS
operations with explicit dependency EDGES, and drives their readiness. It does NOT itself run the ops:
a caller pulls readyOps(), dispatches each to the pool classOf() names, and calls complete() when the
op finishes — which unblocks its dependents. Whatever order the caller runs ready ops in, the hard FS
edges (mkdir-before-open, open<data<close, file meta after close, FOLDER date after EVERY contained file
is closed) can never be violated, because an op is never ready until all its prerequisites complete. This
is exactly the invariant the ops_dependency.py integration test asserts against the live engine; here it
is proven at the plan level.

STAGE 1a scope: single logical Data op per file (the existing per-file chunk pipeline stays INSIDE that
Data op). Move (Unlink/Rmdir) and Prealloc are folded into the existing op kinds for now and added as
explicit kinds when stage 1b wires this into the move path. */

#ifndef ULTRACOPIER_OPSCHEDULER_H
#define ULTRACOPIER_OPSCHEDULER_H

#include <vector>
#include <cstdint>
#include <cstddef>

namespace ucpipe {

/// \brief a basic FS operation kind
enum class OpKind : uint8_t {
    Mkdir,        ///< create a destination directory (inode)
    Open,         ///< open source (read) + destination (create/write) for a file (inode)
    Data,         ///< stream the file content src->dst (data; the per-file chunk pipeline lives here)
    Close,        ///< close the file's handles (inode)
    SetMetaFile,  ///< set the file's perms + date (inode)
    SetMetaDir    ///< set a directory's perms + date (inode) — must run after all its contents
};

/// \brief which pool an op is dispatched to (tiered parallelism; HDD baseline widths in PIPELINING_DESIGN.md)
enum class OpClass : uint8_t {
    Inode,       ///< metadata op: strong parallelism (~8x cores)
    DataSmall,   ///< file < smallMax: medium parallelism (~cores)
    DataMedium,  ///< file < mediumMax: small parallelism (~cores/4, min 2)
    DataLarge    ///< file >= mediumMax: serial (one streaming copy saturates the device)
};

/// \brief a file or directory in the transfer subtree. Indices only — paths resolved lazily elsewhere.
struct FsNode {
    bool isDir;      ///< true = directory, false = regular file
    int parent;      ///< index of the parent DIRECTORY node, or -1 for a subtree root
    uint64_t size;   ///< file size in bytes (0 for directories) — used for the data-pool classification
    /// \brief the engine's action id for this node (ListThread's generateIdNumber order). The single-thread
    /// driver (stage 1b.1) MUST dispatch the lowest-engineId ready op first to reproduce the engine's
    /// ascending-id dispatch order byte-for-byte (readyOps() alone yields FsNode-INDEX order, which differs
    /// and would silently perturb collision/put-to-end/resume). 0 when unused (pure-DAG tests / stage 1a).
    uint64_t engineId = 0;
};

/// \brief size thresholds for the tiered data pools (defaults = HDD baseline)
struct SizeThresholds {
    uint64_t smallMax  = 4ULL * 1024;    ///< size <  smallMax  -> DataSmall
    uint64_t mediumMax = 64ULL * 1024;   ///< size <  mediumMax -> DataMedium ; else DataLarge
};

class OpScheduler {
public:
    struct Op {
        OpKind kind;
        int node;                    ///< the FsNode this op acts on
        int unmet;                   ///< # of prerequisites not yet complete (ready when this hits 0)
        bool completed;
        bool dispatched;             ///< returned by readyOps() already (so it is handed out once)
        std::vector<int> dependents; ///< op ids that require THIS op to finish first
    };

    explicit OpScheduler(const std::vector<FsNode> &nodes, SizeThresholds th = SizeThresholds())
        : nodes_(nodes), th_(th), completedCount_(0)
    {
        build();
    }

    size_t opCount() const { return ops_.size(); }
    const Op &op(int id) const { return ops_[(size_t)id]; }
    bool allDone() const { return completedCount_ == ops_.size(); }

    /// \brief ops runnable NOW (every prerequisite complete), each returned at most once.
    std::vector<int> readyOps()
    {
        std::vector<int> out;
        for (size_t i = 0; i < ops_.size(); i++)
            if (!ops_[i].completed && !ops_[i].dispatched && ops_[i].unmet == 0)
            {
                ops_[i].dispatched = true;
                out.push_back((int)i);
            }
        return out;
    }

    /// \brief the single ready op whose node has the LOWEST engineId (ties broken by op id, for
    /// determinism), marked dispatched; -1 if none is ready. This is the stage-1b.1 single-inode-thread
    /// dispatch pick: consuming ready ops in ascending engineId reproduces the engine's dispatch order,
    /// the invariant that keeps the driver's output byte-identical (diff==0).
    int nextReadyByEngineId()
    {
        int best = -1;
        uint64_t bestEid = 0;
        for (size_t i = 0; i < ops_.size(); i++)
            if (!ops_[i].completed && !ops_[i].dispatched && ops_[i].unmet == 0)
            {
                const uint64_t eid = nodes_[(size_t)ops_[i].node].engineId;
                if (best < 0 || eid < bestEid)   // strictly-lower engineId wins; equal keeps the lower op id
                {
                    best = (int)i;
                    bestEid = eid;
                }
            }
        if (best >= 0)
            ops_[(size_t)best].dispatched = true;
        return best;
    }

    /// \brief mark op `id` complete; decrements its dependents' unmet counts (may make them ready).
    void complete(int id)
    {
        Op &o = ops_[(size_t)id];
        if (o.completed)
            return;
        o.completed = true;
        completedCount_++;
        for (int d : o.dependents)
            if (ops_[(size_t)d].unmet > 0)
                ops_[(size_t)d].unmet--;
    }

    /// \brief the pool this op belongs to (Data ops are size-bucketed; everything else is Inode).
    OpClass classOf(int id) const
    {
        const Op &o = ops_[(size_t)id];
        if (o.kind != OpKind::Data)
            return OpClass::Inode;
        const uint64_t sz = nodes_[(size_t)o.node].size;
        if (sz < th_.smallMax)
            return OpClass::DataSmall;
        if (sz < th_.mediumMax)
            return OpClass::DataMedium;
        return OpClass::DataLarge;
    }

    // --- op-id accessors used by tests / stage 1b (return -1 if the node has no such op) ----------
    int mkdirOp(int node) const      { return dirMkdir_[(size_t)node]; }
    int setMetaDirOp(int node) const { return dirMeta_[(size_t)node]; }
    int openOp(int node) const       { return fileOpen_[(size_t)node]; }
    int dataOp(int node) const       { return fileData_[(size_t)node]; }
    int closeOp(int node) const      { return fileClose_[(size_t)node]; }
    int setMetaFileOp(int node) const{ return fileMeta_[(size_t)node]; }

private:
    int addOp(OpKind k, int node)
    {
        Op o; o.kind = k; o.node = node; o.unmet = 0; o.completed = false; o.dispatched = false;
        ops_.push_back(o);
        return (int)ops_.size() - 1;
    }
    void addEdge(int prereq, int dependent)   // `dependent` needs `prereq` to finish first
    {
        ops_[(size_t)prereq].dependents.push_back(dependent);
        ops_[(size_t)dependent].unmet++;
    }

    void build()
    {
        const int N = (int)nodes_.size();
        dirMkdir_.assign(N, -1); dirMeta_.assign(N, -1);
        fileOpen_.assign(N, -1); fileData_.assign(N, -1);
        fileClose_.assign(N, -1); fileMeta_.assign(N, -1);

        // Pass 1: create each node's ops (record their ids).
        for (int n = 0; n < N; n++)
        {
            if (nodes_[(size_t)n].isDir)
            {
                dirMkdir_[(size_t)n] = addOp(OpKind::Mkdir, n);
                dirMeta_[(size_t)n]  = addOp(OpKind::SetMetaDir, n);
            }
            else
            {
                fileOpen_[(size_t)n]  = addOp(OpKind::Open, n);
                fileData_[(size_t)n]  = addOp(OpKind::Data, n);
                fileClose_[(size_t)n] = addOp(OpKind::Close, n);
                fileMeta_[(size_t)n]  = addOp(OpKind::SetMetaFile, n);
            }
        }

        // Pass 2: wire the dependency edges.
        for (int n = 0; n < N; n++)
        {
            const int par = nodes_[(size_t)n].parent;
            if (nodes_[(size_t)n].isDir)
            {
                // A directory is created after its parent directory; its meta is set after it exists.
                if (par >= 0)
                    addEdge(dirMkdir_[(size_t)par], dirMkdir_[(size_t)n]);
                addEdge(dirMkdir_[(size_t)n], dirMeta_[(size_t)n]);
                // The parent's folder-date must land AFTER this subdir is fully finalized.
                if (par >= 0)
                    addEdge(dirMeta_[(size_t)n], dirMeta_[(size_t)par]);
            }
            else
            {
                // open after the parent dir exists; open < data < close < file-meta.
                if (par >= 0)
                    addEdge(dirMkdir_[(size_t)par], fileOpen_[(size_t)n]);
                addEdge(fileOpen_[(size_t)n],  fileData_[(size_t)n]);
                addEdge(fileData_[(size_t)n],  fileClose_[(size_t)n]);
                addEdge(fileClose_[(size_t)n], fileMeta_[(size_t)n]);
                // The parent's folder-date must land AFTER this file is CLOSED (close bumps the parent
                // dir mtime on some FS/OS — the exact rule ops_dependency.py enforces on the engine).
                if (par >= 0)
                    addEdge(fileClose_[(size_t)n], dirMeta_[(size_t)par]);
            }
        }
    }

    std::vector<FsNode> nodes_;
    SizeThresholds th_;
    std::vector<Op> ops_;
    size_t completedCount_;
    std::vector<int> dirMkdir_, dirMeta_, fileOpen_, fileData_, fileClose_, fileMeta_;
};

} // namespace ucpipe

#endif // ULTRACOPIER_OPSCHEDULER_H
