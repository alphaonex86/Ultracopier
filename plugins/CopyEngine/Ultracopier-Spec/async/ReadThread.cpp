#include "ReadThread.h"
#include "../TransferThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

ReadThread::ReadThread()
{
    start();
    moveToThread(this);
    stopIt=false;
    putInPause=false;
    blockSize=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    setObjectName(QStringLiteral("read"));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Idle;
    #endif
    isInReadLoop=false;
    tryStartRead=false;
    lastGoodPosition=0;
    isOpen.release();

    #ifdef Q_OS_UNIX
    from=-1;
    #else
    from=nullptr;
    #endif
}

ReadThread::~ReadThread()
{
    stopIt=true;
    //disconnect(this);//-> do into ~TransferThread()
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    pauseMutex.release();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    pauseMutex.release();
    //if(isOpen.available()<=0)
        emit internalStartClose();
    isOpen.acquire();
    exit();
    wait();
}

void ReadThread::run()
{
    if(!connect(this,&ReadThread::internalStartOpen,		this,&ReadThread::internalOpenSlot,     Qt::QueuedConnection))
        abort();
    if(!connect(this,&ReadThread::internalStartReopen,		this,&ReadThread::internalReopen,       Qt::QueuedConnection))
        abort();
    if(!connect(this,&ReadThread::internalStartRead,		this,&ReadThread::internalRead,         Qt::QueuedConnection))
        abort();
    if(!connect(this,&ReadThread::internalStartClose,		this,&ReadThread::internalCloseSlot,	Qt::QueuedConnection))
        abort();
    if(!connect(this,&ReadThread::checkIfIsWait,            this,&ReadThread::isInWait,             Qt::QueuedConnection))
        abort();
    exec();
}

void ReadThread::open(const INTERNALTYPEPATH &file, const Ultracopier::CopyMode &mode)
{
    if(!isRunning())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the thread not running to open destination"+TransferThread::internalStringTostring(file));
        errorString_internal=tr("Internal error, please report it!").toStdString();
        emit error();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] open source: "+TransferThread::internalStringTostring(file));
    #ifdef Q_OS_UNIX
    if(from>=0)
    #else
    if(from!=NULL)
    #endif
    {
        if(file==this->file)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try reopen already opened same file: "+TransferThread::internalStringTostring(file));
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already open: "+TransferThread::internalStringTostring(this->file));
        abort();
        emit internalStartClose();
        isOpen.acquire();
        isOpen.release();
    }
    if(isInReadLoop)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already readding: "+TransferThread::internalStringTostring(file));
        return;
    }
    if(tryStartRead)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already try read: "+TransferThread::internalStringTostring(file));
        return;
    }
    stopIt=false;
    fakeMode=false;
    lastGoodPosition=0;
    this->file=file;
    this->mode=mode;
    emit internalStartOpen();
}

std::string ReadThread::errorString() const
{
    return errorString_internal;
}

void ReadThread::stop()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop()");
    stopIt=true;
    pauseMutex.release();
    pauseMutex.release();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    if(isOpen.available()<=0)
        emit internalStartClose();
}

void ReadThread::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try put read thread in pause");
    if(stopIt)
        return;
    pauseMutex.tryAcquire(pauseMutex.available());
    putInPause=true;
}

void ReadThread::resume()
{
    if(putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
        putInPause=false;
        stopIt=false;
    }
    else
        return;
    #ifdef Q_OS_UNIX
    if(from<0)
    #else
    if(from==NULL)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] file is not open");
        return;
    }
    pauseMutex.release();
}

bool ReadThread::seek(const int64_t &position)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with: "+std::to_string(position));
    if((int64_t)position>size())
        return false;

    #ifdef Q_OS_UNIX
    if(from<0)
        abort();//internal failure
    return lseek(from,position,SEEK_SET)==position;
    #else
    if(from==NULL)
        abort();//internal failure
    LARGE_INTEGER liSize;
    liSize.QuadPart=position;
    return SetFilePointerEx(from,liSize,NULL,FILE_BEGIN);
    #endif
}

