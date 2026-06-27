#include "WriteThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif
#include "../TransferThread.h"

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef Q_OS_WIN32
// Lowercase matches mingw-w64 headers on case-sensitive filesystems (MXE, wine cross).
// Older mingw shipped with Qt5.5/Qt5.6 (XP target) lacks versionhelpers.h entirely;
// in that case provide a minimal IsWindows8OrGreater() shim using GetVersionEx.
#  if defined(__has_include)
#    if __has_include(<versionhelpers.h>)
#      include <versionhelpers.h>
#      define ULTRACOPIER_HAVE_VERSIONHELPERS 1
#    endif
#  endif
#  ifndef ULTRACOPIER_HAVE_VERSIONHELPERS
#    include <windows.h>
static inline bool IsWindows8OrGreater()
{
    OSVERSIONINFOW osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#    if defined(__GNUC__)
#      pragma GCC diagnostic push
#      pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#    endif
    if(!GetVersionExW(&osvi))
        return false;
#    if defined(__GNUC__)
#      pragma GCC diagnostic pop
#    endif
    // Win8 is 6.2; >=6.2 means Win8 or greater.
    return (osvi.dwMajorVersion > 6) ||
           (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 2);
}
#  endif
#endif

// --- BEGIN #23 read-salvage TRACE (revertible; compiled out unless -DUC_RSD_TRACE -> ZERO shipping
// perf/size impact; grep "#23 read-salvage"). Writes "<event> <path>" lines to $UC_RSD_TRACE_PATH so a
// test can see EXACTLY which open/close/unlink path a read-fault skip takes, without gdb on a fast race. ---
#ifdef UC_RSD_TRACE
#include <cstdio>
#include <cstdlib>
#define UC_RSD(ev,p) do{ const char*pa=getenv("UC_RSD_TRACE_PATH"); FILE*f=fopen(pa?pa:"/tmp/uc_rsd.log","a"); \
    if(f){ fprintf(f,"WT %s %s\n",(ev),TransferThread::internalStringTostring(p).c_str()); fclose(f);} }while(0)
#else
#define UC_RSD(ev,p) do{}while(0)
#endif
// --- END #23 read-salvage TRACE ---

unsigned int WriteThread::numberOfBlock=ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;

WriteThread::WriteThread()
{
    deletePartiallyTransferredFiles = true;
    lastGoodPosition                = 0;
    stopIt                          = false;
    isOpen.release();
    moveToThread(this);
    setObjectName(QStringLiteral("write"));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status                            = Idle;
    #endif
    putInPause                      = false;
    needRemoveTheFile               = false;
    salvageBufferedOnClose          = false;   // #23 read-salvage-drain (revertible)
    start();

    #ifdef Q_OS_UNIX
    to=-1;
    #else
    to=nullptr;
    #endif
    // Defensive init of members otherwise written only by setters/open(): prevents
    // uninitialised reads on early stop()/dtor/retry paths — id is logged in stop();
    // an uninitialised fakeMode could skip isOpen.release() and deadlock the dtor;
    // an uninitialised blockArray.data could be free()'d on an early stopIt exit.
    id=-1;
    os_spec_flags=false;
    buffer=false;
    postOperationRequested=false;
    writeFullBlocked=false;
    endDetected=false;
    fakeMode=false;
    destinationPreExisted=false;
    startSize=0;
    bytesWriten=0;
    blockArray.data=nullptr;
    blockArray.size=0;
    writeFileList=nullptr;
    writeFileListMutex=nullptr;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    numberOfBlockCopied=0;
    numberOfBlockCopied2=0;
    multiplicatorForBigSpeed=0;
    MultiForBigSpeed=0;
    multiForBigSpeed=0;
    #endif
}

WriteThread::~WriteThread()
{
    stopIt=true;
    needRemoveTheFile=true;
    pauseMutex.release();
    writeFull.release();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    waitNewClockForSpeed2.release();
    #endif
    writeFull.release();
    pauseMutex.release();
    // useless because stopIt will close all thread, but if thread not runing run it
    //endIsDetected();
    emit internalStartClose();
    isOpen.acquire();
    if(!file.empty())
        resumeNotStarted();
    //disconnect(this);//-> do into ~TransferThread()
    quit();
    wait();
}

