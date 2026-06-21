/** \file TransferThreadUring.h
\brief io_uring backend for the pipelined file transfer (Linux only)
\author alpha_one_x86
\licence GPL3, see the file COPYING

Implements only the I/O hooks of TransferThreadPipelined with a Linux io_uring ring;
all transfer logic lives in the shared base class. */

#ifndef TRANSFERTHREADURING_H
#define TRANSFERTHREADURING_H

#include <liburing.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../pipeline/TransferThreadPipelined.h"

/// \brief io_uring implementation of the pipelined transfer I/O hooks
class TransferThreadUring : public TransferThreadPipelined
{
    // Q_OBJECT is required even though the subclass adds no new signals/slots:
    // ListThread does qobject_cast<TransferThreadImpl*>(sender()), which needs the
    // concrete type to carry its own meta-object.
    Q_OBJECT
public:
    explicit TransferThreadUring();
    ~TransferThreadUring();
protected:
    void run() override;
private:
    // io_uring pipeline
    static constexpr unsigned int RING_DEPTH = 64;
    static constexpr int NUM_BUFFERS = 8;

    struct PipelineBuffer {
        char *data;
        unsigned int allocSize;
        int bytesUsed;      // bytes currently held in this buffer (valid data length)
        int64_t fileOffset; // source/destination offset this buffer's data belongs to
        unsigned int chunkSize; // intended size of the chunk assigned to this buffer
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

    // Tag types for CQE user_data to identify completions
    // High 4 bits = op type, low 60 bits = buffer index
    static constexpr uint64_t OP_READ_TAG  = 0x1000000000000000ULL;
    static constexpr uint64_t OP_WRITE_TAG = 0x2000000000000000ULL;
    static constexpr uint64_t OP_FSYNC_TAG = 0x3000000000000000ULL;
    static constexpr uint64_t OP_OPEN_TAG  = 0x4000000000000000ULL;
    static constexpr uint64_t OP_CLOSE_TAG = 0x5000000000000000ULL;
    static constexpr uint64_t OP_MASK      = 0xF000000000000000ULL;
    static constexpr uint64_t IDX_MASK     = 0x0FFFFFFFFFFFFFFFULL;

    // TransferThreadPipelined I/O hooks
    int openSourceFile() override;
    int openDestFile(uint64_t startSize) override;
    void closeFiles() override;
    void doTransferPipeline() override;
    bool remainSourceOpen() const override;
    bool remainDestinationOpen() const override;
    /// \brief futimens() on the still-open destFd using the cached source times (butime),
    /// avoiding the per-file reopen-to-utime() in doFilePostOperation.
    bool applyDateTimeOnOpenDestination() override;
    bool trySymlinkCopy() override;

    void initPipelineBuffers();
    void freePipelineBuffers();
};

#endif // TRANSFERTHREADURING_H
