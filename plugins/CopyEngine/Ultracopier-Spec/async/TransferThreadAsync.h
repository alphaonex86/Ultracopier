/** \file TransferThreadAsync.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>

//defore the next define
#include "../CopyEngineUltracopier-SpecVariable.h"

#include "ReadThread.h"
#include "WriteThread.h"

#ifdef Q_OS_UNIX
    #include <utime.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif
#ifdef Q_OS_WIN32
    #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #include <utime.h>
        #include <time.h>
        #include <unistd.h>
        #include <sys/stat.h>
    #endif
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "../TransferThread.h"
#include "../Environment.h"
#include "../DriveManagement.h"
#include "../StructEnumDefinition_CopyEngine.h"

#ifndef TransferThreadAsync_H
#define TransferThreadAsync_H

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThreadAsync : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadAsync();
    ~TransferThreadAsync();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief get the reading letter
    char readingLetter() const;
    /// \brief get the writing letter
    char writingLetter() const;
    #endif

    //not copied size, ...
    uint64_t realByteTransfered() const;
    std::pair<uint64_t, uint64_t> progression() const;
    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    //set block size in Bytes for speed limitation
    bool setBlockSize(const unsigned int blockSize);
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    void timeOfTheBlockCopyFinished();
    void setOsSpecFlags(bool os_spec_flags);
    void setNativeCopy(bool native_copy);
    #endif
    /// \brief pause the copy
    void pause();
    /// \brief resume the copy
    void resume();

    bool haveStartTime;
    ReadThread readThread;
    WriteThread writeThread;
    bool native_copy;
    void setBuffer(const bool buffer);

    #ifdef Q_OS_WIN32
    static bool mkJunction(LPCWSTR szJunction, LPCWSTR szPath);
    #endif
protected:
    void run();
private slots:
    void preOperation();
    void postOperation();
    //force into the right thread
    void internalStartTheTransfer();

    //implemente to connect async
    void read_error();
    void read_readIsStopped();
    void read_closed();
    void write_error();
    void write_closed();
    void read_opened();
    void write_opened();
signals:
    //internal signal
    // DEAD in async: declared for a resume-after-error-and-seek that async never implemented
    // (async restarts faulted files from 0). Removed so it stops implying async resumes; the
    // working version lives in the io_uring/IOCP pipelined backends. See ReadThread.h note.
    //void internalStartResumeAfterErrorAndSeek() const;
    void internalStartPostOperation() const;
    //async due to tread conflict on from, if(from>=0) {do something, abort() -> on abort from =-1}
    void openRead(const INTERNALTYPEPATH &file, const Ultracopier::CopyMode &mode);
    //async due to tread conflict on to, if(to>=0) {do something, abort() -> on abort to =-1}
    void openWrite(const INTERNALTYPEPATH &file,const uint64_t &startSize);

    void setFileExistsActionSend(const FileExistsAction &action);
public slots:
    /// \brief to start the transfer of data
    void startTheTransfer();
    /// \brief stop the copy
    void stop();
    /// \brief skip the copy
    void skip();
    /// \brief true once the current/last attempt is fully finalized (postOperationStopped sent =
    /// read/write threads drained). The list thread must NOT reuse a thread (setFiles) until this is
    /// true; together with keeping transferId set across a put-to-end, it serialises reuse strictly
    /// after the old attempt's close so a stale close can't drop the requeued file.
    bool readyForReuse() const { return sended_state_postOperationStopped; }
    /// \brief return info about the copied size
    int64_t copiedSize();
    /// \brief put the current file at bottom
    void putAtBottom();
    /// \brief to set files to transfer
    bool setFiles(const INTERNALTYPEPATH &source, const int64_t &size, const INTERNALTYPEPATH &destination, const Ultracopier::CopyMode &mode);
    /// \brief to set file exists action to do
    void setFileExistsAction(const FileExistsAction &action);
    #if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
    void setProgression(const uint64_t &pos,const uint64_t &size);
    #endif

    //eror management
    /// \brief retry after error
    void retryAfterError();
private:
    void setFileExistsActionInternal(const FileExistsAction &action);
    //ready = open + ready to operation (no error to resolv)
    bool			transferIsReadyVariable;
    uint64_t transferProgression;
    // Bytes of source already fed to the post-copy xxh3 verifier. Combined with
    // transferProgression to drive the halved progress curve when checksum is on.
    uint64_t checksumProgression;
    bool sended_state_readStopped;
    // Emit postOperationStopped() at most once per transfer attempt. skip() emits it AND so does
    // checkIfAllIsClosedAndDoOperations() when the (skip-)stopped read/write threads close; on the
    // put-to-end-after-read-error path both fire, double-decrementing the list thread's
    // numberOfInodeOperation (it went negative) and mishandling the requeued entry -> a permanent
    // retry stall. Reset per attempt in setFiles(); guards the three emit sites.
    bool sended_state_postOperationStopped;
    bool readIsClosedVariable;
    bool writeIsClosedVariable;
    bool readIsOpenVariable;
    bool writeIsOpenVariable;
    bool realMove;
    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
    void checkIfAllIsClosedAndDoOperations();
    /// Re-open source and destination read-only and xxh3-64 both. Drives
    /// checksumProgression and reacts to stopIt/needSkip. Returns true on match.
    bool runChecksumVerify();
};

#endif // TransferThreadAsync_H