void WriteThread::run()
{
    if(!connect(this,&WriteThread::internalStartOpen,               this,&WriteThread::internalOpen,		Qt::QueuedConnection))
        abort();
    /// in version 2, full close and retry from open(), see comment into TransferThreadAsync::retryAfterError()
    /*if(!connect(this,&WriteThread::internalStartReopen,             this,&WriteThread::internalReopen,		Qt::QueuedConnection))
        abort();*/
    if(!connect(this,&WriteThread::internalStartWrite,              this,&WriteThread::internalWrite,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartClose,              this,&WriteThread::internalCloseSlot,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartEndOfFile,          this,&WriteThread::internalEndOfFile,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartFlushAndSeekToZero,	this,&WriteThread::internalFlushAndSeekToZero,	Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::openWriteSend,	this,&WriteThread::openWriteInternal,	Qt::QueuedConnection))
        abort();
    exec();
}

//internal function
bool WriteThread::seek(const int64_t &position)/// \todo search if is use full
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with: "+std::to_string(position));
    if((int64_t)position>size())
        return false;

    #ifdef Q_OS_UNIX
    if(to<0)
        abort();//internal failure
    return lseek(to,position,SEEK_SET)==position;
    #else
    if(to==NULL)
        abort();//internal failure
    LARGE_INTEGER liSize;
    liSize.QuadPart=position;
    return SetFilePointerEx(to,liSize,NULL,FILE_BEGIN);
    #endif
}

int64_t WriteThread::size() const
{
    #ifdef Q_OS_UNIX
    struct stat st;
    fstat(to, &st);
    return st.st_size;
    #else
    LARGE_INTEGER lpFileSize;
    if(!GetFileSizeEx(to,&lpFileSize))
        return -1;
    else
        return lpFileSize.QuadPart;
    #endif
}

