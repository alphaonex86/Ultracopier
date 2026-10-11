/** \file TransferThreadWin.cpp
\brief Windows IOCP backend: implements the pipelined I/O hooks of TransferThreadPipelined
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "TransferThreadWin.h"

#ifdef ULTRACOPIER_PLUGIN_WINIOCP

#include <string>
#include <cstring>
#include <cerrno>
#include <iostream>

#include <windows.h>
#include <winioctl.h>

#include "../../../../cpp11addition.h"

// Reparse-point buffer (not exposed by windows.h; same definition the async backend uses).
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8
typedef struct _IOCP_REPARSE_DATA_BUFFER {
  ULONG  ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG  Flags;
      WCHAR  PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR  PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} IOCP_REPARSE_DATA_BUFFER, *PIOCP_REPARSE_DATA_BUFFER;

/* The Windows port mirrors the io_uring backend one-for-one:
   - io_uring ring                 -> I/O completion port (HANDLE iocp)
   - io_uring_prep_read/write      -> ReadFile/WriteFile with a per-buffer OVERLAPPED
   - sqe user_data tag             -> completion key (source=read, dest=write) + the
                                      returned OVERLAPPED* mapped back to its buffer
   - io_uring_submit               -> implicit (each ReadFile/WriteFile queues itself)
   - io_uring_wait/peek_batch_cqe  -> GetQueuedCompletionStatusEx
   The per-buffer fileOffset / short-read / short-write correctness handling and the
   "no per-file flush" decision are kept identical to the io_uring version. */

// Native wide path for CreateFileW (\\?\-prefixed). On Windows WIDESTRING is set so
// INTERNALTYPEPATH is std::wstring and toFinalPath() already returns a wide path.
static std::wstring iocpFinalPath(const INTERNALTYPEPATH &p)
{
    #ifdef WIDESTRING
    return TransferThread::toFinalPath(p);
    #else
    const std::string n=TransferThread::toFinalPath(p);
    if(n.empty())
        return std::wstring();
    const int len=MultiByteToWideChar(CP_UTF8,0,n.c_str(),(int)n.size(),nullptr,0);
    std::wstring w(len,L'\0');
    MultiByteToWideChar(CP_UTF8,0,n.c_str(),(int)n.size(),&w[0],len);
    return w;
    #endif
}

// Format a Windows error code like strerror() does for errno, for debug messages.
static std::string winErrorString(DWORD err)
{
    char *msg=nullptr;
    const DWORD n=FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|
                                 FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,err,
                                 MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPSTR)&msg,0,nullptr);
    std::string s;
    if(n>0 && msg!=nullptr)
    {
        s.assign(msg,n);
        while(!s.empty() && (s.back()=='\n' || s.back()=='\r' || s.back()==' '))
            s.pop_back();
    }
    else
        s="error "+std::to_string((unsigned long)err);
    if(msg!=nullptr)
        LocalFree(msg);
    return s;
}

TransferThreadWin::TransferThreadWin()
{
    iocp=nullptr;
    iocpInitialized=false;
    sourceHandle=INVALID_HANDLE_VALUE;
    destHandle=INVALID_HANDLE_VALUE;
    sourceFileSize=0;
    readOffset=0;

    for(int i=0;i<NUM_BUFFERS;i++)
    {
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].fileOffset=0;
        pipelineBuffers[i].chunkSize=0;
        pipelineBuffers[i].ov=nullptr;
        pipelineBuffers[i].state=PipelineBuffer::Free;
    }

    // Shared signal/slot wiring + TransferThread::run() base setup, then launch the thread.
    connectInternalSignals();

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));
    start();
}

TransferThreadWin::~TransferThreadWin()
{
    stopIt=true;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    pauseMutex.release();
    closeFiles();
    exit(0);
    wait();
    // Free the pipeline buffers only after the worker thread has fully stopped (wait()):
    // run() closes the completion port after exec() returns, so no queued overlapped
    // completion can still reference a buffer once the worker is done. (closeFiles()'s
    // CloseHandle already cancels pending I/O, so this is belt-and-suspenders here, but it
    // keeps the teardown symmetric with the io_uring backend and is strictly safer.)
    freePipelineBuffers();
    // Free any QUARANTINED buffers last: their wedged IRPs (if any) are guaranteed gone now that the
    // worker has stopped and every completion port it used is closed, so nothing can DMA into them.
    for(char *p : orphanedBuffers)
        free(p);
    orphanedBuffers.clear();
    for(OVERLAPPED *o : orphanedOverlapped)
        free(o);
    orphanedOverlapped.clear();
}

void TransferThreadWin::run()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));

    // Lower the I/O worker thread below the GUI thread so, under CPU contention
    // (e.g. Defender scanning every copied file), the Qt message pump preempts the
    // copy work and the progress UI stays responsive. Scheduling-only: does not
    // touch copy correctness. Only the transfer threads are lowered; the inode /
    // ListThread scheduler keeps its normal priority so listing stays responsive.
    #ifdef ULTRACOPIER_PLUGIN_WINIOCP
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    #endif

    // Create one I/O completion port up front and reuse it across all file transfers.
    // Source and destination handles are associated to it as they are opened.
    iocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE,nullptr,0,0);
    if(iocp==nullptr)
    {
        std::cerr << "CreateIoCompletionPort failed: " << winErrorString(GetLastError()) << std::endl;
        abort();
    }
    iocpInitialized=true;

    moveToThread(this);
    exec();

    // Destroy the completion port in the same thread that created it
    if(iocpInitialized)
    {
        CloseHandle(iocp);
        iocp=nullptr;
        iocpInitialized=false;
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop: "+std::to_string((int64_t)QThread::currentThreadId()));
}

