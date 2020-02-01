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
    numberOfBlock                   = ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    putInPause                      = false;
    needRemoveTheFile               = false;
    start();

    #ifdef Q_OS_UNIX
    to=-1;
    #else
    to=nullptr;
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
    if(!connect(this,&WriteThread::internalStartReopen,             this,&WriteThread::internalReopen,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartWrite,              this,&WriteThread::internalWrite,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartClose,              this,&WriteThread::internalCloseSlot,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartEndOfFile,          this,&WriteThread::internalEndOfFile,		Qt::QueuedConnection))
        abort();
    if(!connect(this,&WriteThread::internalStartFlushAndSeekToZero,	this,&WriteThread::internalFlushAndSeekToZero,	Qt::QueuedConnection))
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
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
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
    while(writeFull.available()<numberOfBlock)
        writeFull.release();
    if(writeFull.available()>numberOfBlock)
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
    #ifdef Q_OS_UNIX
    to = ::open(TransferThread::internalStringTostring(file).c_str(), O_WRONLY | O_CREAT);
    #else
    DWORD flags=FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    if(buffer)
        flags|=FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING;
    to=CreateFileW(file.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
                   flags,NULL);
    #endif
    #ifdef Q_OS_UNIX
    if(to>=0)
    #else
    if(to!=INVALID_HANDLE_VALUE)
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the open");
        {
            QMutexLocker lock_mutex(&accessList);
            if(!theBlockList.isEmpty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] General file corruption detected");
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
        posix_fadvise(to, 0, 0, POSIX_FADV_NOREUSE);
        posix_fadvise(to, 0, 0, POSIX_FADV_SEQUENTIAL);
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
        if(destTruncate(startSize)!=0)
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
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        return false;
    }
}

void WriteThread::open(const INTERNALTYPEPATH &file, const uint64_t &startSize)
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
        emit internalStartClose();
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

void WriteThread::stop()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop()");
    needRemoveTheFile=true;
    stopIt=true;
    if(isOpen.available()>0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] isOpen.available()>0");

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
        theBlockList.clear();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop");
}

/// \brief buffer is empty
bool WriteThread::bufferIsEmpty()
{
    bool returnVal;
    {
        QMutexLocker lock_mutex(&accessList);
        returnVal=theBlockList.isEmpty();
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

void WriteThread::internalReopen()
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
}

void WriteThread::reopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    stopIt=true;
    endDetected=false;
    emit internalStartReopen();
}

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

bool WriteThread::write(const QByteArray &data)
{
    if(stopIt)
        return false;
    bool atMax;
    if(stopIt)
        return false;
    {
        QMutexLocker lock_mutex(&accessList);
        theBlockList.append(data);
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
            if(theBlockList.isEmpty())
                haveBlock=false;
            else
            {
                blockArray=theBlockList.first();
                #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
                if(multiForBigSpeed>0)
                {
                    theBlockList.removeFirst();
                    //if remove one block
                    writeFull.release();
                }
                else
                #endif
                {
                    theBlockList.removeFirst();
                    //if remove one block
                    writeFull.release();
                }
                haveBlock=true;
            }
        }
        if(stopIt)
            return;
        if(!haveBlock)
        {
            //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] End detected of the file");
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
                        break;
                }
            }
            else
            {
                if(numberOfBlockCopied>=multiForBigSpeed)
                {
                    numberOfBlockCopied=0;
                    waitNewClockForSpeed.acquire();
                    if(stopIt)
                        break;
                }
            }
        }
        #endif
        if(stopIt)
            return;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Write;
        #endif

        #ifdef Q_OS_WIN32
        BOOL retRead=TRUE;
        #endif
        if(blockArray.size()<=0)
            bytesWriten=0;
        else
        {
            #ifdef Q_OS_WIN32
            DWORD lpNumberOfBytesWrite=0;
            retRead=WriteFile(to,blockArray.data(),blockArray.size(),
                                         &lpNumberOfBytesWrite,NULL);
            bytesWriten=lpNumberOfBytesWrite;
            #else
            bytesWriten=::write(to,blockArray.data(),blockArray.size());
            #endif
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
            DWORD e = GetLastError();
            errorString_internal=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to write: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(e)+") to write "+std::to_string(blockArray.size())+"B at "+std::to_string(lastGoodPosition)+"B"
                                     );
            #else
            int t=errno;
            errorString_internal=strerror(t);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+
                                     "Unable to write: "+TransferThread::internalStringTostring(file)+
                                     ", error: "+errorString_internal+" ("+std::to_string(t)+") to write "+std::to_string(blockArray.size())+"B at "+std::to_string(lastGoodPosition)+"B"
                                     );
            #endif
            stopIt=true;
            emit error();
            return;
        }
        if(bytesWriten!=blockArray.size())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size()).toStdString());
            errorString_internal=QStringLiteral("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size()).toStdString();
            stopIt=true;
            emit error();
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