bool WriteThread::internalOpen()
{
    //do a bug
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] internalOpen destination: "+TransferThread::internalStringTostring(file));
    UC_RSD("ow-enter", file);   // #23 read-salvage trace
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        UC_RSD("ow-bail179-entry-stopit", file);   // #23 read-salvage trace: Case B (stopIt set before open)
        emit closed();
        return false;
    }
    #ifdef Q_OS_UNIX
    if(to>=0)
    #else
    if(to!=NULL)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already open! destination: "+TransferThread::internalStringTostring(file));
        return false;
    }
    if(file.empty())
    {
        errorString_internal=tr("Path resolution error (Empty path)").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+"Unable to open: "+TransferThread::internalStringTostring(file)+", error: Empty path");
        emit error();
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before the mutex");
    //set to LISTBLOCKSIZE
    while(writeFull.available()<(int)numberOfBlock)
        writeFull.release();
    if(writeFull.available()>(int)numberOfBlock)
        writeFull.acquire(writeFull.available()-numberOfBlock);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mutex");
    stopIt=false;
    endDetected=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=InodeOperation;
    #endif
    //mkpath check if exists and return true if already exists
    {
        INTERNALTYPEPATH destination=file;
        #ifdef WIDESTRING
        const size_t destinationIndex=destination.rfind(L'/');
        if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
        {
            const std::wstring &path=destination.substr(0,destinationIndex);
            if(!TransferThread::is_dir(path))
                if(!TransferThread::mkpath(path))
                {
                    #ifdef Q_OS_WIN32
                    errorString_internal=tr("Unable to create the destination folder: ").toStdString()+TransferThread::GetLastErrorStdStr();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable create the folder: "+
                        TransferThread::internalStringTostring(destination)+", error: "+
                        errorString_internal);
                    emit error();
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG
                    status=Idle;
                    #endif
                    return false;
                    #else
                    /// \todo do real folder error here
                    errorString_internal=tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+"Unable create the folder: "+
                                             TransferThread::internalStringTostring(destination)+", error: "+
                                 errorString_internal);
                    emit error();
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG
                    status=Idle;
                    #endif
                    return false;
                    #endif
                }
        }
        #else
        const size_t destinationIndex=destination.rfind('/');
        if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
        {
            const std::string &path=destination.substr(0,destinationIndex);
            if(!TransferThread::is_dir(path))
                if(!TransferThread::mkpath(path))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+path);
                    #ifdef Q_OS_WIN32
                    errorString_internal=tr("Unable to create the destination folder: ")+TransferThread::GetLastErrorStdStr();
                    #else
                    errorString_internal=tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString();
                    #endif
                    emit error();
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG
                    status=Idle;
                    #endif
                    return false;
                }
        }
        #endif
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mkpath");
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        emit closed();
        return false;
    }
    //try open it
    {
        QMutexLocker lock_mutex(writeFileListMutex);
        #ifdef WIDESTRING
        QString qtFile=QString::fromStdWString(file);
        #else
        QString qtFile=QString::fromStdString(file);
        #endif
        if(writeFileList->count(qtFile,this)==0)
        {
            writeFileList->insert(qtFile,this);
            if(writeFileList->count(qtFile)>1)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] in waiting because same file is found");
                return false;
            }
        }
    }
    bool fileWasExists=TransferThread::is_file(file);
    // Only protect a NON-EMPTY pre-existing dest (real user data). An empty file (0 bytes -- e.g. one a
    // previous put-to-end attempt created) has nothing to lose and must stay removable so dead-file
    // cleanup still works (this is the put-to-end retry regression of the first #9 attempt).
    destinationPreExisted = fileWasExists && (TransferThread::file_stat_size(file)>0);
    #ifdef Q_OS_UNIX
    // The last parameter (0755) does nothing. It is simply accepted for compatibility with the UNIX "open" function.
    // Needed for major build compatibility
    to = ::open(TransferThread::internalStringTostring(file).c_str(), O_WRONLY | O_CREAT, 0755);
    #else
    DWORD flags=FILE_ATTRIBUTE_NORMAL;
    if(os_spec_flags)
        flags|=FILE_FLAG_SEQUENTIAL_SCAN;
    // Bypass cache on pre-Win8 (cache thrashing on XP with large files); Win8+ rejects it (err 87) unless writes are sector-aligned.
    if(!buffer && !IsWindows8OrGreater())
        flags|=FILE_FLAG_NO_BUFFERING;
    to=CreateFileW(file.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
                   flags,NULL);
    #endif
    #ifdef Q_OS_UNIX
    if(to>=0)
    #else
    if(to!=INVALID_HANDLE_VALUE)
    #endif
    {
        #ifdef Q_OS_WIN32
        // Mark the destination as sparse so runs of zeros do not consume real disk space.
        // NTFS-only (silently fails on FAT/exFAT, which is fine); supported since Win2000.
        DWORD sparseBytesReturned=0;
        ::DeviceIoControl(to,FSCTL_SET_SPARSE,NULL,0,NULL,0,&sparseBytesReturned,NULL);
        #endif
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the open");
        UC_RSD("ow-opened", file);   // #23 read-salvage trace: ::open succeeded, fd created
        {
            QMutexLocker lock_mutex(&accessList);
            if(!theBlockList.empty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] General file corruption detected");
                UC_RSD("ow-bail330-buffer-nonempty-at-open", file);   // #23 read-salvage trace
                stopIt=true;
                #ifdef Q_OS_UNIX
                if(::close(to)!=0)
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
                to=-1;
                #else
                if(CloseHandle(to)==0)
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
                to=NULL;
                #endif
                resumeNotStarted();
                this->file.clear();
                return false;
            }
        }
        pauseMutex.tryAcquire(pauseMutex.available());
        #ifdef Q_OS_LINUX
        if(os_spec_flags)
        {
            posix_fadvise(to, 0, 0, POSIX_FADV_NOREUSE);
            posix_fadvise(to, 0, 0, POSIX_FADV_SEQUENTIAL);
        }
        #endif
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the pause mutex");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            resumeNotStarted();
            this->file.clear();
            emit closed();
            return false;
        }
        // Data-loss guard: for a FRESH overwrite (startSize==0) do NOT truncate the dest to 0 here --
        // that destroys the pre-existing destination BEFORE the source is confirmed readable, so a
        // source that vanishes/errors before the first byte leaves the user with NOTHING (cp/rsync open
        // the source first and never touch the dest on failure). The dest is opened O_WRONLY|O_CREAT
        // (no O_TRUNC); writes overwrite from offset 0 and the close path truncates to lastGoodPosition
        // (bytes actually written), removing any old tail on success. RESUME (startSize>0) still
        // truncates here to discard bytes past the resume point.
        if(startSize!=0 && destTruncate(startSize)!=0)
        {
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            resumeNotStarted();
            this->file.clear();
            #ifdef Q_OS_WIN32
            errorString_internal=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to seek after resize: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal
                                     );
            #else
            int t=errno;
            errorString_internal=strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to seek after resize: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                     );
            #endif
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            resumeNotStarted();
            this->file.clear();
            emit closed();
            return false;
        }
        if(!seek(0))
        {
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            resumeNotStarted();
            this->file.clear();
            #ifdef Q_OS_WIN32
            errorString_internal=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to seek after open: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal
                                     );
            #else
            int t=errno;
            errorString_internal=strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to seek after open: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                     );
            #endif
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            resumeNotStarted();
            this->file.clear();
            emit closed();
            return false;
        }
        isOpen.acquire();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit opened()");
        UC_RSD("ow-success", file);   // #23 read-salvage trace: open fully succeeded -> write loop will run
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        needRemoveTheFile=false;
        postOperationRequested=false;
        return true;
    }
    else
    {
        #ifdef Q_OS_WIN32
        errorString_internal=TransferThread::GetLastErrorStdStr();
        #else
        int t=errno;
        #endif
        if(!fileWasExists && TransferThread::is_file(file))
            if(unlink(TransferThread::internalStringTostring(file).c_str())!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] file created but can't be removed");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            resumeNotStarted();
            this->file.clear();
            emit closed();
            return false;
        }
        #ifdef Q_OS_WIN32
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                             "Unable to open: "+TransferThread::internalStringTostring(file)+
                             ", error: "+errorString_internal
                             );
        #else
        errorString_internal=strerror(t);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                             "Unable to open: "+TransferThread::internalStringTostring(file)+
                             ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                             );
        #endif
        emit error();
        // A failed write-OPEN means the destination is NOT open. Emit closed() too so the transfer
        // thread sets writeIsClosedVariable (-> remainDestinationOpen()==false). Without it the write
        // never "closes" (closed() was emitted only on the stopIt branch above), so
        // checkIfAllIsClosedAndDoOperations() blocks forever on remainFileOpen() and an unwritable /
        // read-only destination with fileError=Skip HANGS the whole job (move_destination_error).
        // Queued AFTER error() so write_error() sets writeError before write_closed() runs the
        // completion check (which then correctly waits for the skip/retry decision via its gate).
        emit closed();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        return false;
    }
}

