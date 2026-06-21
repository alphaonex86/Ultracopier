/** \file TransferThreadPipelined.cpp
\brief Shared lifecycle for the pipelined copy backends (io_uring and Windows IOCP)
\author alpha_one_x86
\licence GPL3, see the file COPYING

This base class holds every part of a pipelined transfer that does NOT depend on the
async I/O mechanism: the transfer state machine, collision / move / symlink handling,
pause/resume, progress accounting and all the signals/slots. The two backends
(TransferThreadUring, TransferThreadWin) derive from it and implement only the I/O
hooks: openSourceFile(), openDestFile(), closeFiles(), doTransferPipeline(),
remainSourceOpen(), remainDestinationOpen(). Keep transfer LOGIC here so a change
applies to both backends at once instead of being updated in one and forgotten in
the other. */

#include "TransferThreadPipelined.h"
#include <string>
#include <cstring>
#include <cerrno>
#include <vector>
#include <cstdio>
#include <iostream>

#include "xxhash.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "../../../../cpp11addition.h"

TransferThreadPipelined::TransferThreadPipelined() :
    transferProgression(0)
{
    haveStartTime=false;
    native_copy=false;
    checksumProgression=0;
    writeFileList=nullptr;
    writeFileListMutex=nullptr;
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
    /* putInPause was historically the only member left uninitialized; an
       uninitialized non-zero value made transfer threads block forever on the pause
       semaphore (no resume() ever comes) -> the copy hung after a handful of files.
       Always start un-paused. */
    putInPause=false;

    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    multiForBigSpeed=0;
    #endif
}

void TransferThreadPipelined::connectInternalSignals()
{
    /* Called by each backend ctor AFTER the backend-specific members/buffers are
       initialised and BEFORE start(). The connects only reference base-class
       slots/signals, so the wiring lives here once for both backends. TransferThread::run()
       performs the base setup and must run before the queued connections, exactly as
       in the original single-class constructor. */
    TransferThread::run();
    if(!connect(this,&TransferThreadPipelined::internalStartPostOperation,this,&TransferThreadPipelined::doFilePostOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalStartPreOperation,this,&TransferThreadPipelined::preOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalStartPostOperation,this,&TransferThreadPipelined::postOperation,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalTryStartTheTransfer,this,&TransferThreadPipelined::internalStartTheTransfer,Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThreadPipelined::setFileExistsActionSend,this,&TransferThreadPipelined::setFileExistsActionInternal,Qt::QueuedConnection))
        abort();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&driveManagement,&DriveManagement::debugInformation,this,&TransferThreadPipelined::debugInformation,Qt::QueuedConnection))
        abort();
    #endif
}

void TransferThreadPipelined::startTheTransfer()
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

void TransferThreadPipelined::internalStartTheTransfer()
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

bool TransferThreadPipelined::setFiles(const INTERNALTYPEPATH& source, const int64_t &size, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode &mode)
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
    checksumProgression=0;
    sended_state_readStopped=false;
    readIsClosedVariable=false;
    writeIsClosedVariable=false;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    realMove=false;
    return true;
}

void TransferThreadPipelined::resetExtraVariable()
{
    transferProgression=0;
    checksumProgression=0;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    TransferThread::resetExtraVariable();
}

bool TransferThreadPipelined::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

void TransferThreadPipelined::preOperation()
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

    #ifdef Q_OS_WIN32
    // a Windows symlink/junction is recreated (not followed) by trySymlinkCopy, so don't
    // read/apply the (followed) target's date here. Mirrors the async backend.
    doTheDateTransfer=!is_symlink(source);
    #else
    doTheDateTransfer=true;
    #endif
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

void TransferThreadPipelined::postOperation()
{
    doFilePostOperation();
}

#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
void TransferThreadPipelined::setProgression(const uint64_t &pos, const uint64_t &size)
{
    if(transfer_stat==TransferStat_Transfer)
    {
        if(pos==size)
            emit readStopped();
        transferProgression=pos;
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+
                             "] transfer_stat:"+std::to_string(transfer_stat)+
                             ", canStartTransfer: "+std::to_string(canStartTransfer)+", transfert id: "+std::to_string(transferId));
}
#endif