void TransferThreadWin::initPipelineBuffers()
{
    // Snapshot blockSize ONCE. It can be changed from ANOTHER thread (the speed limiter / setBlockSize,
    // TransferThreadPipelined::setBlockSize, which writes it without a lock) between the malloc() and the
    // allocSize store below; reading it twice could record allocSize LARGER than the block actually
    // malloc'd, and a later ReadFile clamped to that inflated allocSize would overrun the heap block. One
    // read guarantees the malloc size and allocSize always agree and every buffer is sized consistently.
    const unsigned int bs=blockSize;
    for(int i=0;i<NUM_BUFFERS;i++)
    {
        if(pipelineBuffers[i].data==nullptr || pipelineBuffers[i].allocSize!=bs)
        {
            free(pipelineBuffers[i].data);
            pipelineBuffers[i].allocSize=0;   // if malloc fails, allocSize must not keep the old (wrong) size
            pipelineBuffers[i].data=(char*)malloc(bs);
            if(pipelineBuffers[i].data==nullptr)
            {
                // an error-wait like any other failure: without a flag the base reported the (empty)
                // destination COMPLETE and a MOVE then deleted its source
                errorString_internal="Out of memory allocating pipeline buffers";
                writeError=true;
                emit errorOnFile(source,errorString_internal);
                return;
            }
            pipelineBuffers[i].allocSize=bs;
        }
        if(pipelineBuffers[i].ov==nullptr)
        {
            pipelineBuffers[i].ov=(OVERLAPPED*)calloc(1,sizeof(OVERLAPPED));
            if(pipelineBuffers[i].ov==nullptr)
            {
                errorString_internal="Out of memory allocating pipeline buffers";
                writeError=true;
                emit errorOnFile(source,errorString_internal);
                return;
            }
        }
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].state=PipelineBuffer::Free;
    }
}

void TransferThreadWin::freePipelineBuffers()
{
    for(int i=0;i<NUM_BUFFERS;i++)
    {
        free(pipelineBuffers[i].data);
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
        free(pipelineBuffers[i].ov);
        pipelineBuffers[i].ov=nullptr;
    }
}

// Returns 0 on success (sourceHandle set), -1 on error (errorString_internal set).
int TransferThreadWin::openSourceFile()
{
    const std::wstring sourcePath=iocpFinalPath(source);
    DWORD flags=FILE_FLAG_OVERLAPPED;
    if(os_spec_flags)
        flags|=FILE_FLAG_SEQUENTIAL_SCAN;
    HANDLE h=CreateFileW(sourcePath.c_str(),GENERIC_READ,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,
                         OPEN_EXISTING,flags,nullptr);
    if(h==INVALID_HANDLE_VALUE)
    {
        const DWORD err=GetLastError();
        errorString_internal=winErrorString(err);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open source: "+
                                 TransferThread::internalStringTostring(source)+", error: "+errorString_internal+" ("+std::to_string((unsigned long)err)+")");
        return -1;
    }
    // Associate to the completion port; read completions arrive with key KEY_SOURCE.
    if(CreateIoCompletionPort(h,iocp,KEY_SOURCE,0)==nullptr)
    {
        errorString_internal=winErrorString(GetLastError());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateIoCompletionPort(source) failed: "+errorString_internal);
        CloseHandle(h);
        return -1;
    }
    LARGE_INTEGER li;
    if(!GetFileSizeEx(h,&li))
    {
        // The size is what the pipeline reads: with 0 nothing was read, "0 == 0 bytes" and the file was
        // reported COMPLETE with an EMPTY destination (a MOVE then deleted the source). Fail the open ->
        // the error policy (retry/skip) handles it. Same fix as io_uring's fstat (iouring_fstat_fail).
        errorString_internal="GetFileSizeEx: "+winErrorString(GetLastError());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to size the opened source: "+
                                 TransferThread::internalStringTostring(source)+", error: "+errorString_internal);
        CloseHandle(h);
        return -1;
    }
    size_at_open=li.QuadPart;
    sourceFileSize=(uint64_t)li.QuadPart;
    // Never keep the PREVIOUS file's time on a failed query: the resume gate requires a non-zero
    // sub-second mtime, so 0 makes it refuse a resume (restart from 0 -- the safe side).
    mtime_at_open=0;
    mtime_nsec_at_open=0;
    ctime_at_open=0;
    FILETIME ftMod;
    if(GetFileTime(h,nullptr,nullptr,&ftMod))
    {
        // FILETIME is 100ns ticks since 1601. Keep FULL precision for the media-reconnect resume
        // gate (TransferThreadPipelined::resumeAfterErrorAndSeek): seconds + 100ns remainder, so an
        // in-place rewrite that keeps size and whole-second mtime is still detected. Windows has no
        // POSIX ctime, so ctime_at_open stays 0 (the gate compares 0==0 and relies on 100ns mtime).
        ULARGE_INTEGER u; u.LowPart=ftMod.dwLowDateTime; u.HighPart=ftMod.dwHighDateTime;
        uint64_t unixTicks=(u.QuadPart>=116444736000000000ULL)?(u.QuadPart-116444736000000000ULL):0;
        mtime_at_open=unixTicks/10000000ULL;
        mtime_nsec_at_open=unixTicks%10000000ULL;   // 100ns units
        ctime_at_open=0;
    }
    sourceHandle=h;
    return 0;
}