void WriteThread::openWrite(const INTERNALTYPEPATH &file, const uint64_t &startSize)
{
    emit openWriteInternal(file,startSize);
}

void WriteThread::openWriteInternal(const INTERNALTYPEPATH &file, const uint64_t &startSize)
{
    if(!isRunning())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the thread not running to open destination: "+TransferThread::internalStringTostring(file)+", numberOfBlock: "+std::to_string(numberOfBlock));
        errorString_internal=tr("Internal error, please report it!").toStdString();
        emit error();
        return;
    }
    #ifdef Q_OS_WIN32
    if(to!=NULL)
    #else
    if(to>=0)
    #endif
    {
        if(file==this->file)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try reopen already opened same file: "+TransferThread::internalStringTostring(file));
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already open: "+TransferThread::internalStringTostring(file));
        //emit internalStartClose();
        internalCloseSlot();
        isOpen.acquire();
        isOpen.release();
    }
    if(numberOfBlock<1 || (numberOfBlock>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK && numberOfBlock>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] numberOfBlock wrong, set to default");
        this->numberOfBlock=ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    }
    /*else
        this->numberOfBlock=numberOfBlock;*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+"open destination: "+TransferThread::internalStringTostring(file));
    stopIt=false;
    fakeMode=false;
    lastGoodPosition=0;
    salvageBufferedOnClose=false;   // #23 read-salvage-drain (revertible): reset per file
    this->file=file;
    this->startSize=startSize;
    endDetected=false;
    writeFullBlocked=false;
    emit internalStartOpen();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    numberOfBlockCopied=0;
    #endif
}

void WriteThread::endIsDetected()
{
    if(endDetected)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    endDetected=true;
    pauseMutex.release();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    emit internalStartEndOfFile();
}

std::string WriteThread::errorString() const
{
    return errorString_internal;
}

void WriteThread::stop(bool finalNoRetry)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop()");
    // PURE skip (fileError=Skip, finalNoRetry) KEEPS the readable salvage already written off the bad
    // sector ("copy every readable byte"); put-to-end / cancel still mark the partial for removal.
    needRemoveTheFile = !finalNoRetry;
    // --- BEGIN #23 read-salvage-drain (revertible) ---
    // A PURE read-fault skip keeps the buffered read-ahead so internalClose() drains it into the kept
    // partial (max salvage off a dying disk). put-to-end / cancel (finalNoRetry=false) do not salvage.
    salvageBufferedOnClose = finalNoRetry;
    UC_RSD(finalNoRetry?"stop-finalNoRetry(salvage)":"stop-retry/cancel", file);   // #23 read-salvage trace
    // --- END #23 read-salvage-drain ---
    stopIt=true;
    if(isOpen.available()>0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] isOpen.available()>0");

        if(finalNoRetry)
        {
            // PURE skip, no put-to-end retry => NO reopen to race, so do NOT pre-close the fd. Wake the
            // (possibly parked, open-but-idle) write thread and let internalClose() run its NORMAL close --
            // truncate to lastGoodPosition + close + emit closed() -- which PERSISTS the readable salvage
            // (needRemoveTheFile is false above) AND completes the inode so the cap=1 large-transfer slot
            // frees for the next large file (faulty_hdd over_1mib.dat / skip_drops_multichunk /
            // opt_delete_partial_files). If the destination was NEVER opened (read faulted at byte 0),
            // internalClose() guards its emit on to>=0 and would hang, so signal closed() explicitly.
            #ifdef Q_OS_WIN32
            const bool destWasOpen=(to!=NULL);
            #else
            const bool destWasOpen=(to>=0);
            #endif
            writeFull.release();
            pauseMutex.release();
            pauseMutex.release();
            #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
            waitNewClockForSpeed.release();
            waitNewClockForSpeed2.release();
            #endif
            emit internalStartClose();
            if(!destWasOpen)
                emit closed();
            return;
        }

        #ifdef Q_OS_WIN32
        if(to!=NULL)
        #else
        if(to>=0)
        #endif
        {
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
        }

        // Put-to-end RETRY: pre-close the fd so internalClose() will NOT re-emit closed() and the retry's
        // reopen (isOpen.acquire) is not raced (transient_sector); emit internalStartClose() to release
        // isOpen and wake the parked write thread. The inode completes via the eventual successful retry.
        emit internalStartClose();
        return;
    }
    writeFull.release();
    pauseMutex.release();
    pauseMutex.release();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    waitNewClockForSpeed2.release();
    #endif
    // useless because stopIt will close all thread, but if thread not runing run it
    endIsDetected();
    //for the stop for skip: void TransferThread::skip()
    emit internalStartClose();
}

void WriteThread::flushBuffer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeFull.release();
    writeFull.acquire();
    pauseMutex.release();
    {
        QMutexLocker lock_mutex(&accessList);
        while(!theBlockList.empty())
        {
            free(theBlockList.front().data);
            theBlockList.pop();
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop");
}

/// \brief buffer is empty
bool WriteThread::bufferIsEmpty()
{
    bool returnVal;
    {
        QMutexLocker lock_mutex(&accessList);
        returnVal=theBlockList.empty();
    }
    return returnVal;
}

void WriteThread::internalEndOfFile()
{
    if(!bufferIsEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] buffer is not empty!");
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeIsStopped");
        postOperation();
        emit writeIsStopped();
    }
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
/*! \brief Set the max speed
\param tempMaxSpeed Set the max speed in KB/s, 0 for no limit */
void WriteThread::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    this->multiForBigSpeed=multiForBigSpeed;
    waitNewClockForSpeed.release();
    waitNewClockForSpeed2.release();
}

