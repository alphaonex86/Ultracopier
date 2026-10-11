/** \file TransferThreadPipelined.h
\brief Shared base class for the pipelined copy backends (io_uring and Windows IOCP)
\author alpha_one_x86
\licence GPL3, see the file COPYING

Both pipelined backends (TransferThreadUring on Linux, TransferThreadWin on Windows)
move data the same way: a fixed pool of buffers cycled through asynchronous reads and
writes against a completion queue. Everything around that — the transfer state
machine, move/rename, symlink dispatch, collision tracking, pause/resume, progress
accounting and all the Qt signals/slots — is identical and lives HERE, once, so a
change to the transfer logic can't be applied to one backend and forgotten in the
other. A backend subclass implements only the I/O hooks (open/close/doTransferPipeline
+ remainSourceOpen/remainDestinationOpen) and, optionally, trySymlinkCopy(). */

#ifndef TRANSFERTHREADPIPELINED_H
#define TRANSFERTHREADPIPELINED_H

#include <QObject>
#include <QMutex>
#include <QSemaphore>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <atomic>

//before the next define
#include "../CopyEngineUltracopier-SpecVariable.h"

#include <time.h>

#include "../TransferThread.h"
#include "../Environment.h"
#include "../DriveManagement.h"
#include "../StructEnumDefinition_CopyEngine.h"

/// \brief Shared lifecycle for a full async pipelined file transfer
class TransferThreadPipelined : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadPipelined();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief get the reading letter (the pipelined backends have no separate read thread)
    char readingLetter() const;
    /// \brief get the writing letter (the pipelined backends have no separate write thread)
    char writingLetter() const;
    #endif

    //not copied size, ...
    uint64_t realByteTransfered() const;
    std::pair<uint64_t, uint64_t> progression() const;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    bool setBlockSize(const unsigned int blockSize);
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    void timeOfTheBlockCopyFinished();
    void setOsSpecFlags(bool os_spec_flags);
    void setNativeCopy(bool native_copy);
    #endif
    void pause();
    void resume();

    bool haveStartTime;
    bool native_copy;
    #if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
    /// \brief progress hook used by the native-copy paths (public: called from the
    /// Windows CopyFileExW progress callback)
    void setProgression(const uint64_t &pos, const uint64_t &size);
    #endif
private slots:
    void preOperation();
    void postOperation();
    void internalStartTheTransfer();
    /// \brief media-reconnect RESUME decision: on a retry after a SOURCE read error, re-stat
    /// the source and (only if it is byte-identical: size + mtime sec&nsec + ctime all match,
    /// and a non-empty contiguous prefix is already written) resume the transfer at
    /// contiguousWrittenOffset; otherwise restart from 0. Wired to internalStartResumeAfterErrorAndSeek.
    void resumeAfterErrorAndSeek();
    /// \brief the retry decision, run ON THE WORKER THREAD (queued from retryAfterError): a retry
    /// answered while the worker is still draining the failed attempt would otherwise close its
    /// fds and clear its error flags underneath it, and the worker's tail then reported the partial
    /// file as COMPLETE (the retry itself was then refused as "already used").
    void retryAfterErrorInternal();

    void setFileExistsActionInternal(const FileExistsAction &action);
signals:
    void internalStartResumeAfterErrorAndSeek() const;
    void internalStartRetryAfterError() const;
    void internalStartPostOperation() const;
    void openReadSignal(const INTERNALTYPEPATH &file, const Ultracopier::CopyMode &mode);
    void openWriteSignal(const INTERNALTYPEPATH &file, const uint64_t &startSize);
    void setFileExistsActionSend(const FileExistsAction &action);
public slots:
    void startTheTransfer();
    void stop();
    void skip();
    /// \brief pipelined backend has a single completion ring (no separate read/write close to
    /// drain), so Idle already implies fully finalized.
    bool readyForReuse() const { return true; }
    int64_t copiedSize();
    void putAtBottom();
    bool setFiles(const INTERNALTYPEPATH &source, const int64_t &size,
                  const INTERNALTYPEPATH &destination, const Ultracopier::CopyMode &mode);
    void setFileExistsAction(const FileExistsAction &action);
    void retryAfterError();
