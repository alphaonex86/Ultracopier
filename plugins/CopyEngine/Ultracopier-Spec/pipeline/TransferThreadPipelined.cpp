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
#include "ContiguousWatermark.h"
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
    mtime_nsec_at_open=0;
    ctime_at_open=0;
    contiguousWrittenOffset=0;
    resumeFromOffset=0;
    completedWriteExtents.clear();
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
    // media-reconnect RESUME: the retry path emits internalStartResumeAfterErrorAndSeek instead of
    // restarting from 0 when a SOURCE read error MAY be resumable; the slot makes the size/mtime/ctime
    // decision on the transfer thread (queued), then re-opens via preOperation.
    if(!connect(this,&TransferThreadPipelined::internalStartResumeAfterErrorAndSeek,this,&TransferThreadPipelined::resumeAfterErrorAndSeek,Qt::QueuedConnection))
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
    // A NEW file always starts fresh: a stale resumeFromOffset here would make openDestFile
    // keep/trim a just-created destination to a previous file's offset (cross-file corruption).
    contiguousWrittenOffset=0;
    resumeFromOffset=0;
    completedWriteExtents.clear();
    return true;
}

void TransferThreadPipelined::resetExtraVariable()
{
    transferProgression=0;
    checksumProgression=0;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    // The contiguous water-mark and its pending extents belong to a single transfer attempt;
    // clear them here. resumeFromOffset is DELIBERATELY NOT cleared: resumeAfterErrorAndSeek()
    // sets it just before this runs on the resume leg, and every restart path zeroes it itself.
    contiguousWrittenOffset=0;
    completedWriteExtents.clear();
    TransferThread::resetExtraVariable();
}

void TransferThreadPipelined::recordCompletedWrite(uint64_t off, uint64_t len)
{
    // Algorithm lives in pipeline/ContiguousWatermark.h so it can be unit-tested directly
    // (test/unit/watermark_test.cpp). completedWriteExtents is bounded by the in-flight buffer pool.
    uc_foldCompletedWrite(contiguousWrittenOffset,completedWriteExtents,off,len);
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

    // #9/#25 invariant: record whether the destination PRE-EXISTED with content BEFORE any
    // truncate/overwrite, on the first leg only (resumeFromOffset>0 means the existing dest is OUR
    // resume prefix, not the user's file). destinationIsOursToRemove() reads this so the #25
    // corrupt-dest cleanup never deletes a user's pre-existing destination.
    if(resumeFromOffset==0)
        destinationPreExisted = is_file(destination) && TransferThread::file_stat_size(destination)>0;

    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same");
        return;
    }

    // On a media-reconnect RESUME (resumeFromOffset>0) the existing destination is OUR partial
    // prefix -- we wrote [0,resumeFromOffset) and will keep+append to it via openDestFile. It must
    // NOT be treated as a collision and overwritten/unlinked here, or the prefix is destroyed and
    // openDestFile's keep-prefix guard then forces a restart from 0 (defeating the resume).
    if(resumeFromOffset==0 && destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists");
        return;
    }
    // Data-loss guard (mirrors the async fix): a FAILED rename of the existing destination to its
    // backup (checkAlwaysRename, fileCollision=RENAME + renameTheOriginalDestination=true) sets
    // readError, but destinationExists() returns false whenever readError is set -- so without this
    // guard preOperation continues and OVERWRITES the original destination, whose backup was never
    // made because the rename failed -> the user's only copy is lost. errorOnFile() already fired in
    // checkAlwaysRename(); return so the error policy keeps the original intact.
    if(readError)
        return;

    // A symlink/junction is recreated verbatim (readlink+symlink / trySymlinkCopy), not followed,
    // so its target's date must NOT be read here: readSourceFileDateTime/stat FOLLOWS the link, and a
    // dangling/escaping/absolute target makes it fail -> errorOnFile -> the entry (and a sibling caught
    // in the error storm) get dropped. utime() can't set a symlink's own time anyway. Skip on EVERY OS
    // (Windows already did this; the UNIX io_uring / IOCP-source branch was missing the guard).
    doTheDateTransfer=!is_symlink(source);
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
    // Mirror the date guard above: never transfer metadata through a symlink. readSourceFilePermissions
    // (stat) FOLLOWS the link and writeDestinationFilePermissions' chmod/chown then hit the TARGET, not the
    // link (no-op re-apply at best, spurious post-op failure on a dangling/un-chmod-able dest target).
    if(doRightTransfer)
        havePermission = !is_symlink(source) && readSourceFilePermissions(source);

    transfer_stat=TransferStat_WaitForTheTransfer;
    ifCanStartTransfer();
    emit preOperationStopped();
}