/// \brief For give timer every X ms
void WriteThread::timeOfTheBlockCopyFinished()
{
    /* this is the old way to limit the speed, it product blocking
     *if(waitNewClockForSpeed.available()<ULTRACOPIER_PLUGIN_NUMSEMSPEEDMANAGEMENT)
        waitNewClockForSpeed.release();*/

    //try this new way:
    /* only if speed limited, else will accumulate waitNewClockForSpeed
     * Disabled because: Stop call of this method when no speed limit
    if(this->maxSpeed>0)*/
    if(waitNewClockForSpeed.available()<=1)
        waitNewClockForSpeed.release();
    if(waitNewClockForSpeed2.available()<=1)
        waitNewClockForSpeed2.release();
}
#endif

void WriteThread::resumeNotStarted()
{
    QMutexLocker lock_mutex(writeFileListMutex);
    #ifdef WIDESTRING
    QString qtFile=QString::fromStdWString(file);
    #else
    QString qtFile=QString::fromStdString(file);
    #endif
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!writeFileList->contains(qtFile))
    {
        /*ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] file: \""+
                                 TransferThread::internalStringTostring(file)+
                                 "\" for similar inode is not located into the list of "+
                                 std::to_string(writeFileList.size())+" items!");*/
        return;
    }
    #endif
    writeFileList->remove(qtFile,this);
    if(writeFileList->contains(qtFile))
    {
        QList<WriteThread *> writeList=writeFileList->values(qtFile);
        if(!writeList.isEmpty())
           writeList.first()->reemitStartOpen();
        return;
    }
}