#ifdef Q_OS_WIN32
static DWORD CALLBACK pipelinedProgressRoutine(
  LARGE_INTEGER TotalFileSize,LARGE_INTEGER TotalBytesTransferred,LARGE_INTEGER StreamSize,
  LARGE_INTEGER StreamBytesTransferred,DWORD dwStreamNumber,DWORD dwCallbackReason,
  HANDLE hSourceFile,HANDLE hDestinationFile,LPVOID lpData)
{
    (void)TotalFileSize;(void)StreamSize;(void)StreamBytesTransferred;(void)dwStreamNumber;
    (void)dwCallbackReason;(void)hSourceFile;(void)hDestinationFile;
    static_cast<TransferThreadPipelined *>(lpData)->setProgression(TotalBytesTransferred.QuadPart,TotalFileSize.QuadPart);
    return PROGRESS_CONTINUE;
}
#endif

bool TransferThreadPipelined::tryNativeCopy()
{
    // native_copy option: copy via the OS instead of the read/write pipeline. Mirrors the
    // async backend: CopyFileExW on Windows, copy_file_range on Linux. Returns false when
    // native_copy is off or the OS path is unsupported (then the caller uses the pipeline).
    if(!native_copy)
        return false;
    bool successFull=false;
    #ifdef Q_OS_WIN32
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,internalStringTostring(source)+" to "+internalStringTostring(destination)+": native_copy enabled");
    successFull=CopyFileExW(TransferThread::toFinalPath(source).c_str(),TransferThread::toFinalPath(destination).c_str(),
        (LPPROGRESS_ROUTINE)pipelinedProgressRoutine,this,&stopItWin,COPY_FILE_ALLOW_DECRYPTED_DESTINATION | 0x00000800);//0x00000800 is COPY_FILE_COPY_SYMLINK
    if(successFull==FALSE)
    {
        const std::string &strError=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] native copy stop in error: "+strError);
        readError=true;
        writeError=true;
        emit errorOnFile(destination,strError);
    }
    #elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(__ANDROID__)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,internalStringTostring(source)+" to "+internalStringTostring(destination)+": native_copy enabled (copy_file_range)");
    {
        const std::string srcPath=TransferThread::internalStringTostring(source);
        const std::string dstPath=TransferThread::internalStringTostring(destination);
        int src_fd=::open(srcPath.c_str(),O_RDONLY);
        if(src_fd<0)
        {
            const int terr=errno;
            const std::string &strError=strerror(terr);
            readError=true;
            writeError=false;
            emit errorOnFile(source,strError);
            return true;
        }
        struct stat src_stat;
        if(fstat(src_fd,&src_stat)<0)
        {
            const int terr=errno;
            const std::string &strError=strerror(terr);
            ::close(src_fd);
            readError=true;
            writeError=false;
            emit errorOnFile(source,strError);
            return true;
        }
        int dst_fd=::open(dstPath.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0755);
        if(dst_fd<0)
        {
            const int terr=errno;
            const std::string &strError=strerror(terr);
            ::close(src_fd);
            readError=false;
            writeError=true;
            emit errorOnFile(destination,strError);
            return true;
        }
        const uint64_t total=(uint64_t)src_stat.st_size;
        uint64_t copied=0;
        transferProgression=0;
        while(copied<total)
        {
            if(stopIt || needSkip)
            {
                ::close(src_fd);
                ::close(dst_fd);
                if(stopIt && needRemove)
                    unlink(destination);
                resetExtraVariable();
                return true;
            }
            size_t to_copy=(size_t)(total-copied);
            if(to_copy>(size_t)(1<<20))
                to_copy=(size_t)(1<<20);
            ssize_t r=copy_file_range(src_fd,NULL,dst_fd,NULL,to_copy,0);
            if(r<0)
            {
                const int terr=errno;
                ::close(src_fd);
                ::close(dst_fd);
                //EXDEV/ENOSYS/EINVAL: cross-device or unsupported -> fall back to the pipeline
                if(terr==EXDEV || terr==ENOSYS || terr==EINVAL)
                {
                    unlink(destination);
                    return false;
                }
                readError=false;
                writeError=true;
                emit errorOnFile(destination,strerror(terr));
                return true;
            }
            if(r==0)
                break;//EOF before total
            copied+=(uint64_t)r;
            setProgression(copied,total);
        }
        ::close(src_fd);
        if(::close(dst_fd)<0)
        {
            const int terr=errno;
            readError=false;
            writeError=true;
            emit errorOnFile(destination,strerror(terr));
            return true;
        }
        transferProgression=total;//final progression (e.g. zero-byte file)
        successFull=true;
    }
    #else
    return false;//no native copy on this platform
    #endif

    if(!successFull)
    {
        if(stopIt)
        {
            if(!source.empty())
                if(exists(source) && source!=destination)
                    unlink(destination);
            resetExtraVariable();
        }
        // error already emitted above
        return true;
    }
    readIsClosedVariable=true;
    writeIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
    return true;
}

