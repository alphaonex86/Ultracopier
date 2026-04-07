/** \file TransferThreadUring.h
\brief Thread using io_uring for async pipelined file transfer (Linux only)
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef TRANSFERTHREADURING_H
#define TRANSFERTHREADURING_H

#include <QObject>
#include <QMutex>
#include <QSemaphore>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>

#include <liburing.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//before the next define
#include "../CopyEngineUltracopier-SpecVariable.h"

#include <utime.h>
#include <time.h>

#include "../TransferThread.h"
#include "../Environment.h"
#include "../DriveManagement.h"
#include "../StructEnumDefinition_CopyEngine.h"

/// \brief Thread using io_uring for full async pipelined file transfer
class TransferThreadUring : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadUring();
    ~TransferThreadUring();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief get the reading letter (io_uring has no separate read thread)
    char readingLetter() const;
    /// \brief get the writing letter (io_uring has no separate write thread)
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

    // Write collision tracking (replaces writeThread.writeFileList)
    QMultiHash<QString, TransferThreadUring *> *writeFileList;
    QMutex *writeFileListMutex;
protected:
    void run();
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
private:
    // io_uring pipeline
    static constexpr unsigned int RING_DEPTH = 64;
    static constexpr int NUM_BUFFERS = 8;

    struct PipelineBuffer {
        char *data;
        unsigned int allocSize;
        int bytesUsed;      // bytes read into this buffer
        enum State { Free, Reading, WriteReady, Writing } state;
    };
    PipelineBuffer pipelineBuffers[NUM_BUFFERS];

    struct io_uring ring;
    bool ringInitialized;

    int sourceFd;
    int destFd;
    uint64_t sourceFileSize;
    int64_t readOffset;
    int64_t writeOffset;
    uint64_t transferProgression;
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

    // Tag types for CQE user_data to identify completions
    // High 4 bits = op type, low 60 bits = buffer index
    static constexpr uint64_t OP_READ_TAG  = 0x1000000000000000ULL;
    static constexpr uint64_t OP_WRITE_TAG = 0x2000000000000000ULL;
    static constexpr uint64_t OP_MASK      = 0xF000000000000000ULL;
    static constexpr uint64_t IDX_MASK     = 0x0FFFFFFFFFFFFFFFULL;

    int openSourceFile();
    int openDestFile(uint64_t startSize);
    void closeFiles();
    void doTransferPipeline();
    void initPipelineBuffers();
    void freePipelineBuffers();

    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
    void checkIfAllIsClosedAndDoOperations();

    std::string errorString_internal;

    int64_t size_at_open;
    uint64_t mtime_at_open;
    INTERNALTYPEPATH sourceFile;
    INTERNALTYPEPATH destFile;
};

#endif // TRANSFERTHREADURING_H
