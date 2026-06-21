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
    writeOffset=0;

    for(int i=0;i<NUM_BUFFERS;i++)
    {
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
        pipelineBuffers[i].bytesUsed=0;
        pipelineBuffers[i].fileOffset=0;
        pipelineBuffers[i].chunkSize=0;
        memset(&pipelineBuffers[i].ov,0,sizeof(OVERLAPPED));
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

void TransferThreadWin::freePipelineBuffers()
{
    for(int i=0;i<NUM_BUFFERS;i++)
    {
        free(pipelineBuffers[i].data);
        pipelineBuffers[i].data=nullptr;
        pipelineBuffers[i].allocSize=0;
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
    if(GetFileSizeEx(h,&li))
    {
        size_at_open=li.QuadPart;
        sourceFileSize=(uint64_t)li.QuadPart;
    }
    else
        sourceFileSize=0;
    FILETIME ftMod;
    if(GetFileTime(h,nullptr,nullptr,&ftMod))
    {
        // FILETIME is 100ns ticks since 1601; convert to Unix seconds for mtime_at_open.
        ULARGE_INTEGER u; u.LowPart=ftMod.dwLowDateTime; u.HighPart=ftMod.dwHighDateTime;
        mtime_at_open=(uint64_t)((u.QuadPart-116444736000000000ULL)/10000000ULL);
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

    const std::wstring destPath=iocpFinalPath(destination);
    HANDLE h=CreateFileW(destPath.c_str(),GENERIC_WRITE,
                         FILE_SHARE_READ,nullptr,
                         CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,nullptr);
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

    // Preallocate (analog of ftruncate): set the size up front so the overlapped
    // out-of-order writes never have to extend the file mid-flight.
    if(startSize>0)
    {
        LARGE_INTEGER li; li.QuadPart=(LONGLONG)startSize;
        if(!SetFilePointerEx(h,li,nullptr,FILE_BEGIN) || !SetEndOfFile(h))
        {
            const DWORD err=GetLastError();
            errorString_internal=winErrorString(err);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to preallocate: "+errorString_internal);
            CloseHandle(h);
            return -1;
        }
        // Reset the pointer to 0; overlapped I/O uses per-op offsets regardless, but
        // keep the handle's logical pointer tidy.
        LARGE_INTEGER zero; zero.QuadPart=0;
        SetFilePointerEx(h,zero,nullptr,FILE_BEGIN);
    }

    destHandle=h;
    return 0;
}

int TransferThreadWin::bufferIndexForOverlapped(OVERLAPPED *ov)
{
    for(int i=0;i<NUM_BUFFERS;i++)
        if(&pipelineBuffers[i].ov==ov)
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

    // Issue an overlapped ReadFile on buffer idx (offset stored in its OVERLAPPED).
    // Returns false only on a real submission error (not ERROR_IO_PENDING); a
    // completion packet is always posted to the IOCP otherwise, even when ReadFile
    // completes synchronously (we don't set FILE_SKIP_COMPLETION_PORT_ON_SUCCESS).
    auto issueRead=[&](int idx,char *ptr,unsigned int len,int64_t off)->bool{
        OVERLAPPED &ov=pipelineBuffers[idx].ov;
        memset(&ov,0,sizeof(OVERLAPPED));
        ov.Offset=(DWORD)((uint64_t)off & 0xFFFFFFFFULL);
        ov.OffsetHigh=(DWORD)((uint64_t)off >> 32);
        pipelineBuffers[idx].state=PipelineBuffer::Reading;
        if(!ReadFile(sourceHandle,ptr,len,nullptr,&ov))
        {
            const DWORD e=GetLastError();
            if(e!=ERROR_IO_PENDING)
            {
                errorString_internal="Read error: "+winErrorString(e);
                return false;
            }
        }
        return true;
    };
    // Issue an overlapped WriteFile on buffer idx at the given file offset.
    auto issueWrite=[&](int idx,char *ptr,unsigned int len,int64_t off)->bool{
        OVERLAPPED &ov=pipelineBuffers[idx].ov;
        memset(&ov,0,sizeof(OVERLAPPED));
        ov.Offset=(DWORD)((uint64_t)off & 0xFFFFFFFFULL);
        ov.OffsetHigh=(DWORD)((uint64_t)off >> 32);
        pipelineBuffers[idx].state=PipelineBuffer::Writing;
        if(!WriteFile(destHandle,ptr,len,nullptr,&ov))
        {
            const DWORD e=GetLastError();
            if(e!=ERROR_IO_PENDING)
            {
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
        if(!GetQueuedCompletionStatusEx(iocp,entries,NUM_BUFFERS,&numEntries,INFINITE,FALSE))
        {
            const DWORD e=GetLastError();
            errorString_internal="GetQueuedCompletionStatusEx failed: "+winErrorString(e);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
            readError=true;
            errorOccurred=true;
            break;
        }

        for(ULONG ci=0;ci<numEntries && !errorOccurred && !stopIt;ci++)
        {
            const ULONG_PTR key=entries[ci].lpCompletionKey;
            const DWORD result=entries[ci].dwNumberOfBytesTransferred;
            const ULONG status=(ULONG)entries[ci].Internal; // NTSTATUS of the op
            const bool isEof=(status==0xC0000011UL);         // STATUS_END_OF_FILE
            const bool isError=(((LONG)status)<0) && !isEof;
            const int bufIdx=bufferIndexForOverlapped(entries[ci].lpOverlapped);
            if(bufIdx<0)
            {
                // Completion for an unknown OVERLAPPED (should not happen) — ignore.
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] completion for unknown overlapped");
            }
            else if(key==KEY_SOURCE)
            {
                // Read completion (analog of OP_READ_TAG)
                readsInFlight--;
                if(isError)
                {
                    errorString_internal="Read error, NTSTATUS 0x"+std::to_string((unsigned long)status);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                    readError=true;
                    errorOccurred=true;
                    emit errorOnFile(source,errorString_internal);
                }
                else if(result==0 || isEof)
                {
                    // EOF
                    pipelineBuffers[bufIdx].state=PipelineBuffer::Free;
                    readDone=true;
                }
                else
                {
                    pipelineBuffers[bufIdx].bytesUsed+=result;
                    if((unsigned int)pipelineBuffers[bufIdx].bytesUsed<pipelineBuffers[bufIdx].chunkSize)
                    {
                        // Short read: fetch the rest of this chunk into the same buffer
                        // before writing, so partial reads under load can't corrupt data.
                        const unsigned int rem=pipelineBuffers[bufIdx].chunkSize-(unsigned int)pipelineBuffers[bufIdx].bytesUsed;
                        if(!issueRead(bufIdx,
                                      pipelineBuffers[bufIdx].data+pipelineBuffers[bufIdx].bytesUsed,
                                      rem,
                                      pipelineBuffers[bufIdx].fileOffset+pipelineBuffers[bufIdx].bytesUsed))
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
                        if(!issueWrite(bufIdx,pipelineBuffers[bufIdx].data,
                                       (unsigned int)pipelineBuffers[bufIdx].bytesUsed,
                                       pipelineBuffers[bufIdx].fileOffset))
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
                    errorString_internal="Write error, NTSTATUS 0x"+std::to_string((unsigned long)status);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+errorString_internal);
                    writeError=true;
                    errorOccurred=true;
                    emit errorOnFile(destination,errorString_internal);
                }
                else
                {
                    transferProgression+=result;
                    // Handle short write
                    if((int)result<pipelineBuffers[bufIdx].bytesUsed)
                    {
                        const int remaining=pipelineBuffers[bufIdx].bytesUsed-(int)result;
                        memmove(pipelineBuffers[bufIdx].data,pipelineBuffers[bufIdx].data+result,remaining);
                        pipelineBuffers[bufIdx].bytesUsed=remaining;
                        pipelineBuffers[bufIdx].fileOffset+=result;
                        if(!issueWrite(bufIdx,pipelineBuffers[bufIdx].data,(unsigned int)remaining,pipelineBuffers[bufIdx].fileOffset))
                        {
                            writeError=true;
                            errorOccurred=true;
                            emit errorOnFile(destination,errorString_internal);
                        }
                        else
                            writesInFlight++;
                    }
                    else
                    {
                        // Write completed fully -> reuse this buffer for the next chunk
                        pipelineBuffers[bufIdx].state=PipelineBuffer::Free;

                        // Queue next read if data remains
                        if(!readDone && !stopIt && (uint64_t)readOffset<sourceFileSize)
                        {
                            unsigned int toRead=blockSize;
                            if((uint64_t)(readOffset+toRead)>sourceFileSize)
                                toRead=(unsigned int)(sourceFileSize-readOffset);
                            // Clamp to the buffer's allocated capacity (see initial-batch note).
                            if(toRead>pipelineBuffers[bufIdx].allocSize)
                                toRead=pipelineBuffers[bufIdx].allocSize;
                            pipelineBuffers[bufIdx].fileOffset=readOffset;
                            pipelineBuffers[bufIdx].chunkSize=toRead;
                            pipelineBuffers[bufIdx].bytesUsed=0;
                            if(!issueRead(bufIdx,pipelineBuffers[bufIdx].data,toRead,readOffset))
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
        }
    }

    // On stop/error there may still be ops in flight: cancel them and drain their
    // (now STATUS_CANCELLED) completions so the kernel never writes into a pipeline
    // buffer after this function returns and the buffer is reused for the next file.
    if(readsInFlight>0 || writesInFlight>0)
    {
        if(sourceHandle!=INVALID_HANDLE_VALUE)
            CancelIoEx(sourceHandle,nullptr);
        if(destHandle!=INVALID_HANDLE_VALUE)
            CancelIoEx(destHandle,nullptr);
        while(readsInFlight>0 || writesInFlight>0)
        {
            ULONG numEntries=0;
            if(!GetQueuedCompletionStatusEx(iocp,entries,NUM_BUFFERS,&numEntries,2000,FALSE))
                break; // timeout/failure: give up draining
            for(ULONG ci=0;ci<numEntries;ci++)
            {
                if(entries[ci].lpCompletionKey==KEY_SOURCE)
                    readsInFlight--;
                else if(entries[ci].lpCompletionKey==KEY_DEST)
                    writesInFlight--;
            }
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
        transferProgression=sourceFileSize;
    }

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] doTransferPipeline end, transferred: "+std::to_string(transferProgression));
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
    readIsClosedVariable=true;
    writeIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
    return true;
}

#endif // ULTRACOPIER_PLUGIN_WINIOCP
