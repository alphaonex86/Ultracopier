/** \file TransferThreadUring.h
\brief io_uring backend for the pipelined file transfer (Linux only)
\author alpha_one_x86
\licence GPL3, see the file COPYING

Implements only the I/O hooks of TransferThreadPipelined with a Linux io_uring ring;
all transfer logic lives in the shared base class. */

#ifndef TRANSFERTHREADURING_H
#define TRANSFERTHREADURING_H

// The io_uring backend requires liburing at build time (and a Linux 5.1+ kernel at run time).
// ULTRACOPIER_PLUGIN_IO_URING is defined ONLY when liburing is installed, by ultracopier.pro /
// CopyEngine.pro (linux:packagesExist(liburing)); without liburing the define is absent, the
// .pro selects the async (thread-based) backend, and this file is not compiled. The build
// gracefully falls back rather than failing.
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
        enum State { Free, Reading, Writing } state;
    };
    PipelineBuffer pipelineBuffers[NUM_BUFFERS];
    /// \brief buffers whose read/write was still owned by the kernel when the bounded cancel-drain gave up
    /// (dead sector, stalled NFS/USB): never reused nor freed while the worker lives -- freed in the dtor
    /// after wait(), when the ring is gone. Bounded: at most NUM_BUFFERS per stuck-I/O event.
    std::vector<char*> orphanedBuffers;
    /// \brief per-file generation carried in every read/write user_data: a late CQE of an orphaned op
    /// from a PREVIOUS file is recognised and dropped instead of being credited to the current file's
    /// buffer of the same index. 32 bits: an 8-bit one wrapped every 256 files, i.e. a few SECONDS of
    /// small files -- well within the 30-120 s a dying disk can hold a wedged read.
    uint32_t fileGeneration;

    struct io_uring ring;
    bool ringInitialized;

    int sourceFd;
    int destFd;
    uint64_t sourceFileSize;
    int64_t readOffset;

    // Tag types for CQE user_data to identify completions
    // High 4 bits = op type; for READ/WRITE bits 8..39 = file generation, bits 0..7 = buffer index
    static constexpr uint64_t OP_READ_TAG  = 0x1000000000000000ULL;
    static constexpr uint64_t OP_WRITE_TAG = 0x2000000000000000ULL;
    static constexpr uint64_t OP_OPEN_TAG  = 0x4000000000000000ULL;
    static constexpr uint64_t OP_CLOSE_TAG = 0x5000000000000000ULL;
    static constexpr uint64_t OP_CANCEL_TAG= 0x6000000000000000ULL;
    static constexpr uint64_t OP_MASK      = 0xF000000000000000ULL;
    static constexpr uint64_t IDX_MASK     = 0x0FFFFFFFFFFFFFFFULL;
    static constexpr uint64_t BUF_MASK     = 0xFFULL;
    static constexpr unsigned GEN_SHIFT    = 8;
    static constexpr uint64_t GEN_MASK     = 0xFFFFFFFFULL;
    uint64_t dataTag(uint64_t op,int bufIdx) const { return op|((uint64_t)fileGeneration<<GEN_SHIFT)|(uint64_t)bufIdx; }

    // TransferThreadPipelined I/O hooks
    int openSourceFile() override;
    int openDestFile(uint64_t startSize) override;
    void closeFiles() override;
    void doTransferPipeline() override;
    /// \brief Bug C: the io_uring worker POLLS stopIt itself (doTransferPipeline's completion wait is
    /// bounded via io_uring_wait_cqe_timeout, not INFINITE), so stop()/skip() must NOT submit a close SQE
    /// on THIS ring from the scheduler thread to wake it -- io_uring's SQ ring is single-producer, so a
    /// cross-thread io_uring_get_sqe+io_uring_submit races the worker. No-op here; the worker notices
    /// stopIt within one poll interval and closes its own fds.
    void interruptTransferForStop() override {}
    bool remainSourceOpen() const override;
    bool remainDestinationOpen() const override;
    void trimDestinationToContiguous() override;
    /// \brief futimens() on the still-open destFd using the cached source times (butime),
    /// avoiding the per-file reopen-to-utime() in doFilePostOperation.
    bool applyDateTimeOnOpenDestination() override;
    bool trySymlinkCopy() override;

    void initPipelineBuffers();
    void freePipelineBuffers();
};

#endif // TRANSFERTHREADURING_H