bool TransferThreadPipelined::destinationIsOursToRemove() const
{
    // OURS iff the destination did NOT pre-exist with content (we created it, or it was empty).
    return !destinationPreExisted;
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
        // #9: a fresh OVERWRITE of a user's pre-existing destination must NOT truncate it up-front --
        // a read/write error partway through copy_file_range would otherwise have already destroyed the
        // OLD data. Open WITHOUT O_TRUNC for a pre-existing dest and ftruncate to the bytes actually
        // written on SUCCESS instead (mirrors the async WriteThread #9 fix). A resume leg keeps O_TRUNC
        // (the existing dest is our own partial prefix to discard); but the native path never resumes.
        const int trunc_flag=(destinationPreExisted && resumeFromOffset==0)?0:O_TRUNC;
        int dst_fd=::open(dstPath.c_str(),O_WRONLY|O_CREAT|trunc_flag,0755);
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
                // #9: on cancel, only remove the destination if it is OURS (never a user's pre-existing
                // dest). Mirrors the skip-path guard. (NB: the native O_TRUNC above already destroyed a
                // pre-existing dest -- the truncate-skip for the native path is separate follow-up work.)
                if(stopIt && needRemove && destinationIsOursToRemove())
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
        // #9: now that the full new content is written, trim any leftover OLD tail (no-op when O_TRUNC
        // was used or when the new file is >= the old size). Only on SUCCESS -- a mid-copy failure keeps
        // the OLD bytes so the error policy can skip/retry without having destroyed the user's data.
        if(ftruncate(dst_fd,(off_t)copied)<0)
        {
            const int terr=errno;
            ::close(dst_fd);
            readError=false;
            writeError=true;
            emit errorOnFile(destination,strerror(terr));
            return true;
        }
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
            if(!source.empty() && destinationIsOursToRemove())   // #9: keep a user's pre-existing dest on cancel
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

        // resumeFromOffset is 0 for a normal/first transfer (-> openDestFile creates a fresh
        // destination, byte-for-byte the previous behaviour) and >0 only on a media-reconnect
        // RESUME, where it equals the verified contiguousWrittenOffset and the backend keeps the
        // destination prefix [0,resumeFromOffset) and trims any out-of-order tail above it.
        const int destRet=openDestFile(resumeFromOffset);
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
            if(!source.empty() && needRemove && destinationIsOursToRemove())   // #9: keep a user's pre-existing dest on cancel
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
            if(!source.empty() && destinationIsOursToRemove())   // #9: keep a user's pre-existing dest on cancel
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
    // #9 source-vanish guard: only remove the destination if it is OURS (we created it / it was empty).
    // A user's PRE-EXISTING non-empty dest (an OVERWRITE whose source vanished before the first byte)
    // must NOT be deleted -- cp/rsync leave it untouched. (iouring_source_vanish)
    if(!source.empty() && needRemove && (stopIt || needSkip) && destinationIsOursToRemove())
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
            // Remove the content-corrupt destination so a full-size WRONG file can't be mistaken for a
            // complete copy (gated by destinationIsOursToRemove() -- never delete a user's pre-existing
            // dest; a MOVE keeps the source via the copyFailed gate below). Mirrors TransferThreadAsync.
            if(!source.empty() && destinationIsOursToRemove() && source!=destination)
                unlink(destination);
            transfer_stat=TransferStat_PostTransfer;
            return;
        }
    }

    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
    transfer_stat=TransferStat_PostOperation;
    emit pushStat(transfer_stat,transferId);
    // Apply destination date/permissions BEFORE removing the source on a MOVE (mirrors async). With
    // keepDate=true a failed date-set is escalated (doFilePostOperation returns false); in the old
    // order the source was unlink()'d first, so that failure destroyed the user's only copy. Run the
    // post-op first and treat a critical failure like copyFailed -> keep the source for retry.
    const bool postOpOk = doFilePostOperation();

    if(mode==Ultracopier::Move && !realMove)
    {
        // CRITICAL: only remove the source if the copy actually succeeded AND the post-operation
        // (date/permissions) succeeded. A skip / stop / read / write / checksum error OR a critical
        // post-op failure means the move did not fully complete; removing the source would silently
        // lose user data (move = copy + delete; if any step failed, the delete must NOT happen).
        const bool copyFailed = stopIt || needSkip || readError || writeError || fileContentError || !postOpOk;
        if(!copyFailed)
        {
            if(exists(destination) && is_file(source) && source!=destination)
            {
                if(!unlink(source))
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move and unable to remove: "+
                                             TransferThread::internalStringTostring(source)+" "+strerror(errno));
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move: destination missing or source==destination, refusing to remove source");
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move: copy failed (stopIt/needSkip/readError/writeError/fileContentError/postOpFailed), keeping source");
    }

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
        // #9 source-vanish guard (mirrors line ~778): on a skip, only remove the destination if it is
        // OURS. A user's pre-existing non-empty dest (an OVERWRITE whose source vanished before the first
        // byte -- exists(source) is a stat that still succeeds) must NOT be deleted. (iouring_source_vanish)
        if(!source.empty() && needRemove && destinationIsOursToRemove())
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
    // Transfer (data-streaming) error. Snapshot the error flags BEFORE anything clears them:
    // a DEST write error sets writeError; an open()/OOM failure sets BOTH read+write (see ~l.327).
    // Resume-at-offset is safe ONLY for a PURE SOURCE read error -- any write/open error means the
    // destination prefix cannot be trusted, so we must restart from 0 (the user's spec: dest always
    // resumes from 0). resumeAfterErrorAndSeek() then makes the size/mtime/ctime identity decision.
    const bool wasWriteError=writeError;
    const bool wasReadError=readError;
    closeFiles();
    if(wasWriteError || !wasReadError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retryAfterError, restart from 0 (write/open error)");
        resumeFromOffset=0;
        resetExtraVariable();
        transfer_stat=TransferStat_PreOperation;
        emit internalStartPreOperation();
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retryAfterError, evaluate media-reconnect resume");
    emit internalStartResumeAfterErrorAndSeek();
}

void TransferThreadPipelined::resumeAfterErrorAndSeek()
{
    // Decide RESUME-at-offset vs RESTART-from-0. Resume ONLY when the source is byte-identical to
    // the version we started copying -- size AND full-precision mtime (sec+sub-second) AND ctime
    // all unchanged -- and a non-empty contiguous prefix is already on the destination, within the
    // current source size. Any uncertainty (re-stat failed, any field differs, sub-second mtime
    // unavailable, no prefix) RESTARTS from 0. This is the safe side: restart is always correct.
    readError=false;
    writeError=false;
    uint64_t resume=0;
    int64_t curSize=-1;
    uint64_t curMtime=0,curMtimeNsec=0,curCtime=0;
    bool statOk=false;
    #ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if(GetFileAttributesExW(source.c_str(),GetFileExInfoStandard,&fad)!=0)
    {
        ULARGE_INTEGER sz; sz.HighPart=fad.nFileSizeHigh; sz.LowPart=fad.nFileSizeLow;
        curSize=(int64_t)sz.QuadPart;
        ULARGE_INTEGER mt; mt.LowPart=fad.ftLastWriteTime.dwLowDateTime; mt.HighPart=fad.ftLastWriteTime.dwHighDateTime;
        // FILETIME is 100ns ticks since 1601; keep FULL precision: seconds + 100ns remainder.
        uint64_t unixTicks=(mt.QuadPart>=116444736000000000ULL)?(mt.QuadPart-116444736000000000ULL):0;
        curMtime=unixTicks/10000000ULL;
        curMtimeNsec=unixTicks%10000000ULL;   // 100ns units -> mtime is effectively 100ns-precise on Windows
        curCtime=0;                            // Windows WIN32_FILE_ATTRIBUTE_DATA has no POSIX ctime
        statOk=true;
    }
    #else
    struct stat st;
    if(::stat(source.c_str(),&st)==0)
    {
        curSize=st.st_size;
        curMtime=(uint64_t)st.st_mtim.tv_sec;
        curMtimeNsec=(uint64_t)st.st_mtim.tv_nsec;
        curCtime=(uint64_t)st.st_ctim.tv_sec;   // ctime changes on any in-place content/metadata write
        statOk=true;
    }
    #endif
    if(statOk
       && curSize==size_at_open
       && curMtime==mtime_at_open
       && curMtimeNsec==mtime_nsec_at_open
       && mtime_nsec_at_open!=0                        // require sub-second precision; 1s-only is too coarse to trust
       && curCtime==ctime_at_open
       && contiguousWrittenOffset>0
       && (int64_t)contiguousWrittenOffset<size_at_open)
    {
        resume=contiguousWrittenOffset;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] media-reconnect RESUME at "+std::to_string(resume)+"/"+std::to_string(size_at_open));
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resume rejected -> restart from 0 (statOk="+std::to_string(statOk)+", contiguous="+std::to_string(contiguousWrittenOffset)+")");
    resumeFromOffset=resume;
    resetExtraVariable();   // clears the water-mark/extents + transferProgression but PRESERVES resumeFromOffset
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
