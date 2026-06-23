/** \file TransferThreadUring.cpp
\brief io_uring backend: implements the pipelined I/O hooks of TransferThreadPipelined
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "TransferThreadUring.h"
#include <string>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <vector>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef Q_OS_LINUX
#include <sys/resource.h>
#include <sys/syscall.h>
#endif

#include "../../../../cpp11addition.h"

TransferThreadUring::TransferThreadUring()
{
    ringInitialized=false;
    sourceFd=-1;
    destFd=-1;
    sourceFileSize=0;
    readOffset=0;
    writeOffset=0;

    for(int i=0;i<NUM_BUFFERS;i++)
    {
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].fileOffset=0;
        pipelineBuffers[i].chunkSize=0;
        pipelineBuffers[i].state=PipelineBuffer::Free;
    }

    // Shared signal/slot wiring + TransferThread::run() base setup, then launch the thread.
    connectInternalSignals();

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));
    start();
}

TransferThreadUring::~TransferThreadUring()
{
    stopIt=true;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    pauseMutex.release();
    closeFiles();
    exit(0);
    wait();
    // Free the pipeline buffers ONLY after the worker thread has fully stopped (wait()):
    // run() calls io_uring_queue_exit() right after exec() returns, which tears down the ring
    // and makes the kernel drop all references to our buffers. Freeing before that (the old
    // order) raced read/write SQEs still in flight on a cancel/stop mid-file — io_uring's close
    // op (unlike Win32 CloseHandle) does NOT cancel pending ops — i.e. a kernel use-after-free
    // of a just-freed buffer. After wait() the ring is gone, so this is safe.
    freePipelineBuffers();
}

void TransferThreadUring::run()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));

    // Lower this I/O worker thread below the GUI thread so, under CPU contention, the
    // Qt message pump preempts the copy work and the progress UI stays responsive.
    // setpriority(PRIO_PROCESS,tid,...) sets the nice value of THIS thread only (Linux
    // per-thread nice via the kernel tid from gettid()), not the whole process; +5 is a
    // moderate demotion mirroring Windows THREAD_PRIORITY_BELOW_NORMAL. Scheduling-only:
    // does not touch copy correctness. Only transfer threads are lowered; the inode /
    // ListThread scheduler keeps its normal priority so listing stays responsive.
    #ifdef Q_OS_LINUX
    setpriority(PRIO_PROCESS,(id_t)syscall(SYS_gettid),5);
    #endif

    // Initialize io_uring ring once in the worker thread, reuse across all file transfers
    int ret=io_uring_queue_init(RING_DEPTH,&ring,0);
    if(ret<0)
    {
        std::cerr << "io_uring_queue_init failed: " << strerror(-ret) << std::endl;
        abort();
    }
    ringInitialized=true;

    moveToThread(this);
    exec();

    // Destroy the ring in the same thread that created it
    if(ringInitialized)
    {
        io_uring_queue_exit(&ring);
        ringInitialized=false;
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop: "+std::to_string((int64_t)QThread::currentThreadId()));
}

void TransferThreadUring::initPipelineBuffers()
{
    for(int i=0;i<NUM_BUFFERS;i++)
    {
        if(pipelineBuffers[i].data==nullptr || pipelineBuffers[i].allocSize!=blockSize)
        {
            free(pipelineBuffers[i].data);
            pipelineBuffers[i].data=(char*)malloc(blockSize);
            if(pipelineBuffers[i].data==nullptr)
            {
                errorString_internal="Out of memory allocating pipeline buffers";
                emit errorOnFile(source,errorString_internal);
                return;
            }
            pipelineBuffers[i].allocSize=blockSize;
        }
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].state=PipelineBuffer::Free;
    }
}

void TransferThreadUring::freePipelineBuffers()
{
    for(int i=0;i<NUM_BUFFERS;i++)
    {
        free(pipelineBuffers[i].data);
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
    }
}

int TransferThreadUring::openSourceFile()
{
    const std::string sourcePath=TransferThread::internalStringTostring(source);
    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
    if(!sqe)
    {
        errorString_internal="io_uring_get_sqe failed for openat source";
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
        return -1;
    }
    io_uring_prep_openat(sqe,AT_FDCWD,sourcePath.c_str(),O_RDONLY,0);
    io_uring_sqe_set_data64(sqe,OP_OPEN_TAG);
    io_uring_submit(&ring);

    // Wait for THIS openat's completion specifically (user_data==OP_OPEN_TAG), discarding any stale
    // CQEs still in the ring from a previous errored+drained attempt of the SAME thread (on an
    // in-place FileError_Retry / media-reconnect resume). Taking the first CQE blindly mistook such
    // a stale result for the openat fd -> a bogus fd that EBADF'd every later fstat/ftruncate/lseek.
    struct io_uring_cqe *cqe;
    int fd=-1;
    while(true)
    {
        int ret=io_uring_wait_cqe(&ring,&cqe);
        if(ret<0)
        {
            errorString_internal="io_uring_wait_cqe for openat source failed: "+std::string(strerror(-ret));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            return -1;
        }
        const bool isOpenCqe=(io_uring_cqe_get_data64(cqe)==OP_OPEN_TAG);
        if(isOpenCqe)
            fd=cqe->res;
        io_uring_cqe_seen(&ring,cqe);
        if(isOpenCqe)
            break;
    }
    if(fd<0)
    {
        errorString_internal=strerror(-fd);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open source: "+
                                 sourcePath+", error: "+errorString_internal+" ("+std::to_string(-fd)+")");
        return -1;
    }
    if(os_spec_flags)
    {
        posix_fadvise(fd,0,0,POSIX_FADV_WILLNEED);
        posix_fadvise(fd,0,0,POSIX_FADV_SEQUENTIAL);
        posix_fadvise(fd,0,0,POSIX_FADV_NOREUSE);
    }
    struct stat st;
    if(fstat(fd,&st)==0)
    {
        size_at_open=st.st_size;
        // Capture FULL-precision mtime + ctime for the media-reconnect resume gate (see
        // TransferThreadPipelined::resumeAfterErrorAndSeek): sub-second mtime defeats the
        // same-size/same-whole-second in-place-rewrite splice, ctime catches in-place writes.
        #ifdef Q_OS_MAC
        mtime_at_open=st.st_mtimespec.tv_sec;
        mtime_nsec_at_open=(uint64_t)st.st_mtimespec.tv_nsec;
        ctime_at_open=(uint64_t)st.st_ctimespec.tv_sec;
        #else
        mtime_at_open=st.st_mtim.tv_sec;
        mtime_nsec_at_open=(uint64_t)st.st_mtim.tv_nsec;
        ctime_at_open=(uint64_t)st.st_ctim.tv_sec;
        #endif
        sourceFileSize=st.st_size;
    }
    else
    {
        sourceFileSize=0;
    }
    // Store internally and report success; the base class no longer captures the fd.
    sourceFd=fd;
    return 0;
}

int TransferThreadUring::openDestFile(uint64_t startSize)
{
    // Ensure destination directory exists
    const size_t destinationIndex=destination.rfind('/');
    if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
    {
        const std::string &path=destination.substr(0,destinationIndex);
        if(!TransferThread::is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                errorString_internal=tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+path);
                return -1;
            }
    }

    // Check write collision
    if(writeFileList!=nullptr && writeFileListMutex!=nullptr)
    {
        QMutexLocker lock_mutex(writeFileListMutex);
        #ifdef WIDESTRING
        QString qtFile=QString::fromStdWString(destination);
        #else
        QString qtFile=QString::fromStdString(destination);
        #endif
        if(writeFileList->count(qtFile,this)==0)
        {
            writeFileList->insert(qtFile,this);
            if(writeFileList->count(qtFile)>1)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] in waiting because same file is found");
                return -2; // collision, not an error
            }
        }
    }

    const std::string destPath=TransferThread::internalStringTostring(destination);
    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
    if(!sqe)
    {
        errorString_internal="io_uring_get_sqe failed for openat dest";
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
        return -1;
    }
    io_uring_prep_openat(sqe,AT_FDCWD,destPath.c_str(),O_WRONLY|O_CREAT,0755);
    io_uring_sqe_set_data64(sqe,OP_OPEN_TAG);
    io_uring_submit(&ring);

    // Wait for THIS openat's completion (user_data==OP_OPEN_TAG), discarding stale CQEs from a
    // previous errored+drained attempt (see openSourceFile for the full rationale).
    struct io_uring_cqe *cqe;
    int fd=-1;
    while(true)
    {
        int ret=io_uring_wait_cqe(&ring,&cqe);
        if(ret<0)
        {
            errorString_internal="io_uring_wait_cqe for openat dest failed: "+std::string(strerror(-ret));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            return -1;
        }
        const bool isOpenCqe=(io_uring_cqe_get_data64(cqe)==OP_OPEN_TAG);
        if(isOpenCqe)
            fd=cqe->res;
        io_uring_cqe_seen(&ring,cqe);
        if(isOpenCqe)
            break;
    }
    if(fd<0)
    {
        errorString_internal=strerror(-fd);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open dest: "+
                                 destPath+", error: "+errorString_internal+" ("+std::to_string(-fd)+")");
        return -1;
    }

    if(os_spec_flags)
    {
        posix_fadvise(fd,0,0,POSIX_FADV_NOREUSE);
        posix_fadvise(fd,0,0,POSIX_FADV_SEQUENTIAL);
    }

    // RESUME keep-prefix guard: startSize>0 only on a media-reconnect resume (the verified
    // contiguous prefix). The existing destination MUST already contain the whole prefix; if it is
    // shorter (deleted/truncated externally between attempts), the ftruncate below would EXTEND it
    // with zeros under the resume point -> corruption. Fail instead -> the retry restarts from 0.
    // Stat BY PATH, not the just-opened fd (the io_uring-opened dest fd returns EBADF on a plain
    // fstat() on the resume path; the file is on a normal FS).
    if(startSize>0)
    {
        struct stat dst;
        if(stat(destPath.c_str(),&dst)!=0 || (uint64_t)dst.st_size<startSize)
        {
            errorString_internal="resume prefix missing/short on destination (have "+
                std::to_string(stat(destPath.c_str(),&dst)==0?(long long)dst.st_size:-1)+", need "+std::to_string(startSize)+")";
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            ::close(fd);
            return -1;
        }
    }
    // Truncate the destination to startSize ALWAYS (mirrors async WriteThread::destTruncate):
    //   startSize==0 -> truncate to 0 = a FRESH file. This is what an OVERWRITE collision needs:
    //     destinationExists() returns false for Overwrite WITHOUT unlinking (TransferThread.cpp:348),
    //     leaving the stale dest for the backend to truncate. O_CREAT alone does NOT truncate, so
    //     before this an overwrite onto a LONGER pre-existing file kept a stale tail (a real io_uring
    //     overwrite bug; copy_override/move_override exposed it once stale test binaries were rebuilt).
    //   startSize>0 -> keep the prefix [0,startSize), trim any out-of-order tail above it (resume).
    if(ftruncate(fd,startSize)!=0)
    {
        int t=errno;
        errorString_internal=strerror(t);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to truncate: "+errorString_internal);
        ::close(fd);
        return -1;
    }
    if(startSize>0)
    {
        // Make the kept prefix durable before we trust+append to it (the resume contract). This
        // fsync is per-RESUME-event (rare), NOT per-file, so it does not reintroduce the per-file
        // fsync cost that was deliberately removed from the normal path.
        if(fdatasync(fd)!=0)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] resume fdatasync failed: "+std::string(strerror(errno)));
    }

    // Seek to 0
    if(lseek(fd,0,SEEK_SET)!=0)
    {
        int t=errno;
        errorString_internal=strerror(t);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to seek: "+errorString_internal);
        ::close(fd);
        return -1;
    }

    // Store internally and report success; the base class no longer captures the fd.
    destFd=fd;
    return 0;
}

void TransferThreadUring::closeFiles()
{
    if(sourceFd<0 && destFd<0)
        return;

    if(!ringInitialized)
    {
        // Fallback to blocking close if ring not available (e.g. during destruction before run())
        if(sourceFd>=0) { ::close(sourceFd); sourceFd=-1; }
        if(destFd>=0)   { ::close(destFd);   destFd=-1; }
        return;
    }

    int pending=0;
    if(sourceFd>=0)
    {
        struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
        if(sqe)
        {
            io_uring_prep_close(sqe,sourceFd);
            io_uring_sqe_set_data64(sqe,OP_CLOSE_TAG|0);
            pending++;
        }
        else
        {
            ::close(sourceFd);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] io_uring_get_sqe failed for close source, used blocking close");
        }
        sourceFd=-1;
    }
    if(destFd>=0)
    {
        struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
        if(sqe)
        {
            io_uring_prep_close(sqe,destFd);
            io_uring_sqe_set_data64(sqe,OP_CLOSE_TAG|1);
            pending++;
        }
        else
        {
            ::close(destFd);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] io_uring_get_sqe failed for close dest, used blocking close");
        }
        destFd=-1;
    }

    if(pending>0)
    {
        io_uring_submit(&ring);
        for(int i=0;i<pending;i++)
        {
            struct io_uring_cqe *cqe;
            int ret=io_uring_wait_cqe(&ring,&cqe);
            if(ret<0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] io_uring_wait_cqe for close failed: "+std::string(strerror(-ret)));
                continue;
            }
            if(cqe->res<0)
            {
                uint64_t idx=io_uring_cqe_get_data64(cqe)&IDX_MASK;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close "+(idx==0?"source":"dest")+": "+std::string(strerror(-cqe->res)));
            }
            io_uring_cqe_seen(&ring,cqe);
        }
    }
}

void TransferThreadUring::doTransferPipeline()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline start");

    int ret;

    // Initialize pipeline buffers
    initPipelineBuffers();
    for(int i=0;i<NUM_BUFFERS;i++)
        if(pipelineBuffers[i].data==nullptr)
        {
            closeFiles();
            return; // error already emitted
        }

    // Seed ALL offsets from the resume cursor together (0 for a normal/first transfer ->
    // byte-for-byte the old behaviour). On a media-reconnect resume, resumeFromOffset is the
    // verified contiguous prefix already on disk, so reads/writes continue from there and the
    // already-written prefix is not re-read. The initial read loop below starts at readOffset.
    readOffset=resumeFromOffset;
    writeOffset=resumeFromOffset;
    transferProgression=resumeFromOffset;
    contiguousWrittenOffset=resumeFromOffset;
    completedWriteExtents.clear();

    int readsInFlight=0;
    int writesInFlight=0;
    bool readDone=false;
    bool errorOccurred=false;

    // Submit initial batch of reads
    {
        int submitted=0;
        for(int i=0;i<NUM_BUFFERS && !readDone && !stopIt;i++)
        {
            if((uint64_t)readOffset>=sourceFileSize)
            {
                readDone=true;
                break;
            }
            unsigned int toRead=blockSize;
            if((uint64_t)(readOffset+toRead)>sourceFileSize)
                toRead=(unsigned int)(sourceFileSize-readOffset);

            struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
            if(!sqe)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] io_uring_get_sqe failed for initial read");
                break;
            }
            pipelineBuffers[i].fileOffset=readOffset;
            pipelineBuffers[i].chunkSize=toRead;
            pipelineBuffers[i].bytesUsed=0;
            io_uring_prep_read(sqe,sourceFd,pipelineBuffers[i].data,toRead,readOffset);
            io_uring_sqe_set_data64(sqe,OP_READ_TAG|(uint64_t)i);
            pipelineBuffers[i].state=PipelineBuffer::Reading;
            readOffset+=toRead;
            readsInFlight++;
            submitted++;
        }
        if(submitted>0)
            io_uring_submit(&ring);
    }

    // Main event loop — batch CQE processing + single submit per iteration
    struct io_uring_cqe *cqes[RING_DEPTH];
    while((readsInFlight>0 || writesInFlight>0) && !stopIt && !errorOccurred)
    {
        // Handle pause
        if(putInPause && !stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] pipeline paused");
            pauseMutex.acquire();
            if(stopIt)
                break;
        }

        // Wait for at least one completion
        {
            struct io_uring_cqe *cqe;
            ret=io_uring_wait_cqe(&ring,&cqe);
            if(ret<0)
            {
                if(ret==-EINTR)
                    continue;
                errorString_internal="io_uring_wait_cqe failed: "+std::string(strerror(-ret));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                readError=true;
                errorOccurred=true;
                break;
            }
        }

        // Drain all available completions in one batch
        unsigned int numCqes=io_uring_peek_batch_cqe(&ring,cqes,RING_DEPTH);
        bool needSubmit=false;

        for(unsigned int ci=0;ci<numCqes && !errorOccurred && !stopIt;ci++)
        {
            struct io_uring_cqe *cqe=cqes[ci];
            uint64_t userData=io_uring_cqe_get_data64(cqe);
            uint64_t opType=userData&OP_MASK;
            int bufIdx=(int)(userData&IDX_MASK);
            int result=cqe->res;

            if(opType==OP_READ_TAG)
            {
                readsInFlight--;
                if(result<0)
                {
                    errorString_internal="Read error: "+std::string(strerror(-result));
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                    readError=true;
                    errorOccurred=true;
                    emit errorOnFile(source,errorString_internal);
                    break;
                }
                if(result==0)
                {
                    // EOF
                    pipelineBuffers[bufIdx].state=PipelineBuffer::Free;
                    readDone=true;
                }
                else
                {
                    pipelineBuffers[bufIdx].bytesUsed+=result;

                    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
                    if(!sqe)
                    {
                        errorString_internal="io_uring_get_sqe failed for write";
                        errorOccurred=true;
                        writeError=true;
                        emit errorOnFile(destination,errorString_internal);
                        break;
                    }
                    if((unsigned int)pipelineBuffers[bufIdx].bytesUsed<pipelineBuffers[bufIdx].chunkSize)
                    {
                        // Short read: fetch the rest of this chunk into the same buffer
                        // before writing, so partial reads under load can't corrupt data.
                        const unsigned int rem=pipelineBuffers[bufIdx].chunkSize-(unsigned int)pipelineBuffers[bufIdx].bytesUsed;
                        io_uring_prep_read(sqe,sourceFd,
                                           pipelineBuffers[bufIdx].data+pipelineBuffers[bufIdx].bytesUsed,
                                           rem,
                                           pipelineBuffers[bufIdx].fileOffset+pipelineBuffers[bufIdx].bytesUsed);
                        io_uring_sqe_set_data64(sqe,OP_READ_TAG|(uint64_t)bufIdx);
                        pipelineBuffers[bufIdx].state=PipelineBuffer::Reading;
                        readsInFlight++;
                    }
                    else
                    {
                        // Chunk fully read -> write it back at the exact offset it came
                        // from (NOT a global write cursor): io_uring completions arrive
                        // out of order, so completion-order writing scrambled the file.
                        pipelineBuffers[bufIdx].state=PipelineBuffer::Writing;
                        io_uring_prep_write(sqe,destFd,pipelineBuffers[bufIdx].data,
                                            pipelineBuffers[bufIdx].bytesUsed,pipelineBuffers[bufIdx].fileOffset);
                        io_uring_sqe_set_data64(sqe,OP_WRITE_TAG|(uint64_t)bufIdx);
                        writesInFlight++;
                    }
                    needSubmit=true;
                }
            }
            else if(opType==OP_WRITE_TAG)
            {
                writesInFlight--;
                if(result<0)
                {
                    errorString_internal="Write error: "+std::string(strerror(-result));
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                    writeError=true;
                    errorOccurred=true;
                    emit errorOnFile(destination,errorString_internal);
                    break;
                }
                transferProgression+=result;
                // Fold this completed extent into the contiguous low-water mark. MUST use the LIVE
                // fileOffset+result (this completion's bytes) BEFORE the short-write advance below,
                // so out-of-order completions are merged correctly and contiguousWrittenOffset stays
                // the true written-from-0 prefix (the only safe resume point).
                recordCompletedWrite(pipelineBuffers[bufIdx].fileOffset,(uint64_t)result);
                // Handle short write
                if(result<pipelineBuffers[bufIdx].bytesUsed)
                {
                    int remaining=pipelineBuffers[bufIdx].bytesUsed-result;
                    memmove(pipelineBuffers[bufIdx].data,pipelineBuffers[bufIdx].data+result,remaining);
                    pipelineBuffers[bufIdx].bytesUsed=remaining;
                    pipelineBuffers[bufIdx].fileOffset+=result;

                    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
                    if(!sqe)
                    {
                        errorString_internal="io_uring_get_sqe failed for short write retry";
                        errorOccurred=true;
                        writeError=true;
                        emit errorOnFile(destination,errorString_internal);
                        break;
                    }
                    io_uring_prep_write(sqe,destFd,pipelineBuffers[bufIdx].data,remaining,pipelineBuffers[bufIdx].fileOffset);
                    io_uring_sqe_set_data64(sqe,OP_WRITE_TAG|(uint64_t)bufIdx);
                    writesInFlight++;
                    needSubmit=true;
                    continue;
                }

                // Write completed fully -> reuse this buffer for the next chunk
                pipelineBuffers[bufIdx].state=PipelineBuffer::Free;

                // Queue next read if data remains
                if(!readDone && !stopIt && (uint64_t)readOffset<sourceFileSize)
                {
                    unsigned int toRead=blockSize;
                    if((uint64_t)(readOffset+toRead)>sourceFileSize)
                        toRead=(unsigned int)(sourceFileSize-readOffset);

                    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
                    if(sqe)
                    {
                        pipelineBuffers[bufIdx].fileOffset=readOffset;
                        pipelineBuffers[bufIdx].chunkSize=toRead;
                        pipelineBuffers[bufIdx].bytesUsed=0;
                        io_uring_prep_read(sqe,sourceFd,pipelineBuffers[bufIdx].data,toRead,readOffset);
                        io_uring_sqe_set_data64(sqe,OP_READ_TAG|(uint64_t)bufIdx);
                        pipelineBuffers[bufIdx].state=PipelineBuffer::Reading;
                        readOffset+=toRead;
                        readsInFlight++;
                        needSubmit=true;
                    }
                }
                else if((uint64_t)readOffset>=sourceFileSize)
                {
                    readDone=true;
                }
            }
        }

        // Mark all processed CQEs as seen in one go
        io_uring_cq_advance(&ring,numCqes);

        // Single submit for all queued SQEs this iteration
        if(needSubmit)
            io_uring_submit(&ring);
    }

    if(!errorOccurred && !stopIt)
    {
        /* No per-file fsync. A blocking fsync per file forced a disk-durability
           flush for every file (~35 ms/file on a spinning/slow volume -> ~1 hour
           of pure fsync wait for 100k files); this was THE dominant "much slower
           than rsync" cost, and it is wall-clock/latency, not CPU or I/O
           bandwidth. Neither rsync nor the async backend fsync each file: the
           written data stays in the page cache and the kernel flushes it
           normally. A file copy does not need per-file durability. */
        transferProgression=sourceFileSize;
        // Whole file is contiguously written: the resume water-mark equals the file size. (On the
        // ERROR path we deliberately leave contiguousWrittenOffset at its true mid-file value so the
        // resume decision in resumeAfterErrorAndSeek() reads the real safe offset.)
        contiguousWrittenOffset=(uint64_t)sourceFileSize;
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline end, transferred: "+std::to_string(transferProgression));
}
bool TransferThreadUring::remainSourceOpen() const
{
    return sourceFd>=0;
}