void WriteThread::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try put read thread in pause");
    pauseMutex.tryAcquire(pauseMutex.available());
    putInPause=true;
    return;
}

void WriteThread::resume()
{
    if(putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
        putInPause=false;
        stopIt=false;
    }
    else
        return;
    #ifdef Q_OS_WIN32
    if(to==NULL)
    #else
    if(to<0)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] file is not open");
        return;
    }
    pauseMutex.release();
}

void WriteThread::reemitStartOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] start");
    emit internalStartOpen();
}

void WriteThread::postOperation()
{
    if(postOperationRequested)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    postOperationRequested=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    emit internalStartClose();
}

void WriteThread::internalCloseSlot()
{
    internalClose();
}

void WriteThread::internalClose(bool emitSignal)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close for file: "+TransferThread::internalStringTostring(file));
    UC_RSD("close-enter", file);   // #23 read-salvage trace: internalClose reached (the drain path)
    /// \note never send signal here, because it's called by the destructor
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Close;
    #endif
    bool emit_closed=false;
    if(!fakeMode)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] !fakeMode: "+TransferThread::internalStringTostring(file));
        #ifdef Q_OS_WIN32
        if(to!=NULL)
        #else
        if(to>=0)
        #endif
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] to>=0: "+TransferThread::internalStringTostring(file));
            if(!needRemoveTheFile)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] !needRemoveTheFile: "+TransferThread::internalStringTostring(file));
                // --- BEGIN #23 read-salvage-drain (revertible -- grep "#23 read-salvage") ---
                // Pure read-fault skip: skip() kept theBlockList (did not flushBuffer); drain the buffered
                // read-ahead (already-read good bytes that out-paced the write) so the kept partial = ALL
                // readable bytes, not just the lagging write position. Gated to OUR fresh dest
                // (!destinationPreExisted); seek() first to stay consistent with lastGoodPosition. Other
                // close paths leave salvageBufferedOnClose==false -> strict no-op.
                if(salvageBufferedOnClose)
                {
                    QMutexLocker lock_mutex(&accessList);
                    UC_RSD("close-drain-begin", file);
                    bool drainOk = !destinationPreExisted && seek((int64_t)lastGoodPosition);
                    while(!theBlockList.empty())
                    {
                        DataBlock blk=theBlockList.front();
                        theBlockList.pop();
                        if(drainOk && blk.data!=NULL && blk.size>0)
                        {
                            #ifdef Q_OS_WIN32
                            DWORD wr=0;
                            if(WriteFile(to,blk.data,blk.size,&wr,NULL) && wr==blk.size)
                                lastGoodPosition+=wr;
                            else
                                drainOk=false;
                            #else
                            const ssize_t wr=::write(to,blk.data,blk.size);
                            if(wr==(ssize_t)blk.size)
                                lastGoodPosition+=(uint64_t)wr;
                            else
                                drainOk=false;
                            #endif
                        }
                        if(blk.data!=NULL)
                            free(blk.data);
                    }
                }
                // --- END #23 read-salvage-drain ---
                if(startSize!=lastGoodPosition)
                    if(destTruncate(lastGoodPosition)!=0)
                    {
                        if(emitSignal)
                        {
                            #ifdef Q_OS_WIN32
                            errorString_internal=TransferThread::GetLastErrorStdStr();
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                                     "Unable to resize: "+TransferThread::internalStringTostring(file)+
                                                     ", error: "+errorString_internal
                                                     );
                            #else
                            int t=errno;
                            errorString_internal=strerror(t);
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                                     "Unable to resize: "+TransferThread::internalStringTostring(file)+
                                                     ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                                     );
                            #endif
                            emit error();
                        }
                        else
                            needRemoveTheFile=true;
                    }
            }
            #ifdef Q_OS_UNIX
            if(::close(to)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            to=-1;
            #else
            if(CloseHandle(to)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            to=NULL;
            #endif
            this->file.clear();
            if(needRemoveTheFile || stopIt)
            {
                if(deletePartiallyTransferredFiles)
                {
                    if(unlink(TransferThread::internalStringTostring(file).c_str())!=0)
                        if(emitSignal)
                        {
                            #ifdef Q_OS_UNIX
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to remove the destination file: "+std::to_string(errno));
                            #else
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to remove the destination file: "+TransferThread::GetLastErrorStdStr());
                            #endif
                        }
                }
            }
            //here and not after, because the transferThread don't need try close if not open
            if(emitSignal)
                emit_closed=true;
        }
    }
    else
    {
        //here and not after, because the transferThread don't need try close if not open

        if(emitSignal)
            emit_closed=true;
    }
    needRemoveTheFile=false;
    resumeNotStarted();
    this->file.clear();
    if(emit_closed)
        emit closed();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Idle;
    #endif

    /// \note always the last of this function
    if(!fakeMode)
        isOpen.release();
}