protected:
    /// \brief wire the internal queued signals/slots; each backend ctor calls this
    /// after its members are initialised and before start().
    void connectInternalSignals();

    // ---- Backend I/O hooks: implemented by each subclass ------------------------
    /// \brief open the source; store the fd/handle internally; return 0 on success, -1 on error
    virtual int openSourceFile()=0;
    /// \brief open the destination; return 0 on success, -1 on error
    virtual int openDestFile(uint64_t startSize)=0;
    /// \brief close any open source/destination
    virtual void closeFiles()=0;
    /// \brief run the async read/write pipeline source->destination
    virtual void doTransferPipeline()=0;
    /// \brief interrupt a doTransferPipeline() that is BLOCKED waiting for I/O completions, called from
    /// the SCHEDULER (ListThread) thread by stop()/skip() to make the worker notice stopIt/needSkip and
    /// tear itself down. Default = closeFiles() (the historical wake: closing the fds/handles forces the
    /// pending ops to complete so the blocking wait returns). A backend whose worker POLLS stopIt itself
    /// (e.g. a bounded completion-wait) overrides this to a NO-OP: closing the handles from another thread
    /// while the worker is mid ReadFile/WriteFile is a data race (the closed HANDLE/fd is even recycled by a
    /// sibling inode thread) -- the IOCP heap-corruption Bug C. Letting the worker close its OWN handles
    /// removes the cross-thread hazard entirely.
    virtual void interruptTransferForStop() { closeFiles(); }
    virtual bool remainSourceOpen() const=0;
    virtual bool remainDestinationOpen() const=0;
    /// \brief cut the still-open destination to contiguousWrittenOffset so a KEPT partial (pure fault
    /// skip salvage, media-reconnect resume prefix) is a faithful prefix: the out-of-order pipelined
    /// writes may have landed bytes above a hole. Called by the base before closeFiles() on every
    /// non-success exit of a destination that is ours. Best-effort (a failure only leaves a longer file).
    virtual void trimDestinationToContiguous()=0;
    /// \brief handle a symlinked source. Return true if fully handled (success OR error
    /// already emitted); false to fall back to a normal pipelined copy. Default: false
    /// (follow the link / copy as a regular file). The POSIX backend overrides it.
    virtual bool trySymlinkCopy();
    /// \brief apply the source date/time (read into the base by readSourceFileDateTime)
    /// onto the STILL-OPEN destination handle/fd, just before closeFiles(). Returns true
    /// on success (then doFilePostOperation skips the reopen-to-set-times path). Default:
    /// false (not handled -> the base reopens the destination as before). Each backend
    /// overrides it: SetFileTime(destHandle,...) on Windows, futimens(destFd,...) on Linux.
    virtual bool applyDateTimeOnOpenDestination();

    // ---- Shared transfer logic --------------------------------------------------
    bool remainFileOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
    void checkIfAllIsClosedAndDoOperations();
    /// \brief if native_copy is enabled, copy via the OS (CopyFileExW / copy_file_range)
    /// and finalize. Returns true if it handled the copy (success or error already
    /// emitted), false to fall back to the async pipeline. Mirrors async behaviour.
    bool tryNativeCopy();
    /// \brief re-read source+destination and compare their xxh3-64 hashes (option
    /// transferChecksum). Returns true when they match. Mirrors async runChecksumVerify().
    bool runChecksumVerify();

    /// \brief true when the destination is OURS to delete (we created it, or it was empty), i.e. NOT a
    /// user's pre-existing file. Mirrors TransferThreadAsync::destinationIsOursToRemove() (the #9
    /// invariant). Used by the #25 corrupt-dest cleanup so a checksum-failed copy never deletes a
    /// user's pre-existing destination. Backed by destinationPreExisted, captured in preOperation().
    bool destinationIsOursToRemove() const;
    /// \brief the ONE completion of a stopped/skipped transfer (partial cleanup, Idle, postOperationStopped),
    /// run by whichever side owns the tail: the worker when it is inside the pipeline, else the caller of
    /// stop()/skip(). stopFinalized guarantees it runs at most once per attempt.
    void finalizeStoppedTransfer();
    void finalizeIfStopRequested();

    /// \brief speed limit (async parity, WriteThread's waitNewClockForSpeed): called by the backend before it
    /// ISSUES each new block read; every multiForBigSpeed blocks it waits one tick of ListThread's speed clock
    /// (timeOfTheBlockCopyFinished releases one permit per tick). The pipelined backends had the clock plumbing
    /// but never waited on it, so the UI's "limit speed" did nothing on io_uring/IOCP. Returns at once when no
    /// limit is set. Counter reset per file by the caller (resetSpeedThrottle()).
    void throttleBlockForSpeedLimit();
    void resetSpeedThrottle() { blocksSinceSpeedTick=0; }
    int blocksSinceSpeedTick;

    /// \brief fold a just-COMPLETED destination write extent [off, off+len) into the
    /// contiguous-from-0 low-water mark. MUST be called once per write completion with the
    /// LIVE (post-short-write-advance) fileOffset+result, never the logical chunk size --
    /// out-of-order completions are buffered in completedWriteExtents (bounded by the in-flight
    /// buffer pool) until they become contiguous. contiguousWrittenOffset is the ONLY offset
    /// safe to resume from; transferProgression counts in completion order and may sit above a hole.
    void recordCompletedWrite(uint64_t off, uint64_t len);

    // ---- Shared state -----------------------------------------------------------
    uint64_t transferProgression;
    uint64_t checksumProgression; // bytes hashed so far during the post-copy verify
    /// \brief set by runChecksumVerify() when BOTH digests were computed and DIFFER (a real content mismatch),
    /// as opposed to a verify aborted by stop/skip. The mismatch handling keys on this, not on
    /// "!ok && !stopIt && !needSkip": the policy's skip() for the mismatch error can land between the
    /// verify's errorOnFile and that check (the error is emitted inside the verify), and then the corrupt
    /// full-size destination was KEPT and counted complete (iouring_checksum_skip_hang, 1 run in 4).
    bool checksumMismatchDetected;
    /// \brief largest offset N such that EVERY byte in [0,N) is written to the destination.
    /// Advanced only by recordCompletedWrite across contiguous extents; the safe resume point.
    uint64_t contiguousWrittenOffset;
    /// \brief out-of-order completed write extents not yet contiguous with contiguousWrittenOffset.
    /// Bounded by the in-flight buffer pool (<= NUM_BUFFERS), cleared at each (re)start.
    std::vector<std::pair<uint64_t,uint64_t> > completedWriteExtents;
    /// \brief 0 => start/restart the transfer from offset 0; >0 => RESUME keeping the dest
    /// prefix [0,resumeFromOffset). Set ONLY by resumeAfterErrorAndSeek; zeroed at every new
    /// file (setFiles) and on every restart path so a stale value can't trim an unrelated dest.
    uint64_t resumeFromOffset;

    /// \brief A FRESH destination file whose size is >= this gets a best-effort PREALLOCATION (reserve
    /// its full extent up front: io_uring fallocate(FALLOC_FL_KEEP_SIZE) / IOCP SetEndOfFile) so the
    /// out-of-order pipelined writes land contiguously instead of growing the file piecemeal. Files
    /// SMALLER than this are skipped: they don't fragment, and preallocating them would only add
    /// syscalls to the very small-file path the pipelining is trying to speed up. io_uring/IOCP only
    /// (async is untouched). 1 MiB — below it a file is almost always laid out contiguously anyway.
    static constexpr uint64_t PREALLOCATE_MIN_SIZE=1024*1024;

    unsigned int blockSize;
    bool os_spec_flags;
    bool realMove;
    /// \brief did the destination PRE-EXIST with non-empty content before we touched it? Captured in
    /// preOperation() on the first leg only (resumeFromOffset==0; a resume prefix is OURS, not the
    /// user's). Backs destinationIsOursToRemove() for the #25 corrupt-dest cleanup.
    bool destinationPreExisted=false;
    /// \brief true once openDestFile() succeeded for the CURRENT file: the destination now holds OUR bytes
    /// (a partial after an error). A RESTART-FROM-0 retry re-runs preOperation(); it must then neither
    /// re-capture destinationPreExisted (our partial is not the user's file) nor re-apply the collision
    /// policy to it (fileCollision=Skip / if-newer skipped the file and left the partial counted as done).
    /// Reset per file in setFiles().
    bool destinationTouched=false;

    volatile bool putInPause;
    /// \brief true while the worker thread runs the copy of the current file (open..close): a stop()/skip()
    /// arriving then only sets the flags and lets the worker finish -- publishing Idle from the caller thread
    /// while the worker still copied let ListThread hand it the NEXT file underneath (its tail then unlinked
    /// the new destination and reported the new entry done without copying it).
    std::atomic<bool> workerInsidePipeline;
    std::atomic<bool> stopFinalized;

    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QSemaphore waitNewClockForSpeed;
    volatile int multiForBigSpeed;
    #endif
    QSemaphore pauseMutex;

    std::string errorString_internal;

    int64_t size_at_open;
    uint64_t mtime_at_open;
    // Sub-second mtime + ctime captured at source open, used by the resume gate to detect an
    // in-place rewrite that keeps size AND whole-second mtime identical (which 1s-mtime+size
    // alone would miss -> spliced old-prefix/new-tail corruption). ctime changes on any
    // metadata/content write. If nsec is unavailable (0 / FAT-class FS) the gate must NOT resume.
    uint64_t mtime_nsec_at_open;
    uint64_t ctime_at_open;
    INTERNALTYPEPATH sourceFile;
    INTERNALTYPEPATH destFile;
};

#endif // TRANSFERTHREADPIPELINED_H