bool TransferThreadUring::remainDestinationOpen() const
{
    return destFd>=0;
}

bool TransferThreadUring::applyDateTimeOnOpenDestination()
{
    // Set the source times (cached into butime by readSourceFileDateTime in preOperation)
    // on the destination while its fd is STILL OPEN, so doFilePostOperation does not reopen
    // the file just to utime() it. futimens on the open fd is the fd analog of the path
    // utime() that writeDestinationFileDateTime() uses; same seconds precision (butime is
    // a struct utimbuf in whole seconds), so behaviour is unchanged.
    if(destFd<0)
        return false;
    struct timespec ts[2];
    ts[0].tv_sec=butime.actime;  ts[0].tv_nsec=0; // atime
    ts[1].tv_sec=butime.modtime; ts[1].tv_nsec=0; // mtime
    if(futimens(destFd,ts)!=0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] futimens on open dest fd failed: "+std::string(strerror(errno)));
        return false; // fall back to the reopen path in doFilePostOperation
    }
    return true;
}


bool TransferThreadUring::trySymlinkCopy()
{
    // POSIX semantics: a symlinked source is recreated as a symlink at the destination
    // (cp -a / rsync -a behaviour), not followed. Return true for any symlink source
    // (whether the recreation succeeded or an error was emitted); false otherwise.
    if(!TransferThread::is_symlink(source))
        return false;
    realMove=true;
    bool isFileOrSymlink=false;
    {
        struct stat p_statbuf;
        if(lstat(TransferThread::internalStringTostring(destination).c_str(),&p_statbuf)>=0)
        {
            if(S_ISLNK(p_statbuf.st_mode) || S_ISREG(p_statbuf.st_mode))
                isFileOrSymlink=true;
        }
    }
    if(isFileOrSymlink)
        if(!unlink(destination))
        {
            const int terr=errno;
            if(terr!=2)
            {
                const std::string &strError=strerror(terr);
                writeError=true;
                emit errorOnFile(destination,strError);
                return true;
            }
        }
    std::vector<char> buf(512);
    ssize_t s=0;
    do {
        buf.resize(buf.size()*2);
        s=readlink(TransferThread::internalStringTostring(source).c_str(),buf.data(),buf.size());
    } while(s==(ssize_t)buf.size());
    if(s!=-1)
    {
        buf.resize(s+1);
        buf[s]='\0';
    }
    if(s<0)
    {
        const int terr=errno;
        readError=true;
        emit errorOnFile(source,strerror(terr));
        return true;
    }
    else
    {
        if(symlink(buf.data(),TransferThread::internalStringTostring(destination).c_str())!=0)
        {
            const int terr=errno;
            readError=true;
            emit errorOnFile(destination,strerror(terr));
            return true;
        }
    }
    readIsClosedVariable=true;
    writeIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
    return true;
}