int64_t ReadThread::size() const
{
    #ifdef Q_OS_UNIX
    struct stat st;
    if(fstat(from, &st)==0)
        return st.st_size;
    else
    {
        struct stat source_statbuf;
        #ifdef Q_OS_UNIX
        if(lstat(TransferThread::internalStringTostring(file).c_str(), &source_statbuf)==0)
        #else
        if(stat(TransferThread::internalStringTostring(file).c_str(), &source_statbuf)==0)
        #endif
            return source_statbuf.st_size;
        else
            return -1;
    }
    #else
    LARGE_INTEGER lpFileSize;
    if(!GetFileSizeEx(from,&lpFileSize))
    {
        WIN32_FILE_ATTRIBUTE_DATA sourceW;
        if(GetFileAttributesExW(file.c_str(),GetFileExInfoStandard,&sourceW))
        {
            uint64_t size=sourceW.nFileSizeHigh;
            size<<=32;
            size|=sourceW.nFileSizeLow;
            return size;
        }
        else
            return -1;
    }
    else
        return lpFileSize.QuadPart;
    #endif
}

void ReadThread::postOperation()
{
    emit internalStartClose();
}

bool ReadThread::internalOpenSlot()
{
    return internalOpen();
}

bool ReadThread::internalOpen(bool resetLastGoodPosition)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] internalOpen source: "+
                             TransferThread::internalStringTostring(file)+", open in write because move: "+std::to_string(mode==Ultracopier::Move));
    if(stopIt)
    {
        emit closed();
        return false;
    }
    putInPause=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=InodeOperation;
    #endif
    #ifdef Q_OS_UNIX
    if(from>=0)
    #else
    if(from!=NULL)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] this file is already open: "+TransferThread::internalStringTostring(file));
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        emit closed();
        return false;
    }
    /*can have permision to remove but not write
     * if(mode==Ultracopier::Move)
        openMode=QIODevice::ReadWrite;*/
    seekToZero=false;
    #ifdef Q_OS_UNIX
    from = ::open(TransferThread::internalStringTostring(file).c_str(), O_RDONLY);
    #else
    DWORD flags=FILE_ATTRIBUTE_NORMAL;
    if(os_spec_flags)
        flags|=FILE_FLAG_SEQUENTIAL_SCAN;
    from=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,flags,NULL);
    #endif
    #ifdef Q_OS_UNIX
    if(from>=0)
    #else
    if(from!=INVALID_HANDLE_VALUE)
    #endif
    {
        if(stopIt)
        {
            #ifdef Q_OS_UNIX
            if(::close(from)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            from=-1;
            #else
            if(CloseHandle(from)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            from=NULL;
            #endif
            this->file.clear();
            emit closed();
            return false;
        }
        pauseMutex.tryAcquire(pauseMutex.available());
        #ifdef Q_OS_LINUX
        if(os_spec_flags)
        {
            posix_fadvise(from, 0, 0, POSIX_FADV_WILLNEED);
            posix_fadvise(from, 0, 0, POSIX_FADV_SEQUENTIAL);
            posix_fadvise(from, 0, 0, POSIX_FADV_NOREUSE);
        }
        #endif
        if(stopIt)
        {
            #ifdef Q_OS_UNIX
            if(::close(from)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            from=-1;
            #else
            if(CloseHandle(from)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            from=NULL;
            #endif
            this->file.clear();
            emit closed();
            return false;
        }

        //do one at same time
        #ifdef Q_OS_UNIX
        {
            struct stat st;
            fstat(from, &st);
            size_at_open=st.st_size;
            #ifdef Q_OS_MAC
            mtime_at_open=st.st_mtimespec.tv_sec;
            #else
            mtime_at_open=st.st_mtim.tv_sec;
            #endif
        }
        #else
        {
            LARGE_INTEGER lpFileSize;
            GetFileSizeEx(from,&lpFileSize);
            size_at_open=lpFileSize.QuadPart;
            FILETIME LastWriteTime;
            GetFileTime(from,NULL,NULL,&LastWriteTime);
            mtime_at_open=LastWriteTime;
        }
        #endif

        putInPause=false;
        if(resetLastGoodPosition)
            lastGoodPosition=0;
        if(!seek(lastGoodPosition))
        {
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

            #ifdef Q_OS_UNIX
            if(::close(from)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            from=-1;
            #else
            if(CloseHandle(from)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            from=NULL;
            #endif
            this->file.clear();

            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif
            return false;
        }
        isOpen.acquire();
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        return true;
    }
    else
    {
        #ifdef Q_OS_WIN32
        from=NULL;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        DWORD e = GetLastError();
        #endif
        errorString_internal=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                 "Unable to open: "+TransferThread::internalStringTostring(file)+
                                 ", error: "+errorString_internal+" ("+std::to_string(e)+")"
                                 );
        #else
        int t=errno;
        errorString_internal=strerror(t);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                 "Unable to open: "+TransferThread::internalStringTostring(file)+
                                 ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                 );
        #endif

        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        return false;
    }
}

void ReadThread::internalRead()
{
    if(writeThread==nullptr)
        abort();
    isInReadLoop=true;
    tryStartRead=false;
    if(stopIt)
    {
        if(seekToZero &&
                #ifdef Q_OS_WIN32
                from!=NULL
                #else
                from>=0
                #endif
                )
        {
            stopIt=false;
            lastGoodPosition=0;
            seek(0);
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
            isInReadLoop=false;
            internalClose();
            return;
        }
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=InodeOperation;
    #endif
    int sizeReaden=0;
    #ifdef Q_OS_WIN32
    if(from==NULL)
    #else
    if(from<0)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] is not open!");
        isInReadLoop=false;
        return;
    }
    char * data=NULL;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    numberOfBlockCopied=0;
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start the copy");
    emit readIsStarted();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Idle;
    #endif
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
        isInReadLoop=false;
        internalClose();
        return;
    }
    do
    {
        //read one block
        if(putInPause)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] read put in pause");
            if(stopIt)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
                isInReadLoop=false;
                internalClose();
                return;
            }
            pauseMutex.acquire();
            if(stopIt)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
                isInReadLoop=false;
                internalClose();
                return;
            }
        }
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Read;
        #endif
        data=(char *)malloc(blockSize);
        if(data==NULL)
        {
            errorString_internal="Out of memory";
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to read the source file: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+"Out of memory"
                                     );
            isInReadLoop=false;
            emit error();
            return;
        }
        #ifdef Q_OS_WIN32
        DWORD lpNumberOfBytesRead=0;
        const BOOL retRead=ReadFile(from,data,blockSize,
                                    &lpNumberOfBytesRead,NULL);
        sizeReaden=lpNumberOfBytesRead;
        #else
        sizeReaden=::read(from,data,blockSize);
        #endif
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif

        #ifdef Q_OS_WIN32
        if(retRead==FALSE)
        #else
        if(sizeReaden<0)
        #endif
        {
            #ifdef Q_OS_WIN32
            errorString_internal=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to read the source file: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal
                                     );
            #else
            int t=errno;
            errorString_internal=tr("Unable to read the source file: ").toStdString()+strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to read the source file: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(t)+")"
                                     );
            #endif
            isInReadLoop=false;
            emit error();
            return;
        }
        if(sizeReaden>0)
        {
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=WaitWritePipe;
            #endif
            if(!writeThread->write(data,sizeReaden))//speed limitation here
            {
                #ifdef ULTRACOPIER_PLUGIN_DEBUG
                status=Idle;
                #endif
                if(!stopIt)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopped because the write is stopped: "+std::to_string(lastGoodPosition));
                    stopIt=true;
                }
            }

            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif

            if(stopIt)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
                isInReadLoop=false;
                internalClose();//need re-open the destination and then the source
                return;
            }
            lastGoodPosition+=sizeReaden;
        }
        /*
        if(lastGoodPosition>16*1024)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("[")+QString::number(id)+QStringLiteral("] ")+QStringLiteral("Test error in reading: %1 (%2)").arg(file.errorString()).arg(file.error()));
            errorString_internal=QStringLiteral("Test error in reading: %1 (%2)").arg(file.errorString()).arg(file.error());
            isInReadLoop=false;
            emit error();
            return;
        }
        */
    }
    while(sizeReaden>0 && !stopIt);
    if(lastGoodPosition>size())
    {
        errorString_internal=tr("File truncated during the read, possible data change").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                 "Source truncated during the read");
        isInReadLoop=false;
        emit error();
        return;
    }
    isInReadLoop=false;
    if(stopIt)
    {
        stopIt=false;
        return;
    }
    postOperation();
    emit readIsStopped();//will product by signal connection writeThread->endIsDetected();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop the read");
}