int TransferThreadWin::openDestFile(uint64_t startSize)
{
    // Ensure destination directory exists
    const size_t destinationIndex=destination.rfind('/');
    if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
    {
        const INTERNALTYPEPATH &path=destination.substr(0,destinationIndex);
        if(!TransferThread::is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                errorString_internal=tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+TransferThread::internalStringTostring(path));
                return -1;
            }
    }

    const std::wstring destPath=iocpFinalPath(destination);
    // CREATE_ALWAYS truncates the destination to 0 at open. That is right ONLY for a genuinely fresh
    // destination. OPEN_ALWAYS (no truncation) for a media-reconnect RESUME (startSize>0: keep the prefix
    // [0,startSize)) AND for a fresh OVERWRITE of a user's PRE-EXISTING destination (#9, as io_uring): the old
    // content stays until the whole new content is in place and is only then cut to the new size (end of
    // doTransferPipeline), so a source that turns out unreadable at byte 0 leaves the user's file untouched
    // instead of a 0-byte wreck. Data safety over the CREATE_ALWAYS shortcut (operator decision 2026-10-06).
    const DWORD disposition=(startSize>0 || destinationPreExisted)?OPEN_ALWAYS:CREATE_ALWAYS;
    DWORD destFlags=FILE_FLAG_OVERLAPPED;
    if(os_spec_flags)
        destFlags|=FILE_FLAG_SEQUENTIAL_SCAN;   // mirror the source hint: the dest is a whole-file
                                                // sequential write, so let the cache manager optimise
                                                // write-behind for it (same os_spec_flags gate as source).
    HANDLE h=CreateFileW(destPath.c_str(),GENERIC_WRITE,
                         FILE_SHARE_READ,nullptr,
                         disposition,destFlags,nullptr);
    if(h==INVALID_HANDLE_VALUE)
    {
        const DWORD err=GetLastError();
        errorString_internal=winErrorString(err);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open dest: "+
                                 TransferThread::internalStringTostring(destination)+", error: "+errorString_internal+" ("+std::to_string((unsigned long)err)+")");
        return -1;
    }
    // Associate to the completion port; write completions arrive with key KEY_DEST.
    if(CreateIoCompletionPort(h,iocp,KEY_DEST,0)==nullptr)
    {
        errorString_internal=winErrorString(GetLastError());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateIoCompletionPort(dest) failed: "+errorString_internal);
        CloseHandle(h);
        return -1;
    }

    // RESUME keep-prefix: startSize>0 only on a media-reconnect resume (the verified contiguous
    // prefix). With OPEN_ALWAYS above, bytes [0,startSize) survive; SetEndOfFile trims any
    // out-of-order tail written above the prefix back to startSize.
    if(startSize>0)
    {
        // Guard: the existing destination MUST already contain the whole prefix. If it is shorter
        // (deleted/truncated externally between attempts), SetEndOfFile would EXTEND it with zeros
        // below the resume point -> corruption. Fail instead -> the retry restarts from 0.
        LARGE_INTEGER curSz;
        if(!GetFileSizeEx(h,&curSz) || (uint64_t)curSz.QuadPart<startSize)
        {
            errorString_internal="resume prefix missing/short on destination";
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            CloseHandle(h);
            return -1;
        }
        LARGE_INTEGER li; li.QuadPart=(LONGLONG)startSize;
        if(!SetFilePointerEx(h,li,nullptr,FILE_BEGIN) || !SetEndOfFile(h))
        {
            const DWORD err=GetLastError();
            errorString_internal=winErrorString(err);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to trim to resume prefix: "+errorString_internal);
            CloseHandle(h);
            return -1;
        }
        // Make the kept prefix durable before we trust+append to it (the resume contract). Per
        // RESUME-event (rare), not per-file, so it does not reintroduce a per-file flush cost.
        FlushFileBuffers(h);
        // Reset the pointer to 0; overlapped I/O uses per-op offsets regardless, but
        // keep the handle's logical pointer tidy.
        LARGE_INTEGER zero; zero.QuadPart=0;
        SetFilePointerEx(h,zero,nullptr,FILE_BEGIN);
    }
    else if(sourceFileSize>=PREALLOCATE_MIN_SIZE)
    {
        // Best-effort PREALLOCATION of a FRESH large destination (reduce fragmentation): reserve the
        // full extent's worth of CONTIGUOUS clusters up front so the out-of-order pipelined writes land
        // contiguously instead of growing the file piecemeal. Use FileAllocationInfo, NOT SetEndOfFile:
        // it reserves the ALLOCATION size WITHOUT moving the logical end-of-file, so st_size still grows
        // only with real writes -- matching io_uring's fallocate(FALLOC_FL_KEEP_SIZE). THIS IS THE
        // DATA-SAFETY-CRITICAL CHOICE: SetEndOfFile would set st_size==sourceFileSize immediately, so a
        // HARD crash / power-loss / kill (which bypasses the graceful destinationIsOursToRemove() unlink)
        // would leave a FULL-size, zero-tailed file that a later size-only collision policy
        // (fileCollision "overwrite if not same size", idx 7) would silently SKIP as already-complete =
        // silent data corruption. FileAllocationInfo instead leaves an interrupted copy SHORT (st_size <
        // source), so that policy correctly RE-COPIES it (and it also avoids the VDL zero-fill cost).
        // FRESH file only (startSize==0), size-gated, best-effort (ignore failure; ENOSPC still surfaces
        // at write time). NOT SetFileValidData (privileged + leaks stale on-disk bytes).
        FILE_ALLOCATION_INFO fai;
        fai.AllocationSize.QuadPart=(LONGLONG)sourceFileSize;
        SetFileInformationByHandle(h,FileAllocationInfo,&fai,sizeof(fai));
    }

    destHandle=h;
    return 0;
}