void TransferThreadPipelined::ifCanStartTransfer()
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
    if(destinationIndex!=INTERNALTYPEPATH::npos && destinationIndex<destination.size())
    {
        const INTERNALTYPEPATH &path=destination.substr(0,destinationIndex);
        if(!is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+TransferThread::internalStringTostring(path));
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
            // fall through to pipeline copy
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
        // Symlink handling is backend-specific: the POSIX backend recreates the link,
        // the Windows backend follows it and copies the target. The hook returns true
        // when it fully handled a symlinked source (success or error already emitted);
        // false means "not a symlink / follow semantics" -> do the normal pipelined
        // copy below through the backend I/O hooks.
        if(trySymlinkCopy())
            return;

        // native_copy option (CopyFileExW / copy_file_range); falls through to the
        // pipeline below when the option is off or the OS path is unsupported.
        if(tryNativeCopy())
            return;

        if(openSourceFile()<0)
        {
            readError=true;
            emit errorOnFile(source,errorString_internal);
            return;
        }
        readIsOpenVariable=true;

        const int destRet=openDestFile(0);
        if(destRet<0)
        {
            if(destRet==-2) // collision, not an error
                return;
            writeError=true;
            closeFiles(); // closes the already-open source handle/fd
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

        /* Set the source date/time on the destination while its handle/fd is STILL OPEN,
           before closeFiles(). This avoids the per-file reopen-just-to-set-times that
           doFilePostOperation() would otherwise do (one fewer open+close per file = one
           fewer Defender handle-close scan on Windows). Only on success: a failed/stopped
           transfer keeps doTheDateTransfer's normal handling. When the source is a symlink,
           doTheDateTransfer was already cleared in preOperation() (Windows) and the symlink
           is handled by trySymlinkCopy() (never reaching here), so the existing guard holds.
           On failure here we leave dateAppliedOnOpenHandle false, so doFilePostOperation
           falls back to the reopen path exactly as before. */
        if(doTheDateTransfer && !readError && !writeError && !stopIt && !needSkip)
            if(applyDateTimeOnOpenDestination())
                dateAppliedOnOpenHandle=true;

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

bool TransferThreadPipelined::trySymlinkCopy()
{
    // Default: not handled here. Backends that recreate symlinks (POSIX) override
    // this; backends that follow symlinks (Windows IOCP) keep the default so the
    // source is opened and copied as a regular file.
    return false;
}

bool TransferThreadPipelined::applyDateTimeOnOpenDestination()
{
    // Default: not handled -> doFilePostOperation() reopens the destination to set the
    // times, exactly as before. Each backend overrides this to set the times on its
    // still-open destination handle/fd.
    return false;
}

bool TransferThreadPipelined::runChecksumVerify()
{
    XXH3_state_t *srcState = XXH3_createState();
    XXH3_state_t *dstState = XXH3_createState();
    if(srcState==NULL || dstState==NULL)
    {
        if(srcState!=NULL)
            XXH3_freeState(srcState);
        if(dstState!=NULL)
            XXH3_freeState(dstState);
        emit errorOnFile(destination,tr("Checksum: unable to allocate xxh3 state").toStdString());
        return false;
    }
    XXH3_64bits_reset(srcState);
    XXH3_64bits_reset(dstState);

    bool readFailure=false;
    bool sizeMismatch=false;

    #ifdef Q_OS_WIN32
    HANDLE srcH=CreateFileW(TransferThread::toFinalPath(source).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(srcH==INVALID_HANDLE_VALUE)
    {
        XXH3_freeState(srcState);
        XXH3_freeState(dstState);
        emit errorOnFile(source,tr("Checksum: unable to re-open source: ").toStdString()+TransferThread::GetLastErrorStdStr());
        return false;
    }
    HANDLE dstH=CreateFileW(TransferThread::toFinalPath(destination).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(dstH==INVALID_HANDLE_VALUE)
    {
        CloseHandle(srcH);
        XXH3_freeState(srcState);
        XXH3_freeState(dstState);
        emit errorOnFile(destination,tr("Checksum: unable to re-open destination: ").toStdString()+TransferThread::GetLastErrorStdStr());
        return false;
    }
    #else
    int srcFd=::open(TransferThread::internalStringTostring(source).c_str(),O_RDONLY);
    if(srcFd<0)
    {
        XXH3_freeState(srcState);
        XXH3_freeState(dstState);
        emit errorOnFile(source,std::string(tr("Checksum: unable to re-open source: ").toStdString())+strerror(errno));
        return false;
    }
    int dstFd=::open(TransferThread::internalStringTostring(destination).c_str(),O_RDONLY);
    if(dstFd<0)
    {
        ::close(srcFd);
        XXH3_freeState(srcState);
        XXH3_freeState(dstState);
        emit errorOnFile(destination,std::string(tr("Checksum: unable to re-open destination: ").toStdString())+strerror(errno));
        return false;
    }
    #endif

    const size_t chunk=1*1024*1024;
    std::vector<char> srcBuf(chunk),dstBuf(chunk);
    while(!stopIt && !needSkip)
    {
        #ifdef Q_OS_WIN32
        DWORD srcRead=0,dstRead=0;
        if(!ReadFile(srcH,srcBuf.data(),(DWORD)chunk,&srcRead,NULL))
        {
            readFailure=true;
            break;
        }
        if(!ReadFile(dstH,dstBuf.data(),(DWORD)chunk,&dstRead,NULL))
        {
            readFailure=true;
            break;
        }
        #else
        ssize_t srcRead=::read(srcFd,srcBuf.data(),chunk);
        if(srcRead<0)
        {
            readFailure=true;
            break;
        }
        ssize_t dstRead=::read(dstFd,dstBuf.data(),chunk);
        if(dstRead<0)
        {
            readFailure=true;
            break;
        }
        #endif
        if(srcRead!=dstRead)
        {
            sizeMismatch=true;
            break;
        }
        if(srcRead==0)
            break;
        XXH3_64bits_update(srcState,srcBuf.data(),(size_t)srcRead);
        XXH3_64bits_update(dstState,dstBuf.data(),(size_t)dstRead);
        checksumProgression+=(uint64_t)srcRead;
    }

    XXH64_hash_t srcDigest=XXH3_64bits_digest(srcState);
    XXH64_hash_t dstDigest=XXH3_64bits_digest(dstState);
    XXH3_freeState(srcState);
    XXH3_freeState(dstState);
    #ifdef Q_OS_WIN32
    CloseHandle(srcH);
    CloseHandle(dstH);
    #else
    ::close(srcFd);
    ::close(dstFd);
    #endif

    if(stopIt || needSkip)
        return false;
    if(readFailure)
    {
        emit errorOnFile(destination,tr("Checksum: read error during verify").toStdString());
        return false;
    }
    if(sizeMismatch || srcDigest!=dstDigest)
    {
        char hashStr[64];
        std::snprintf(hashStr,sizeof(hashStr),"0x%016llx vs 0x%016llx",
                      (unsigned long long)srcDigest,(unsigned long long)dstDigest);
        emit errorOnFile(destination,tr("Checksum mismatch (xxh3-64): ").toStdString()+hashStr);
        return false;
    }
    return true;
}

void TransferThreadPipelined::checkIfAllIsClosedAndDoOperations()
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

    // Re-open source and destination read-only (after both handles were closed and the
    // OS write cache flushed) and verify both files xxh3-64 hash. Gated by transferChecksum,
    // snapshotted at setFiles() so the option cannot flip mid-transfer. On mismatch abort
    // BEFORE the Move source-removal below, so the source survives. Mirrors the async backend.
    if(transferChecksum && !readError && !writeError && !needSkip && !stopIt
            && !realMove && size>0)
    {
        transfer_stat=TransferStat_Checksum;
        emit pushStat(transfer_stat,transferId);
        const bool checksumOk=runChecksumVerify();
        if(!stopIt && !needSkip && !checksumOk)
        {
            fileContentError=true;
            transfer_stat=TransferStat_PostTransfer;
            return;
        }
    }

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

void TransferThreadPipelined::stop()
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

void TransferThreadPipelined::skip()
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

int64_t TransferThreadPipelined::copiedSize()
{
    // When the checksum option was snapshotted on at setFiles(), the per-file budget is
    // split in two: the copy phase fills the first half, the post-copy xxh3 verify fills
    // the second half (same 2.5G/5G/7.5G/10G progress curve as the async backend).
    int64_t copyPos=0;
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
    case TransferStat_Checksum:
        copyPos=transferProgression;
        break;
    case TransferStat_PostOperation:
        copyPos=transferSize;
        break;
    default:
        return 0;
    }
    if(transferChecksum)
        return (int64_t)(copyPos/2 + checksumProgression/2);
    return copyPos;
}

void TransferThreadPipelined::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
void TransferThreadPipelined::setId(int id)
{
    TransferThread::setId(id);
}

char TransferThreadPipelined::readingLetter() const
{
    // no separate read thread in the pipelined backends; report from the transfer stat
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

char TransferThreadPipelined::writingLetter() const
{
    // no separate write thread in the pipelined backends; report from the transfer stat
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

uint64_t TransferThreadPipelined::realByteTransfered() const
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

std::pair<uint64_t, uint64_t> TransferThreadPipelined::progression() const
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

void TransferThreadPipelined::setFileExistsAction(const FileExistsAction &action)
{
    emit setFileExistsActionSend(action);
}

void TransferThreadPipelined::setFileExistsActionInternal(const FileExistsAction &action)
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

void TransferThreadPipelined::retryAfterError()
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
bool TransferThreadPipelined::setBlockSize(const unsigned int blockSize)
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

void TransferThreadPipelined::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    this->multiForBigSpeed=multiForBigSpeed;
    waitNewClockForSpeed.release();
}

void TransferThreadPipelined::timeOfTheBlockCopyFinished()
{
    if(waitNewClockForSpeed.available()<=1)
        waitNewClockForSpeed.release();
}

void TransferThreadPipelined::setOsSpecFlags(bool os_spec_flags)
{
    this->os_spec_flags=os_spec_flags;
}

void TransferThreadPipelined::setNativeCopy(bool native_copy)
{
    this->native_copy=native_copy;
}
#endif

void TransferThreadPipelined::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try put in pause");
    if(stopIt)
        return;
    pauseMutex.tryAcquire(pauseMutex.available());
    putInPause=true;
}

void TransferThreadPipelined::resume()
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

void TransferThreadPipelined::setBuffer(const bool buffer)
{
    this->bufferEnabled=buffer;
}