/// in version 2, full close and retry from open(), see comment into TransferThreadAsync::retryAfterError()
/*void WriteThread::internalReopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    INTERNALTYPEPATH tempFile=file;
    internalClose(false);
    flushBuffer();
    stopIt=false;
    lastGoodPosition=0;
    file=tempFile;
    if(internalOpen())
        emit reopened();
}*/

/// in version 2, full close and retry from open(), see comment into TransferThreadAsync::retryAfterError()
/*void WriteThread::reopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    stopIt=true;
    endDetected=false;
    emit internalStartReopen();
}*/

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void WriteThread::setId(int id)
{
    this->id=id;
}
#endif

/// \brief do the fake open
void WriteThread::fakeOpen()
{
    fakeMode=true;
    postOperationRequested=false;
    emit opened();
}

/// \brief do the fake writeIsStarted
void WriteThread::fakeWriteIsStarted()
{
    emit writeIsStarted();
}

/// \brief do the fake writeIsStopped
void WriteThread::fakeWriteIsStopped()
{
    postOperation();
    emit writeIsStopped();
}

/** \brief set block size
\param block the new block size in B
\return Return true if succes */
bool WriteThread::setBlockSize(const int blockSize)
{
    //can be smaller than min block size to do correct speed limitation
    if(blockSize>1 && blockSize<ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE*1024)
    {
        //this->blockSize=blockSize;
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"block size out of range: "+std::to_string(blockSize));
        return false;
    }
}

/// \brief get the last good position
int64_t WriteThread::getLastGoodPosition() const
{
    return lastGoodPosition;
}

bool WriteThread::getDestinationPreExisted() const
{
    return destinationPreExisted;
}

void WriteThread::flushAndSeekToZero()
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flushAndSeekToZero: "+std::to_string(blockSize));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flushAndSeekToZero");
    stopIt=true;
    emit internalStartFlushAndSeekToZero();
}

void WriteThread::internalFlushAndSeekToZero()
{
    flushBuffer();
    if(!seek(0))
    {
        #ifdef Q_OS_WIN32
        errorString_internal=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                 "Unable to seek: "+TransferThread::internalStringTostring(file)+
                                 ", error: "+errorString_internal
                                 );
        #else
        int t=errno;
        errorString_internal=strerror(t);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                 "Unable to seek: "+TransferThread::internalStringTostring(file)+
                                 ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                 );
        #endif
        emit error();
        return;
    }
    stopIt=false;
    emit flushedAndSeekedToZero();
}

void WriteThread::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
}

bool WriteThread::write(char * data, const unsigned int size)
{
    if(stopIt)
        return false;
    bool atMax;
    if(stopIt)
        return false;
    {
        QMutexLocker lock_mutex(&accessList);
        DataBlock d;
        d.data=data;
        d.size=size;
        theBlockList.push(d);
        atMax=(theBlockList.size()>=numberOfBlock);
    }
    emit internalStartWrite();
    if(atMax)
    {
        writeFullBlocked=true;
        writeFull.acquire();
        writeFullBlocked=false;
    }
    if(stopIt)
        return false;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    //wait for limitation speed if stop not query
    if(multiForBigSpeed>0)
    {
        numberOfBlockCopied2++;
        if(numberOfBlockCopied2>=multiForBigSpeed)
        {
            numberOfBlockCopied2=0;
            waitNewClockForSpeed2.acquire();
        }
    }
    #endif
    if(stopIt)
        return false;
    return true;
}

