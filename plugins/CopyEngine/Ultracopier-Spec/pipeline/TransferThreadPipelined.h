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

    // ---- Shared state -----------------------------------------------------------
    uint64_t transferProgression;
    uint64_t checksumProgression; // bytes hashed so far during the post-copy verify

    unsigned int blockSize;
    bool os_spec_flags;
    bool bufferEnabled;

    bool sended_state_readStopped;
    bool readIsClosedVariable;
    bool writeIsClosedVariable;
    bool readIsOpenVariable;
    bool writeIsOpenVariable;
    bool realMove;

    volatile bool putInPause;

    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QSemaphore waitNewClockForSpeed;
    volatile int multiForBigSpeed;
    #endif
    QSemaphore pauseMutex;

    std::string errorString_internal;

    int64_t size_at_open;
    uint64_t mtime_at_open;
    INTERNALTYPEPATH sourceFile;
    INTERNALTYPEPATH destFile;
};

#endif // TRANSFERTHREADPIPELINED_H