void ReadThread::startRead()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    if(tryStartRead)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] already in try start");
        return;
    }
    if(isInReadLoop)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
    else
    {
        tryStartRead=true;
        emit internalStartRead();
    }
}

void ReadThread::internalCloseSlot()
{
    internalClose();
}

void ReadThread::internalClose(bool callByTheDestructor)
{
    /// \note never send signal here, because it's called by the destructor
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[")+QString::number(id)+QStringLiteral("] start"));
    bool closeTheFile=false;
    if(!fakeMode)
    {
        #ifdef Q_OS_UNIX
        if(from>=0)
        #else
        if(from!=NULL)
        #endif
        {
            closeTheFile=true;
            #ifdef Q_OS_UNIX
            if(::close(from)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
            from=-1;
            #else
            if(CloseHandle(from)==0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
            from=NULL;
            #endif
            this->file.clear();
            isInReadLoop=false;
        }
    }
    if(!callByTheDestructor)
        emit closed();

    /// \note always the last of this function
    if(closeTheFile)
        isOpen.release();
}

/** \brief set block size
\param block the new block size in B
\return Return true if succes */
bool ReadThread::setBlockSize(const int blockSize)
{
    //can be smaller than min block size to do correct speed limitation
    if(blockSize>1 && blockSize<ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE*1024)
    {
        this->blockSize=blockSize;
        //set the new max speed because the timer have changed
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"block size out of range: "+std::to_string(blockSize));
        return false;
    }
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
/*! \brief Set the max speed
\param tempMaxSpeed Set the max speed in KB/s, 0 for no limit */
void ReadThread::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    this->multiForBigSpeed=multiForBigSpeed;
    waitNewClockForSpeed.release();
}

/// \brief For give timer every X ms
void ReadThread::timeOfTheBlockCopyFinished()
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
}
#endif