int TransferThreadWin::bufferIndexForOverlapped(OVERLAPPED *ov)
{
    for(int i=0;i<NUM_BUFFERS;i++)
        if(pipelineBuffers[i].ov==ov)
            return i;
    return -1;
}

void TransferThreadWin::closeFiles()
{
    // CloseHandle cancels any still-pending overlapped op on the handle. The caller
    // (doTransferPipeline) drains all completions before getting here in the normal
    // path; on stop/error it cancels+drains first, so no completion can land in a
    // buffer after close. No FlushFileBuffers: a file copy needs no per-file
    // durability (mirrors the io_uring backend dropping per-file fsync).
    if(sourceHandle!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(sourceHandle);
        sourceHandle=INVALID_HANDLE_VALUE;
    }
    if(destHandle!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(destHandle);
        destHandle=INVALID_HANDLE_VALUE;
    }
}

void TransferThreadWin::doTransferPipeline()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline start");

    // Initialize pipeline buffers
    initPipelineBuffers();
    for(int i=0;i<NUM_BUFFERS;i++)
        if(pipelineBuffers[i].data==nullptr || pipelineBuffers[i].ov==nullptr)
        {
            closeFiles();
            return; // error already emitted
        }

    // Seed ALL offsets from the resume cursor together (0 for a normal/first transfer ->
    // byte-for-byte the old behaviour). On a media-reconnect resume, resumeFromOffset is the
    // verified contiguous prefix already on disk, so reads/writes continue from there.
    readOffset=(int64_t)resumeFromOffset;
    transferProgression=resumeFromOffset;
    contiguousWrittenOffset=resumeFromOffset;
    completedWriteExtents.clear();

    int readsInFlight=0;
    int writesInFlight=0;
    bool readDone=false;
    bool errorOccurred=false;
    resetSpeedThrottle();

    // Issue an overlapped ReadFile on buffer idx (offset stored in its OVERLAPPED).
    // Returns false only on a real submission error (not ERROR_IO_PENDING); a
    // completion packet is always posted to the IOCP otherwise, even when ReadFile
    // completes synchronously (we don't set FILE_SKIP_COMPLETION_PORT_ON_SUCCESS).
    auto issueRead=[&](int idx,char *ptr,unsigned int len,int64_t off)->bool{
        OVERLAPPED &ov=*pipelineBuffers[idx].ov;
        memset(&ov,0,sizeof(OVERLAPPED));
        ov.Offset=(DWORD)((uint64_t)off & 0xFFFFFFFFULL);
        ov.OffsetHigh=(DWORD)((uint64_t)off >> 32);
        pipelineBuffers[idx].state=PipelineBuffer::Reading;
        if(!ReadFile(sourceHandle,ptr,len,nullptr,&ov))
        {
            const DWORD e=GetLastError();
            if(e!=ERROR_IO_PENDING)
            {
                // a SYNCHRONOUS failure queues no completion packet: the buffer is NOT in flight (left as
                // Reading, the drain waited its deadline for it and quarantined it)
                pipelineBuffers[idx].state=PipelineBuffer::Free;
                errorString_internal="Read error: "+winErrorString(e);
                return false;
            }
        }
        return true;
    };
    // Issue an overlapped WriteFile on buffer idx at the given file offset.
    auto issueWrite=[&](int idx,char *ptr,unsigned int len,int64_t off)->bool{
        OVERLAPPED &ov=*pipelineBuffers[idx].ov;
        memset(&ov,0,sizeof(OVERLAPPED));
        ov.Offset=(DWORD)((uint64_t)off & 0xFFFFFFFFULL);
        ov.OffsetHigh=(DWORD)((uint64_t)off >> 32);
        pipelineBuffers[idx].state=PipelineBuffer::Writing;
        if(!WriteFile(destHandle,ptr,len,nullptr,&ov))
        {
            const DWORD e=GetLastError();
            if(e!=ERROR_IO_PENDING)
            {
                pipelineBuffers[idx].state=PipelineBuffer::Free;// synchronous failure: no packet will come
                errorString_internal="Write error: "+winErrorString(e);
                return false;
            }
        }
        return true;
    };

    // Submit initial batch of reads (one per buffer)
    for(int i=0;i<NUM_BUFFERS && !readDone && !stopIt;i++)
    {
        if((uint64_t)readOffset>=sourceFileSize)
        {
            readDone=true;
            break;
        }
        throttleBlockForSpeedLimit();// speed limit: one tick of the clock every multiForBigSpeed blocks
        if(stopIt)
            break;
        unsigned int toRead=blockSize;
        if((uint64_t)(readOffset+toRead)>sourceFileSize)
            toRead=(unsigned int)(sourceFileSize-readOffset);
        // Never read past the buffer's allocated capacity: blockSize can be changed from another
        // thread (speed-limiter/options) after initPipelineBuffers() sized this buffer, so clamp
        // to allocSize to prevent a heap overflow if blockSize grew since allocation.
        if(toRead>pipelineBuffers[i].allocSize)
            toRead=pipelineBuffers[i].allocSize;

        pipelineBuffers[i].fileOffset=readOffset;
        pipelineBuffers[i].chunkSize=toRead;
        pipelineBuffers[i].bytesUsed=0;
        if(!issueRead(i,pipelineBuffers[i].data,toRead,readOffset))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            readError=true;
            errorOccurred=true;
            emit errorOnFile(source,errorString_internal);
            break;
        }
        readOffset+=toRead;
        readsInFlight++;
    }

    // Main event loop — drain a batch of completions, react, re-arm
    OVERLAPPED_ENTRY entries[NUM_BUFFERS];
    // Bounded completion wait, NOT INFINITE (Bug C): stop()/skip() only SET stopIt (they must not
    // CloseHandle across threads to wake us -- that races our live ReadFile/WriteFile and the closed handle
    // is recycled by a sibling inode thread). We poll stopIt/putInPause every UC_IOCP_WAIT_MS. During an
    // active transfer, completions arrive far faster than this, so the timeout is a zero-cost fallback that
    // only fires when the pipeline is genuinely idle-waiting (or on a stop/skip/pause).
    static constexpr DWORD UC_IOCP_WAIT_MS=200;
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

        ULONG numEntries=0;
        if(!GetQueuedCompletionStatusEx(iocp,entries,NUM_BUFFERS,&numEntries,UC_IOCP_WAIT_MS,FALSE))
        {
            const DWORD e=GetLastError();
            if(e==WAIT_TIMEOUT)
                continue; // nothing completed in this window -> loop back, re-check stopIt/putInPause, wait again
            errorString_internal="GetQueuedCompletionStatusEx failed: "+winErrorString(e);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            readError=true;
            errorOccurred=true;
            break;
        }

        // EVERY completion of this file is ACCOUNTED (counter + buffer state) -- even after an error or a stop
        // has been seen. This batch is already dequeued: a completion left unaccounted made the cancel-drain
        // wait its full deadline for a packet that could never come, QUARANTINE a buffer the kernel no longer
        // owned and recreate the port -- a 2 s stall + up to 8 leaked buffers PER failing file on a dying disk
        // (the io_uring twin: iouring_faulty_hdd_many). Only the issuing of NEW I/O stops at the first
        // error/stop (the file is abandoned).
        for(ULONG ci=0;ci<numEntries;ci++)
        {
            const ULONG_PTR key=entries[ci].lpCompletionKey;
            const DWORD result=entries[ci].dwNumberOfBytesTransferred;
            const ULONG status=(ULONG)entries[ci].Internal; // NTSTATUS of the op
            const bool isEof=(status==0xC0000011UL);         // STATUS_END_OF_FILE
            const bool isError=(((LONG)status)<0) && !isEof;
            const int bufIdx=bufferIndexForOverlapped(entries[ci].lpOverlapped);
            const bool abandon=errorOccurred || stopIt;
            if(bufIdx<0)
            {
                // Completion for an unknown OVERLAPPED (should not happen) -- ignore.
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] completion for unknown overlapped");
                continue;
            }
            PipelineBuffer &buf=pipelineBuffers[bufIdx];
            if(key==KEY_SOURCE)
            {
                // Read completion (analog of OP_READ_TAG)
                readsInFlight--;
                if(isError)
                {
                    buf.state=PipelineBuffer::Free;
                    if(!errorOccurred)// one error per attempt
                    {
                        errorString_internal="Read error, NTSTATUS 0x"+std::to_string((unsigned long)status);
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                        readError=true;
                        errorOccurred=true;
                        emit errorOnFile(source,errorString_internal);
                    }
                }
                else if(abandon)
                    buf.state=PipelineBuffer::Free;// accounted, not written: the file is being abandoned
                else if(result==0 || isEof)
                {
                    // EOF before the chunk was full: the source SHRANK since the scan. The bytes already
                    // in the buffer are still written (the end-of-copy size check then reports the change).
                    readDone=true;
                    if(buf.bytesUsed>0)
                    {
                        if(!issueWrite(bufIdx,buf.data,(unsigned int)buf.bytesUsed,buf.fileOffset))
                        {
                            writeError=true;
                            errorOccurred=true;
                            emit errorOnFile(destination,errorString_internal);
                        }
                        else
                            writesInFlight++;
                    }
                    else
                        buf.state=PipelineBuffer::Free;
                }
                else
                {
                    buf.bytesUsed+=result;
                    if((unsigned int)buf.bytesUsed<buf.chunkSize)
                    {
                        // Short read: fetch the rest of this chunk into the same buffer
                        // before writing, so partial reads under load can't corrupt data.
                        const unsigned int rem=buf.chunkSize-(unsigned int)buf.bytesUsed;
                        if(!issueRead(bufIdx,buf.data+buf.bytesUsed,rem,buf.fileOffset+buf.bytesUsed))
                        {
                            readError=true;
                            errorOccurred=true;
                            emit errorOnFile(source,errorString_internal);
                        }
                        else
                            readsInFlight++;
                    }
                    else
                    {
                        // Chunk fully read -> write it back at the exact offset it came
                        // from (NOT a global write cursor): completions arrive out of
                        // order, so completion-order writing scrambled the file.
                        if(!issueWrite(bufIdx,buf.data,(unsigned int)buf.bytesUsed,buf.fileOffset))
                        {
                            writeError=true;
                            errorOccurred=true;
                            emit errorOnFile(destination,errorString_internal);
                        }
                        else
                            writesInFlight++;
                    }
                }
            }
            else if(key==KEY_DEST)
            {
                // Write completion (analog of OP_WRITE_TAG)
                writesInFlight--;
                if(isError)
                {
                    buf.state=PipelineBuffer::Free;
                    if(!errorOccurred)
                    {
                        errorString_internal="Write error, NTSTATUS 0x"+std::to_string((unsigned long)status);
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                        writeError=true;
                        errorOccurred=true;
                        emit errorOnFile(destination,errorString_internal);
                    }
                    continue;
                }
                transferProgression+=result;
                // Fold this completed extent into the contiguous low-water mark, using the LIVE
                // fileOffset+result BEFORE the short-write advance below, so out-of-order overlapped
                // completions merge correctly and contiguousWrittenOffset stays the true written-
                // from-0 prefix (the only safe resume point; transferProgression may sit above a hole).
                recordCompletedWrite((uint64_t)buf.fileOffset,(uint64_t)result);
                // Handle short write
                if((int)result<buf.bytesUsed)
                {
                    if(abandon)
                    {
                        buf.state=PipelineBuffer::Free;// the rest of the chunk is not re-issued
                        continue;
                    }
                    const int remaining=buf.bytesUsed-(int)result;
                    memmove(buf.data,buf.data+result,remaining);
                    buf.bytesUsed=remaining;
                    buf.fileOffset+=result;
                    if(!issueWrite(bufIdx,buf.data,(unsigned int)remaining,buf.fileOffset))
                    {
                        writeError=true;
                        errorOccurred=true;
                        emit errorOnFile(destination,errorString_internal);
                    }
                    else
                        writesInFlight++;
                    continue;
                }

                // Write completed fully -> reuse this buffer for the next chunk
                buf.state=PipelineBuffer::Free;
                if(abandon)
                    continue;

                // Queue next read if data remains
                if(!readDone && (uint64_t)readOffset<sourceFileSize)
                {
                    throttleBlockForSpeedLimit();
                    if(stopIt)
                        continue;
                    unsigned int toRead=blockSize;
                    if((uint64_t)(readOffset+toRead)>sourceFileSize)
                        toRead=(unsigned int)(sourceFileSize-readOffset);
                    // Clamp to the buffer's allocated capacity (see initial-batch note).
                    if(toRead>buf.allocSize)
                        toRead=buf.allocSize;
                    buf.fileOffset=readOffset;
                    buf.chunkSize=toRead;
                    buf.bytesUsed=0;
                    if(!issueRead(bufIdx,buf.data,toRead,readOffset))
                    {
                        readError=true;
                        errorOccurred=true;
                        emit errorOnFile(source,errorString_internal);
                    }
                    else
                    {
                        readOffset+=toRead;
                        readsInFlight++;
                    }
                }
                else if((uint64_t)readOffset>=sourceFileSize)
                {
                    readDone=true;
                }
            }
        }
    }

    // On stop/error there may still be ops in flight. They MUST be fully accounted for before this
    // function returns, because the completion port and the pipeline buffers/OVERLAPPEDs are PERSISTENT
    // and reused for the NEXT file: any completion still owned by the kernel, if it lands after we reuse
    // (or free) a buffer, is a write-after-free / stale-completion-misattribution -> the probabilistic
    // STATUS_HEAP_CORRUPTION. (THE #1 root-cause fix.)
    if(readsInFlight>0 || writesInFlight>0)
    {
        // Close the handles FIRST: CloseHandle forces the kernel to cancel/complete every pending IRP on
        // them, which BOUNDS the drain. CancelIoEx alone only REQUESTS cancellation and an IRP wedged in
        // the device / a filesystem minifilter (dying sector, Defender under load) can outlive any wait.
        closeFiles();
        const ULONGLONG deadline=GetTickCount64()+2000;// 64-bit: a 32-bit wrap inside the window blocked for days
        while(readsInFlight>0 || writesInFlight>0)
        {
            const ULONGLONG now=GetTickCount64();
            const DWORD waitMs=(now>=deadline)?0:(DWORD)(deadline-now);
            ULONG numEntries=0;
            if(!GetQueuedCompletionStatusEx(iocp,entries,NUM_BUFFERS,&numEntries,waitMs,FALSE))
                break; // timed out with ops STILL in flight -> a genuinely wedged IRP; quarantine below
            for(ULONG ci=0;ci<numEntries;ci++)
            {
                if(entries[ci].lpCompletionKey==KEY_SOURCE)
                    readsInFlight--;
                else if(entries[ci].lpCompletionKey==KEY_DEST)
                    writesInFlight--;
            }
        }
        if(readsInFlight>0 || writesInFlight>0)
        {
            // A wedged IRP will complete/cancel LATER: its kernel DMA still references this buffer's
            // data and writes status into this buffer's OVERLAPPED. We must NEVER reuse or free those.
            // (1) QUARANTINE every still-in-flight buffer (orphan its memory; freed only in ~dtor, after
            //     the worker has fully stopped) so the next file allocates a FRESH block instead.
            // (2) RECREATE the completion port so that, whenever the stale packet finally posts, it
            //     targets the destroyed port and is DROPPED -- it can never be dequeued by a later file's
            //     GetQueuedCompletionStatusEx and mis-mapped (by OVERLAPPED address) to a reused buffer.
            // Bounded: <= NUM_BUFFERS buffers orphaned per unrecoverable stuck-I/O event; no hang.
            for(int i=0;i<NUM_BUFFERS;i++)
                if(pipelineBuffers[i].state==PipelineBuffer::Reading ||
                   pipelineBuffers[i].state==PipelineBuffer::Writing)
                {
                    orphanedBuffers.push_back(pipelineBuffers[i].data);
                    pipelineBuffers[i].data=nullptr;      // next initPipelineBuffers() mallocs a fresh block
                    pipelineBuffers[i].allocSize=0;
                    orphanedOverlapped.push_back(pipelineBuffers[i].ov);// the wedged IRP still owns it too
                    pipelineBuffers[i].ov=nullptr;
                    pipelineBuffers[i].state=PipelineBuffer::Free;
                }
            if(iocpInitialized)
            {
                CloseHandle(iocp);
                iocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE,nullptr,0,0);
                if(iocp==nullptr)
                {
                    // Cannot continue without a completion port (same fatal stance as run()).
                    std::cerr << "CreateIoCompletionPort (recreate) failed: " << winErrorString(GetLastError()) << std::endl;
                    iocpInitialized=false;
                    abort();
                }
            }
            readsInFlight=0;
            writesInFlight=0;
        }
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
        if(transferProgression!=sourceFileSize)
        {
            // fewer bytes than the size at open: the source SHRANK while it was read (async parity:
            // "File truncated during the read"); never claim the file complete
            errorString_internal=tr("File truncated during the read, possible data change").toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            readError=true;
            errorOccurred=true;
            emit errorOnFile(source,errorString_internal);
        }
        else
        {
            // a pre-existing destination longer than the source keeps a stale tail: cut it, now that the whole
            // new content is in place (the open deliberately did not truncate it, guard #9 -- as io_uring)
            LARGE_INTEGER newEnd;
            newEnd.QuadPart=(LONGLONG)sourceFileSize;
            if(destinationPreExisted && resumeFromOffset==0 &&
               (!SetFilePointerEx(destHandle,newEnd,nullptr,FILE_BEGIN) || !SetEndOfFile(destHandle)))
            {
                errorString_internal="Unable to truncate: "+winErrorString(GetLastError());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                writeError=true;
                errorOccurred=true;
                emit errorOnFile(destination,errorString_internal);
            }
            else
                // Whole file contiguously written -> resume water-mark equals the file size. (On the ERROR
                // path leave contiguousWrittenOffset at its true mid-file value so resumeAfterErrorAndSeek()
                // reads the real safe offset.)
                contiguousWrittenOffset=(uint64_t)sourceFileSize;
        }
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline end, transferred: "+std::to_string(transferProgression));
}
void TransferThreadWin::trimDestinationToContiguous()
{
    LARGE_INTEGER pos;
    pos.QuadPart=(LONGLONG)contiguousWrittenOffset;
    HANDLE h=destHandle;
    // The error/stop drain closes the handles FIRST (CloseHandle is what bounds a wedged IRP), so by the time
    // the base asks for the trim the handle is gone -- the kept salvage of a fault-skip was then left with
    // whatever landed out of order above the contiguous prefix (zero-filled gap, NOT a faithful prefix).
    // Reopen by path (plain synchronous handle) just to cut it; nothing of ours is in flight any more.
    const bool reopened=(h==INVALID_HANDLE_VALUE);
    if(reopened)
    {
        h=CreateFileW(iocpFinalPath(destination).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to reopen the partial destination to trim it: "+winErrorString(GetLastError()));
            return;
        }
    }
    if(!SetFilePointerEx(h,pos,nullptr,FILE_BEGIN) || !SetEndOfFile(h))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to trim the partial destination: "+winErrorString(GetLastError()));
    if(reopened)
        CloseHandle(h);
}