void WriteThread::internalWrite()
{
    bool haveBlock;
    do
    {
        if(putInPause)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] write put in pause");
            if(stopIt)
                return;
            pauseMutex.acquire();
            if(stopIt)
                return;
        }
        if(stopIt)
        {
//            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt");
            return;
        }
        if(stopIt)
            return;
        //read one block
        {
            QMutexLocker lock_mutex(&accessList);
            if(theBlockList.empty())
                haveBlock=false;
            else
            {
                blockArray=theBlockList.front();
                #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
                if(multiForBigSpeed>0)
                {
                    theBlockList.pop();
                    //if remove one block
                    writeFull.release();
                }
                else
                #endif
                {
                    theBlockList.pop();
                    //if remove one block
                    writeFull.release();
                }
                haveBlock=true;
            }
        }
        if(stopIt)
        {
            if(blockArray.data!=NULL)
            {
                free(blockArray.data);
                blockArray.data=NULL;
            }
            return;
        }
        if(!haveBlock)
        {
            //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] End detected of the file");
            if(blockArray.data!=NULL)
            {
                free(blockArray.data);
                blockArray.data=NULL;
            }
            return;
        }
        #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
        //wait for limitation speed if stop not query
        if(multiForBigSpeed>0)
        {
            numberOfBlockCopied++;
            if(writeFullBlocked)
            {
                if(numberOfBlockCopied>=(multiForBigSpeed*2))
                {
                    numberOfBlockCopied=0;
                    waitNewClockForSpeed.acquire();
                    if(stopIt)
                    {
                        free(blockArray.data);
                        break;
                    }
                }
            }
            else
            {
                if(numberOfBlockCopied>=multiForBigSpeed)
                {
                    numberOfBlockCopied=0;
                    waitNewClockForSpeed.acquire();
                    if(stopIt)
                    {
                        if(blockArray.data!=NULL)
                        {
                            free(blockArray.data);
                            blockArray.data=NULL;
                        }
                        break;
                    }
                }
            }
        }
        #endif
        if(stopIt)
        {
            if(blockArray.data!=NULL)
            {
                free(blockArray.data);
                blockArray.data=NULL;
            }
            return;
        }
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Write;
        #endif

        #ifdef Q_OS_WIN32
        BOOL retRead=TRUE;
        #endif
        if(blockArray.size<=0)
            bytesWriten=0;
        else
        {
            #ifdef Q_OS_WIN32
            DWORD lpNumberOfBytesWrite=0;
            retRead=WriteFile(to,blockArray.data,blockArray.size,
                                         &lpNumberOfBytesWrite,NULL);
            bytesWriten=lpNumberOfBytesWrite;
            #else
            bytesWriten=::write(to,blockArray.data,blockArray.size);
            #endif
        }
        if(blockArray.data!=NULL)
        {
            free(blockArray.data);
            blockArray.data=NULL;
        }

        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        //mutex for stream this data
        if(lastGoodPosition==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit writeIsStarted()");
            emit writeIsStarted();
        }
        if(stopIt)
            return;
        #ifdef Q_OS_WIN32
        if(retRead==FALSE)
        #else
        if(bytesWriten<0)
        #endif
        {
            #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            DWORD e = GetLastError();
            #endif
            errorString_internal=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to write: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(e)+") to write "+std::to_string(blockArray.size)+"B at "+std::to_string(lastGoodPosition)+"B"
                                     );
            #else
            int t=errno;
            errorString_internal=strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to write: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(t)+") to write "+std::to_string(blockArray.size)+"B at "+std::to_string(lastGoodPosition)+"B"
                                     );
            #endif
            stopIt=true;
            emit error();
            /// in version 2, full close and retry from open(), see comment into TransferThreadAsync::retryAfterError()
            internalClose(false);
            flushBuffer();
            return;
        }
        if(bytesWriten!=blockArray.size)
        {
            #ifdef Q_OS_WIN32
            std::string eStr=TransferThread::GetLastErrorStdStr();
            #else
            int t=errno;
            std::string eStr=strerror(t);
            #endif
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size).toStdString()+", "+eStr);
            errorString_internal=QStringLiteral("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size).toStdString()+", "+eStr;
            stopIt=true;
            emit error();
            /// in version 2, full close and retry from open(), see comment into TransferThreadAsync::retryAfterError()
            internalClose(false);
            flushBuffer();
            return;
        }
        lastGoodPosition+=bytesWriten;
    } while(true);
}

//return 0 if sucess
int WriteThread::destTruncate(const uint64_t &startSize)
{
    #ifdef Q_OS_WIN32
    if(to==NULL)
        abort();
    seek(startSize);
    if(SetEndOfFile(to)!=0)
        return 0;
    else
        return 1;
    #else
    if(to<0)
        abort();
    return ::ftruncate(to,startSize);
    #endif
}

void WriteThread::setOsSpecFlags(bool os_spec_flags)
{
    this->os_spec_flags=os_spec_flags;
}