/// \brief do the fake open
void ReadThread::fakeOpen()
{
    fakeMode=true;
    emit opened();
}

/// \brief do the fake writeIsStarted
void ReadThread::fakeReadIsStarted()
{
    emit readIsStarted();
}

/// \brief do the fake writeIsStopped
void ReadThread::fakeReadIsStopped()
{
    postOperation();
    emit readIsStopped();
}

int64_t ReadThread::getLastGoodPosition() const
{
    /*if(lastGoodPosition>file.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("[")+QString::number(id)+QStringLiteral("] Bug, the lastGoodPosition is greater than the file size!"));
        return file.size();
    }
    else*/
    return lastGoodPosition;
}

//reopen after an error
void ReadThread::reopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    if(isInReadLoop)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] try reopen where read is not finish");
        return;
    }
    stopIt=true;
    emit internalStartReopen();
}

bool ReadThread::internalReopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    stopIt=false;
    #ifdef Q_OS_UNIX
    if(from>=0)
    #else
    if(from!=NULL)
    #endif
    {
        #ifdef Q_OS_UNIX
        if(::close(from)!=0)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+std::to_string(errno));
        from=-1;
        #else
        if(CloseHandle(from)==0)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to close: "+TransferThread::GetLastErrorStdStr());
        from=NULL;
        #endif
        this->file.clear();
        isOpen.release();
    }
    int64_t temp_size=-1;
    #ifdef Q_OS_UNIX
    uint64_t temp_mtime=0;
    {
        struct stat st;
        if(::stat(TransferThread::internalStringTostring(file).c_str(), &st)!=-1)
        {
            temp_size=st.st_size;
            #ifdef Q_OS_MAC
            temp_mtime=st.st_mtimespec.tv_sec;
            #else
            temp_mtime=st.st_mtim.tv_sec;
            #endif
        }
    }
    if(size_at_open!=temp_size || mtime_at_open!=temp_mtime)
    #else
    FILETIME temp_mtime;
    {
        LARGE_INTEGER FileSize;
        GetFileSizeEx(from,&FileSize);
        temp_size=FileSize.QuadPart;
        GetFileTime(from,NULL,NULL,&temp_mtime);
    }
    if(size_at_open!=temp_size || mtime_at_open.dwLowDateTime!=temp_mtime.dwLowDateTime || mtime_at_open.dwHighDateTime!=temp_mtime.dwHighDateTime)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source file have changed since the last open, restart all");
        //fix this function like the close function
        if(internalOpen(true))
        {
            emit resumeAfterErrorByRestartAll();
            return true;
        }
        else
            return false;
    }
    else
    {
        //fix this function like the close function
        if(internalOpen(false))
        {
            emit resumeAfterErrorByRestartAtTheLastPosition();
            return true;
        }
        else
            return false;
    }
    return false;
}

//set the write thread
void ReadThread::setWriteThread(WriteThread * writeThread)
{
    this->writeThread=writeThread;
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void ReadThread::setId(int id)
{
    this->id=id;
}
#endif

void ReadThread::seekToZeroAndWait()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    stopIt=true;
    seekToZero=true;
    emit checkIfIsWait();
}

void ReadThread::isInWait()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    if(seekToZero)
    {
        stopIt=false;
        seekToZero=false;
        #ifdef Q_OS_UNIX
        if(from>=0)
        #else
        if(from!=NULL)
        #endif
        {
            lastGoodPosition=0;
            seek(0);
        }
        else
            internalOpen(true);
        emit isSeekToZeroAndWait();
    }
}

bool ReadThread::isReading() const
{
    return isInReadLoop;
}

void ReadThread::setOsSpecFlags(bool os_spec_flags)
{
    this->os_spec_flags=os_spec_flags;
}
