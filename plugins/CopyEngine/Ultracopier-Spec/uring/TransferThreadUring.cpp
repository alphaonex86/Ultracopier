/** \file TransferThreadUring.cpp
\brief Thread using io_uring for async pipelined file transfer (Linux only)
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "TransferThreadUring.h"
#include <string>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../../../cpp11addition.h"

TransferThreadUring::TransferThreadUring() :
    transferProgression(0)
{
    haveStartTime=false;
    native_copy=false;
    writeFileList=nullptr;
    writeFileListMutex=nullptr;
    ringInitialized=false;
    sourceFd=-1;
    destFd=-1;
    sourceFileSize=0;
    readOffset=0;
    writeOffset=0;
    blockSize=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    os_spec_flags=false;
    bufferEnabled=true;

    sended_state_readStopped=false;
    readIsClosedVariable=false;
    writeIsClosedVariable=false;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    realMove=false;
    size_at_open=0;
    mtime_at_open=0;

    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    multiForBigSpeed=0;
    #endif

    for(int i=0;i<NUM_BUFFERS;i++)
    {
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].state=PipelineBuffer::Free;
    }

    TransferThread::run();
    if(!connect(this,&TransferThreadUring::internalStartPostOperation,this,&TransferThreadUring::doFilePostOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalStartPreOperation,this,&TransferThreadUring::preOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalStartPostOperation,this,&TransferThreadUring::postOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalTryStartTheTransfer,this,&TransferThreadUring::internalStartTheTransfer,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThreadUring::setFileExistsActionSend,this,&TransferThreadUring::setFileExistsActionInternal,Qt::QueuedConnection))
        abort();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&driveManagement,&DriveManagement::debugInformation,this,&TransferThreadUring::debugInformation,Qt::QueuedConnection))
        abort();
    #endif

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
    freePipelineBuffers();
    exit(0);
    wait();
}

void TransferThreadUring::run()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));

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

    struct io_uring_cqe *cqe;
    int ret=io_uring_wait_cqe(&ring,&cqe);
    if(ret<0)
    {
        errorString_internal="io_uring_wait_cqe for openat source failed: "+std::string(strerror(-ret));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
        return -1;
    }
    int fd=cqe->res;
    io_uring_cqe_seen(&ring,cqe);
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
        #ifdef Q_OS_MAC
        mtime_at_open=st.st_mtimespec.tv_sec;
        #else
        mtime_at_open=st.st_mtim.tv_sec;
        #endif
        sourceFileSize=st.st_size;
    }
    else
    {
        sourceFileSize=0;
    }
    return fd;
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

    struct io_uring_cqe *cqe;
    int ret=io_uring_wait_cqe(&ring,&cqe);
    if(ret<0)
    {
        errorString_internal="io_uring_wait_cqe for openat dest failed: "+std::string(strerror(-ret));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
        return -1;
    }
    int fd=cqe->res;
    io_uring_cqe_seen(&ring,cqe);
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

    // Truncate/preallocate
    if(startSize>0)
    {
        if(ftruncate(fd,startSize)!=0)
        {
            int t=errno;
            errorString_internal=strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to truncate: "+errorString_internal);
            ::close(fd);
            return -1;
        }
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

    return fd;
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

    readOffset=0;
    writeOffset=0;
    transferProgression=0;

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
                    // Read completed, queue write
                    pipelineBuffers[bufIdx].bytesUsed=result;
                    pipelineBuffers[bufIdx].state=PipelineBuffer::Writing;

                    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
                    if(!sqe)
                    {
                        errorString_internal="io_uring_get_sqe failed for write";
                        errorOccurred=true;
                        writeError=true;
                        emit errorOnFile(destination,errorString_internal);
                        break;
                    }
                    io_uring_prep_write(sqe,destFd,pipelineBuffers[bufIdx].data,result,writeOffset);
                    io_uring_sqe_set_data64(sqe,OP_WRITE_TAG|(uint64_t)bufIdx);
                    writeOffset+=result;
                    writesInFlight++;
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
                // Handle short write
                if(result<pipelineBuffers[bufIdx].bytesUsed)
                {
                    int remaining=pipelineBuffers[bufIdx].bytesUsed-result;
                    memmove(pipelineBuffers[bufIdx].data,pipelineBuffers[bufIdx].data+result,remaining);
                    pipelineBuffers[bufIdx].bytesUsed=remaining;

                    struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
                    if(!sqe)
                    {
                        errorString_internal="io_uring_get_sqe failed for short write retry";
                        errorOccurred=true;
                        writeError=true;
                        emit errorOnFile(destination,errorString_internal);
                        break;
                    }
                    io_uring_prep_write(sqe,destFd,pipelineBuffers[bufIdx].data,remaining,writeOffset-remaining);
                    io_uring_sqe_set_data64(sqe,OP_WRITE_TAG|(uint64_t)bufIdx);
                    writesInFlight++;
                    needSubmit=true;
                    continue;
                }

                // Write completed fully
                transferProgression=writeOffset;
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
        // Async fsync via io_uring instead of blocking fsync()
        struct io_uring_sqe *sqe=io_uring_get_sqe(&ring);
        if(!sqe)
        {
            errorString_internal="io_uring_get_sqe failed for fsync";
            writeError=true;
            emit errorOnFile(destination,errorString_internal);
        }
        else
        {
            io_uring_prep_fsync(sqe,destFd,0);
            io_uring_sqe_set_data64(sqe,OP_FSYNC_TAG);
            io_uring_submit(&ring);

            struct io_uring_cqe *cqe;
            ret=io_uring_wait_cqe(&ring,&cqe);
            if(ret<0)
            {
                errorString_internal="io_uring_wait_cqe for fsync failed: "+std::string(strerror(-ret));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                writeError=true;
                emit errorOnFile(destination,errorString_internal);
            }
            else if(cqe->res<0)
            {
                errorString_internal="fsync error: "+std::string(strerror(-cqe->res));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                writeError=true;
                emit errorOnFile(destination,errorString_internal);
            }
            if(ret>=0)
                io_uring_cqe_seen(&ring,cqe);
        }
        transferProgression=sourceFileSize;
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline end, transferred: "+std::to_string(transferProgression));
}

void TransferThreadUring::startTheTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, transfert id: "+std::to_string(transferId));
    if(transferId==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] can't start transfert if transferId==0");
        return;
    }
    startTransferTime.restart();
    haveTransferTime=true;
    emit internalTryStartTheTransfer();
}

void TransferThreadUring::internalStartTheTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, transfert id: "+std::to_string(transferId)+
                             " readError: "+std::to_string(writeError)+" writeError: "+std::to_string(writeError)+" canStartTransfer: "+std::to_string(canStartTransfer));
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] can't start transfert at idle");
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] can't start transfert at PostOperation");
        return;
    }
    if(transfer_stat==TransferStat_Transfer && !readError && !writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] can't start transfert at Transfer (double start?)");
        return;
    }
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] canStartTransfer is already set to true");
    }
    canStartTransfer=true;
    ifCanStartTransfer();
}

bool TransferThreadUring::setFiles(const INTERNALTYPEPATH& source, const int64_t &size, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination));
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] already used, source: "+TransferThread::internalStringTostring(source)+
                                 ", destination: "+TransferThread::internalStringTostring(destination));
        return false;
    }
    if(!isRunning())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] The current thread is not running");
    if(!TransferThread::setFiles(source,size,destination,mode))
        return false;
    transferProgression=0;
    sended_state_readStopped=false;
    readIsClosedVariable=false;
    writeIsClosedVariable=false;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    realMove=false;
    return true;
}

void TransferThreadUring::resetExtraVariable()
{
    transferProgression=0;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    TransferThread::resetExtraVariable();
}

bool TransferThreadUring::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

bool TransferThreadUring::remainSourceOpen() const
{
    return sourceFd>=0;
}

bool TransferThreadUring::remainDestinationOpen() const
{
    return destFd>=0;
}

void TransferThreadUring::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] already used, source: "+
                                 TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination)+
                                 " transfer_stat: "+std::to_string(transfer_stat));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: source: "+
                             TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
    haveStartTime=true;
    needRemove=false;

    // Close any previously open files
    closeFiles();

    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same");
        return;
    }

    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists");
        return;
    }

    doTheDateTransfer=true;
    if(doTheDateTransfer)
    {
        doTheDateTransfer=readSourceFileDateTime(source);
        if(!doTheDateTransfer && keepDate)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"!doTheDateTransfer && keepDate");
            emit errorOnFile(source,tr("Wrong modification date or unable to get it, you can disable time transfer to do it").toStdString());
            return;
        }
    }
    if(doRightTransfer)
        havePermission=readSourceFilePermissions(source);

    transfer_stat=TransferStat_WaitForTheTransfer;
    ifCanStartTransfer();
    emit preOperationStopped();
}

void TransferThreadUring::postOperation()
{
    doFilePostOperation();
}

void TransferThreadUring::ifCanStartTransfer()
{
    realMove=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start "+internalStringTostring(source)+"->"+internalStringTostring(destination));
    if(transfer_stat!=TransferStat_WaitForTheTransfer || !canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+
                                 "] transfer_stat:"+std::to_string(transfer_stat)+
                                 ", canStartTransfer: "+std::to_string(canStartTransfer)+", transfert id: "+std::to_string(transferId));
        return;
    }
    transfer_stat=TransferStat_Transfer;

    // Check/create destination folder
    const size_t destinationIndex=destination.rfind('/');
    if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
    {
        const std::string &path=destination.substr(0,destinationIndex);
        if(!is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+path);
                emit errorOnFile(destination,tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString());
                return;
            }
    }

    emit pushStat(transfer_stat,transferId);
    realMove=(mode==Ultracopier::Move && driveManagement.isSameDrive(
                       internalStringTostring(source),
                       internalStringTostring(destination)
                       ));
    needRemove=true;
    bool successFull=false;
    readError=false;
    writeError=false;

    if(realMove)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start real move");
        if(TransferThread::exists(source))
            successFull=TransferThread::rename(source,destination);
        if(!successFull && errno==18)
        {
            realMove=false;
            // fall through to io_uring copy
        }
        else if(successFull)
        {
            readIsClosedVariable=true;
            writeIsClosedVariable=true;
            checkIfAllIsClosedAndDoOperations();
            return;
        }
    }

    if(!realMove)
    {
        if(TransferThread::is_symlink(source))
        {
            realMove=true;
            // Handle symlink
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
                        return;
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
                return;
            }
            else
            {
                if(symlink(buf.data(),TransferThread::internalStringTostring(destination).c_str())!=0)
                {
                    const int terr=errno;
                    readError=true;
                    emit errorOnFile(destination,strerror(terr));
                    return;
                }
            }
            successFull=true;
            readIsClosedVariable=true;
            writeIsClosedVariable=true;
            checkIfAllIsClosedAndDoOperations();
            return;
        }
        else
        {
            // io_uring pipeline copy
            sourceFd=openSourceFile();
            if(sourceFd<0)
            {
                readError=true;
                emit errorOnFile(source,errorString_internal);
                return;
            }
            readIsOpenVariable=true;

            destFd=openDestFile(0);
            if(destFd<0)
            {
                if(destFd==-2) // collision
                    return;
                writeError=true;
                ::close(sourceFd);
                sourceFd=-1;
                emit errorOnFile(destination,errorString_internal);
                return;
            }
            writeIsOpenVariable=true;

            // Do the pipeline transfer
            doTransferPipeline();

            // Remove write collision entry
            if(writeFileList!=nullptr && writeFileListMutex!=nullptr)
            {
                QMutexLocker lock_mutex(writeFileListMutex);
                #ifdef WIDESTRING
                QString qtFile=QString::fromStdWString(destination);
                #else
                QString qtFile=QString::fromStdString(destination);
                #endif
                writeFileList->remove(qtFile,this);
            }

            // Close files
            closeFiles();
            readIsClosedVariable=true;
            writeIsClosedVariable=true;
            readIsOpenVariable=false;
            writeIsOpenVariable=false;

            if(!readError && !writeError && !stopIt)
            {
                emit readStopped();
                checkIfAllIsClosedAndDoOperations();
                return;
            }
            else if(stopIt)
            {
                // Cleanup on stop
                if(!source.empty() && needRemove)
                    if(exists(source) && source!=destination)
                        unlink(destination);
                return;
            }
            // Error case: error already emitted
            return;
        }
    }

    if(!successFull)
    {
        const int terr=errno;
        const std::string &strError=strerror(terr);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                 TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                 " "+strError+"("+std::to_string(terr)+")");
        if(stopIt)
        {
            if(!source.empty())
                if(exists(source) && source!=destination)
                    unlink(destination);
            resetExtraVariable();
            return;
        }
        if(readError)
            emit errorOnFile(source,strError);
        else
            emit errorOnFile(destination,strError);
    }
}

void TransferThreadUring::checkIfAllIsClosedAndDoOperations()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop copy");
    if((readError || writeError) && !needSkip && !stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resolve error before progress");
        return;
    }
    if(remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] remainFileOpen()");
        return;
    }
    if(!source.empty() && needRemove && (stopIt || needSkip))
        if(is_file(source) && source!=destination)
            unlink(destination);

    transfer_stat=TransferStat_Idle;
    transferSize=transferProgression;

    if(mode==Ultracopier::Move && !realMove)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] remove mode==Ultracopier::Move && !realMove");
        if(exists(destination))
            if(!unlink(source))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move and unable to remove: "+
                                         TransferThread::internalStringTostring(source)+" "+strerror(errno));
    }
    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
    transfer_stat=TransferStat_PostOperation;
    emit pushStat(transfer_stat,transferId);
    doFilePostOperation();

    source.clear();
    destination.clear();
    resetExtraVariable();
    needRemove=false;
    needSkip=false;
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

void TransferThreadUring::stop()
{
    stopIt=true;
    haveStartTime=false;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] transfer_stat==TransferStat_Idle");
        return;
    }
    if(transfer_stat==TransferStat_PreOperation)
    {
        transfer_stat=TransferStat_Idle;
        return;
    }
    if(realMove)
    {
        if(readError || writeError)
            transfer_stat=TransferStat_Idle;
        return;
    }
    pauseMutex.release();
    closeFiles();
}

void TransferThreadUring::skip()
{
    stopIt=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with stat: "+std::to_string(transfer_stat));
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_WaitForTheTransfer:
    case TransferStat_PreOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        if(remainFileOpen())
            closeFiles();
        else
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        resetExtraVariable();
        break;
    case TransferStat_Transfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        if(realMove)
        {
            emit readStopped();
            emit postOperationStopped();
            transfer_stat=TransferStat_Idle;
            emit pushStat(transfer_stat,transferId);
            return;
        }
        pauseMutex.release();
        closeFiles();
        if(!source.empty() && needRemove)
            if(exists(source) && source!=destination)
                unlink(destination);
        break;
    case TransferStat_PostTransfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        closeFiles();
        break;
    case TransferStat_PostOperation:
        break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] can't skip in this state: "+std::to_string(transfer_stat));
        return;
    }
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

int64_t TransferThreadUring::copiedSize()
{
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
        return transferProgression;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

void TransferThreadUring::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
void TransferThreadUring::setId(int id)
{
    TransferThread::setId(id);
}

char TransferThreadUring::readingLetter() const
{
    // io_uring has no separate read thread, report based on overall transfer stat
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Idle:
        return '_';
    case TransferStat_PreOperation:
        return 'I';
    case TransferStat_WaitForTheTransfer:
        return 'W';
    case TransferStat_Transfer:
        return 'C';
    case TransferStat_PostTransfer:
    case TransferStat_PostOperation:
        return 'P';
    default:
        return '?';
    }
}

char TransferThreadUring::writingLetter() const
{
    // io_uring has no separate write thread, report based on overall transfer stat
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Idle:
        return '_';
    case TransferStat_PreOperation:
        return 'I';
    case TransferStat_WaitForTheTransfer:
        return 'W';
    case TransferStat_Transfer:
        return 'C';
    case TransferStat_PostTransfer:
    case TransferStat_PostOperation:
        return 'P';
    default:
        return '?';
    }
}
#endif

uint64_t TransferThreadUring::realByteTransfered() const
{
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
    case TransferStat_PostOperation:
        return transferProgression;
    default:
        return 0;
    }
}

std::pair<uint64_t, uint64_t> TransferThreadUring::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
        returnVar.first=transferProgression;
        returnVar.second=transferProgression;
        break;
    case TransferStat_PostTransfer:
    case TransferStat_PostOperation:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
        break;
    default:
        returnVar.first=0;
        returnVar.second=0;
    }
    return returnVar;
}

void TransferThreadUring::setFileExistsAction(const FileExistsAction &action)
{
    emit setFileExistsActionSend(action);
}

void TransferThreadUring::setFileExistsActionInternal(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] already used, source: "+
                                 TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
        return;
    }
    if(action!=FileExists_Rename)
        fileExistsAction=action;
    else
    {
        fileExistsAction=action;
        alwaysDoFileExistsAction=action;
    }
    if(action==FileExists_Skip)
    {
        skip();
        return;
    }
    resetExtraVariable();
    emit internalStartPreOperation();
}

void TransferThreadUring::retryAfterError()
{
    if(transfer_stat==TransferStat_Idle)
    {
        if(transferId==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] seam have bug");
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] restart all");
        readError=false;
        emit internalStartPreOperation();
        return;
    }
    if(transfer_stat==TransferStat_PreOperation)
    {
        readError=false;
        writeError=false;
        emit internalStartPreOperation();
        return;
    }
    // For transfer errors, restart from scratch
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retryAfterError, restart");
    closeFiles();
    resetExtraVariable();
    transfer_stat=TransferStat_PreOperation;
    emit internalStartPreOperation();
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
bool TransferThreadUring::setBlockSize(const unsigned int blockSize)
{
    if(blockSize>1 && blockSize<ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE*1024)
    {
        this->blockSize=blockSize;
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"block size out of range: "+std::to_string(blockSize));
        return false;
    }
}

void TransferThreadUring::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    this->multiForBigSpeed=multiForBigSpeed;
    waitNewClockForSpeed.release();
}

void TransferThreadUring::timeOfTheBlockCopyFinished()
{
    if(waitNewClockForSpeed.available()<=1)
        waitNewClockForSpeed.release();
}

void TransferThreadUring::setOsSpecFlags(bool os_spec_flags)
{
    this->os_spec_flags=os_spec_flags;
}

void TransferThreadUring::setNativeCopy(bool native_copy)
{
    this->native_copy=native_copy;
}
#endif

void TransferThreadUring::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try put in pause");
    if(stopIt)
        return;
    pauseMutex.tryAcquire(pauseMutex.available());
    putInPause=true;
}

void TransferThreadUring::resume()
{
    if(putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resume");
        putInPause=false;
        stopIt=false;
    }
    else
        return;
    pauseMutex.release();
}

void TransferThreadUring::setBuffer(const bool buffer)
{
    this->bufferEnabled=buffer;
}