bool TransferThreadWin::remainSourceOpen() const
{
    return sourceHandle!=INVALID_HANDLE_VALUE;
}

bool TransferThreadWin::remainDestinationOpen() const
{
    return destHandle!=INVALID_HANDLE_VALUE;
}

bool TransferThreadWin::applyDateTimeOnOpenDestination()
{
    // Set the source times (cached into ftCreate/ftAccess/ftWrite by
    // readSourceFileDateTime in preOperation) on the destination while its handle is
    // STILL OPEN, so doFilePostOperation does not reopen the file just to SetFileTime.
    // The dest handle was opened GENERIC_WRITE (CREATE_ALWAYS), enough for SetFileTime.
    if(destHandle==INVALID_HANDLE_VALUE)
        return false;
    FILETIME c=this->ftCreate, a=this->ftAccess, w=this->ftWrite;
    if(!SetFileTime(destHandle,&c,&a,&w))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] SetFileTime on open dest handle failed: "+winErrorString(GetLastError()));
        return false; // fall back to the reopen path in doFilePostOperation
    }
    return true;
}

bool TransferThreadWin::mkJunction(const wchar_t *szJunction, const wchar_t *szPath)
{
    BYTE buf[sizeof(IOCP_REPARSE_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    IOCP_REPARSE_DATA_BUFFER& ReparseBuffer = (IOCP_REPARSE_DATA_BUFFER&)buf;

    if(!CreateDirectoryW(szJunction, NULL))
        return false;
    HANDLE hDir = CreateFileW(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if(hDir == INVALID_HANDLE_VALUE)
        return false;

    memset(buf, 0, sizeof(buf));
    ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = wcslen(szPath)+1;
    if(len > MAX_PATH)
    {
        // The reparse PathBuffer holds at most MAX_PATH WCHARs; a longer junction target
        // would overflow the fixed stack buffer in the wcscpy below. Fail this junction
        // (same as the other error paths) rather than corrupt the stack.
        CloseHandle(hDir);
        RemoveDirectoryW(szJunction);
        return false;
    }
    ReparseBuffer.MountPointReparseBuffer.PrintNameOffset = (len--) * sizeof(WCHAR);
    ReparseBuffer.MountPointReparseBuffer.SubstituteNameLength = len * sizeof(WCHAR);
    ReparseBuffer.ReparseDataLength = ReparseBuffer.MountPointReparseBuffer.SubstituteNameLength + 12;
    wcscpy(ReparseBuffer.MountPointReparseBuffer.PathBuffer, szPath);

    DWORD dwRet;
    if(!DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer,
                        ReparseBuffer.ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL))
    {
        CloseHandle(hDir);
        RemoveDirectoryW(szJunction);
        return false;
    }
    CloseHandle(hDir);
    return true;
}

bool TransferThreadWin::trySymlinkCopy()
{
    // Recreate a Windows symlink/junction at the destination instead of following it
    // (matches the async backend). A non-reparse source returns false -> regular pipeline.
    bool isJunction=false;
    bool isSymlink=false;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if(GetFileAttributesExW(TransferThread::toFinalPath(source).c_str(),GetFileExInfoStandard,&fileInfo)!=FALSE)
        isSymlink = fileInfo.dwFileAttributes!=INVALID_FILE_ATTRIBUTES &&
                    (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
    if(!isSymlink)
        return false;

    BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    memset(buf,0,sizeof(buf));
    IOCP_REPARSE_DATA_BUFFER& ReparseBuffer = (IOCP_REPARSE_DATA_BUFFER&)buf;
    DWORD dwRet=0;
    HANDLE hDir = CreateFileW(TransferThread::toFinalPath(source).c_str(), GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if(hDir != INVALID_HANDLE_VALUE)
    {
        BOOL ioc = DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseBuffer,
                                   MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRet, NULL);
        CloseHandle(hDir);
        if(ioc!=FALSE)
        {
            if(ReparseBuffer.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                isJunction=true;
        }
        else
        {
            const std::string &strError=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] reparse read error: "+strError);
            readError=true;
            writeError=false;
            emit errorOnFile(source,strError);
            return true;
        }
    }
    else
    {
        const std::string &strError=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] reparse open error: "+strError);
        readError=true;
        writeError=false;
        emit errorOnFile(source,strError);
        return true;
    }

    bool successFull=false;
    if(isJunction)
    {
        // Extract the substitute name via its offset+length rather than treating PathBuffer as a
        // NUL-terminated string: the reparse buffer is not guaranteed NUL-terminated at PathBuffer[0],
        // so scanning for a NUL could over-read past the kernel-filled data (and feed a garbage,
        // possibly very long, path to mkJunction).
        const auto &mp=ReparseBuffer.MountPointReparseBuffer;
        std::wstring cleanPath((const wchar_t*)((const BYTE*)mp.PathBuffer+mp.SubstituteNameOffset),
                               mp.SubstituteNameLength/sizeof(WCHAR));
        if(cleanPath.substr(0,4)==L"\\??\\")
            cleanPath=cleanPath.substr(4);
        successFull=mkJunction(TransferThread::toFinalPath(destination).c_str(),cleanPath.c_str());
        if(!successFull)
        {
            const std::string &strError=TransferThread::GetLastErrorStdStr();
            readError=true;
            writeError=true;
            emit errorOnFile(destination,strError);
            return true;
        }
    }
    else//symlink or symlinkD
    {
        successFull=CopyFileExW(TransferThread::toFinalPath(source).c_str(),TransferThread::toFinalPath(destination).c_str(),
            NULL,NULL,&stopItWin,COPY_FILE_ALLOW_DECRYPTED_DESTINATION | 0x00000800);//0x00000800 is COPY_FILE_COPY_SYMLINK
        if(!successFull)
        {
            const std::string &strError=TransferThread::GetLastErrorStdStr();
            readError=true;
            writeError=true;
            emit errorOnFile(destination,strError);
            return true;
        }
    }
    // The reparse point was recreated at the destination (every failure path returned above). As on POSIX:
    // realMove=true keeps the completion from running the checksum re-open through the link (a dangling
    // target made that a spurious "unable to re-open" error) and from treating it as a byte copy; a MOVE
    // then completes by removing the SOURCE link/junction here (DeleteFileW || RemoveDirectoryW). Without
    // it a cross-drive MOVE of a junction kept the source (is_file() is false for it) and silently became
    // a copy. COPY keeps the source. (Return checked per the FS-mutation rule.)
    realMove=true;
    if(mode==Ultracopier::Move && source!=destination)
        if(!unlink(source))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+
                                     "] reparse-point move: unable to remove the source link: "+
                                     internalStringTostring(source)+" ("+TransferThread::GetLastErrorStdStr()+")");
    checkIfAllIsClosedAndDoOperations();
    return true;
}

#endif // ULTRACOPIER_PLUGIN_WINIOCP
