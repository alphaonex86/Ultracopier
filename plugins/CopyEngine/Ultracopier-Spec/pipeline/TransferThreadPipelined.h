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
    void setBuffer(const bool buffer);
    #if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
    /// \brief progress hook used by the native-copy paths (public: called from the
    /// Windows CopyFileExW progress callback)
    void setProgression(const uint64_t &pos, const uint64_t &size);
    #endif

    // Write collision tracking (replaces writeThread.writeFileList). Typed on the base
    // so ListThread holds one pointer type regardless of which backend is compiled.
    QMultiHash<QString, TransferThreadPipelined *> *writeFileList;
    QMutex *writeFileListMutex;
private slots:
    void preOperation();
    void postOperation();
    void internalStartTheTransfer();
    /// \brief media-reconnect RESUME decision: on a retry after a SOURCE read error, re-stat
    /// the source and (only if it is byte-identical: size + mtime sec&nsec + ctime all match,
    /// and a non-empty contiguous prefix is already written) resume the transfer at
    /// contiguousWrittenOffset; otherwise restart from 0. Wired to internalStartResumeAfterErrorAndSeek.
    void resumeAfterErrorAndSeek();

    void setFileExistsActionInternal(const FileExistsAction &action);
signals:
    void internalStartResumeAfterErrorAndSeek() const;
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
    /// \brief open the destination; return 0 on success, -1 on error, -2 on write collision
    virtual int openDestFile(uint64_t startSize)=0;
    /// \brief close any open source/destination
    virtual void closeFiles()=0;
    /// \brief run the async read/write pipeline source->destination
    virtual void doTransferPipeline()=0;
    virtual bool remainSourceOpen() const=0;
    virtual bool remainDestinationOpen() const=0;
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
    bool bufferEnabled;

    bool sended_state_readStopped;
    bool readIsClosedVariable;
    bool writeIsClosedVariable;
    bool readIsOpenVariable;
    bool writeIsOpenVariable;
    bool realMove;
    /// \brief did the destination PRE-EXIST with non-empty content before we touched it? Captured in
    /// preOperation() on the first leg only (resumeFromOffset==0; a resume prefix is OURS, not the
    /// user's). Backs destinationIsOursToRemove() for the #25 corrupt-dest cleanup.
    bool destinationPreExisted=false;

    volatile bool putInPause;

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
