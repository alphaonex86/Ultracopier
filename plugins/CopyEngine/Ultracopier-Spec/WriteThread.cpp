#include "WriteThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "EventLoop.h"
#include "ReadThread.h"

WriteThread::WriteThread()
{
    deletePartiallyTransferredFiles = true;
    lastGoodPosition                = 0;
    stopIt                          = false;
    setObjectName(QStringLiteral("write"));
    //this->mkpathTransfer            = mkpathTransfer;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status                            = Idle;
    #endif
    needRemoveTheFile               = false;
    blockArrayStart                 = BLOCKDEFAULTINITVAL;
    blockArrayStop                  = BLOCKDEFAULTINITVAL;
    blockArrayIsFull                = false;
    file                            = NULL;
    readThread=NULL;
    //if not QThread
    run();
}

WriteThread::~WriteThread()
{
    stopIt=true;
    needRemoveTheFile=true;
    // useless because stopIt will close all thread, but if thread not runing run it
    //endIsDetected();
    emit internalStartClose();
    //disconnect(this);//-> do into ~TransferThread()
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
}

bool WriteThread::internalOpen()
{
    //do a bug
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] internalOpen destination: "+fileName);
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        emit closed();
        return false;
    }
    if(file!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already open! destination: "+fileName);
        return false;
    }
    if(fileName.empty())
    {
        errorString_internal=tr("Path resolution error (Empty path)").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open: "+fileName+", error: "+errorString_internal);
        emit error();
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before the mutex");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mutex");
    stopIt=false;
    endDetected=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=InodeOperation;
    #endif
    //mkpath check if exists and return true if already exists
    /*do into transferThread ...QFileInfo destinationInfo(file);
    QDir destinationFolder;
    {
        if(!destinationFolder.exists(destinationInfo.absolutePath()))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] Try create the path: "+
                         destinationInfo.absolutePath().toStdString());
            if(!destinationFolder.mkpath(destinationInfo.absolutePath()))
            {
                if(!destinationFolder.exists(destinationInfo.absolutePath()))
                {
                    /// \todo do real folder error here
                    errorString_internal="mkpath error on destination";
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable create the folder: %1, error: %2")
                                 .arg(destinationInfo.absolutePath())
                                 .arg(QString::fromStdString(errorString_internal))
                                             .toStdString());
                    emit error();
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG
                    stat=Idle;
                    #endif
                    return false;
                }
            }
        }
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mkpath");
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        emit closed();
        return false;
    }
    //try open it
    file=fopen(fileName.c_str(),"wbx");
    bool fileWasExists=(file==NULL);
    if(file==NULL)
        file=fopen(fileName.c_str(),"wb");
    if(file!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the open");
        {
            if(blockArrayStart!=BLOCKDEFAULTINITVAL || blockArrayStop!=BLOCKDEFAULTINITVAL)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] General file corruption detected");
                stopIt=true;
                if(file!=NULL)
                {
                    fclose(file);
                    file=NULL;
                }
                fileName.clear();
                return false;
            }
        }
        const int intfd=fileno(file);
        #ifdef Q_OS_LINUX
        if(intfd!=-1)
        {
            posix_fadvise(intfd, 0, 0, POSIX_FADV_SEQUENTIAL);
            /*int flags = fcntl(intfd, F_GETFL, 0);
            fcntl(intfd, F_SETFL, flags | O_NONBLOCK);*/
        }
        #endif
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the pause mutex");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            if(file!=NULL)
            {
                fclose(file);
                file=NULL;
            }
            fileName.clear();
            emit closed();
            return false;
        }
        if(ftruncate64(intfd, startSize)!=0)
        {
            fclose(file);
            file=NULL;
            fileName.clear();
            errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to seek after open: "+fileName+", error: "+errorString_internal);
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            fclose(file);
            file=NULL;
            fileName.clear();
            emit closed();
            return false;
        }
        if(fseeko64(file,0,SEEK_SET)!=0)
        {
            fclose(file);
            file=NULL;
            fileName.clear();
            errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to resize to "+std::to_string(startSize)+" after open: "+fileName+", error: "+errorString_internal);
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            status=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            fclose(file);
            file=NULL;
            fileName.clear();
            emit closed();
            return false;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit opened()");
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        #ifdef Q_OS_LINUX
        EventLoop::eventLoop.watchSource(this,fileno(file));
        #endif
        needRemoveTheFile=false;
        postOperationRequested=false;
        return true;
    }
    else
    {
        struct stat fileStat;
        if(!fileWasExists && stat(fileName.c_str(),&fileStat)==0)
            if(unlink(fileName.c_str())!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] file created but can't be removed");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            fileName.clear();
            emit closed();
            return false;
        }
        errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open: "+fileName+", error: "+errorString_internal);
        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        return false;
    }
}

void WriteThread::open(const std::string &file, const uint64_t &startSize)
{
    if(this->file!=NULL)
    {
        if(file==this->fileName)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try reopen already opened same file: "+fileName);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already open: "+fileName);
        emit internalStartClose();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] open destination: "+file);
    stopIt=false;
    fakeMode=false;
    lastGoodPosition=0;
    this->fileName=file;
    this->startSize=startSize;
    endDetected=false;
    writeFullBlocked=false;
    emit internalStartOpen();
}

void WriteThread::endIsDetected()
{
    if(endDetected)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    endDetected=true;
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
    // useless because stopIt will close all thread, but if thread not runing run it
    endIsDetected();
    //for the stop for skip: void TransferThread::skip()
    emit internalStartClose();
}

void WriteThread::flushBuffer()
{
    blockArrayStart=BLOCKDEFAULTINITVAL;
    blockArrayStop=BLOCKDEFAULTINITVAL;
    blockArrayIsFull=false;
}

/// \brief buffer is empty
bool WriteThread::bufferIsEmpty()
{
    return blockArrayStart==blockArrayStop && !blockArrayIsFull;
}

void WriteThread::internalEndOfFile()
{
    if(!bufferIsEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start the write");
        emit internalStartWrite();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeIsStopped");
        flushBuffer();
        emit writeIsStopped();
    }
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close for file: "+fileName);
    /// \note never send signal here, because it's called by the destructor
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Close;
    #endif
    bool emit_closed=false;
    if(!fakeMode)
    {
        if(file!=NULL)
        {
            if(!needRemoveTheFile)
            {
                if(startSize!=lastGoodPosition)
                    if(ftruncate(fileno(file),lastGoodPosition)!=0)
                    {
                        if(emitSignal)
                        {
                            errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to seek after open: "+fileName+", error: "+errorString_internal);
                            emit error();
                        }
                        else
                            needRemoveTheFile=true;
                    }
            }
            fclose(file);
            file=NULL;
            if(needRemoveTheFile || stopIt)
            {
                if(deletePartiallyTransferredFiles)
                {
                    if(unlink(fileName.c_str())!=0)
                        if(emitSignal)
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to remove the destination file");
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
    //warning: file.setFileName("");
    fileName.clear();
    if(emit_closed)
        emit closed();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    status=Idle;
    #endif
}

void WriteThread::internalReopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    std::string tempFile=fileName;
    internalClose(false);
    flushBuffer();
    stopIt=false;
    lastGoodPosition=0;
    fileName=tempFile;
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
    emit writeIsStopped();
}

/// \brief get the last good position
int64_t WriteThread::getLastGoodPosition() const
{
    return lastGoodPosition;
}

void WriteThread::flushAndSeekToZero()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flushAndSeekToZero");
    stopIt=true;
    emit internalStartFlushAndSeekToZero();
}

void WriteThread::internalFlushAndSeekToZero()
{
    flushBuffer();
    if(fseeko64(file, 0, SEEK_SET)!=0)
    {
        errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to seek after open: "+fileName+", error: "+errorString_internal);
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

bool WriteThread::write()
{
    if(stopIt)
        return false;
    emit internalStartWrite();
    if(stopIt)
        return false;
    return true;
}

void WriteThread::internalWrite()
{
    const size_t blockSize=sizeof(blockArray);
    int32_t              bytesWriten=0;		///< temp data for block writing, the bytes writen
    uint32_t blockArrayStop=this->blockArrayStop;//load value out of atomic
    uint32_t blockArrayStart=this->blockArrayStart;//load value out of atomic
    bool bytesWasWriten=false;
    if(blockArrayStart==blockArrayStop && !blockArrayIsFull)
    {
        if(!endDetected)
            readThread->callBack();
        return;
    }
    do
    {
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt");
            return;
        }
        if(stopIt)
            return;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Write;
        #endif
        errno=0;
        if(blockArrayStop>blockArrayStart)
        {
            void * ptr=blockArray+blockArrayStart;
            const size_t size=blockArrayStop-blockArrayStart;
            bytesWriten=fwrite(ptr,1,size,file);
            if(bytesWriten>0 && (errno==0 || errno==EAGAIN))
                blockArrayStart+=bytesWriten;
        }
        else //if(blockArrayStart==blockSize || blockArrayStop<blockArrayStart)//and then blockArrayIsFull
        {
            void * ptr=blockArray+blockArrayStart;
            const size_t size=blockSize-blockArrayStart;
            bytesWriten=fwrite(ptr,1,size,file);
            if(bytesWriten>0 && (errno==0 || errno==EAGAIN))
                blockArrayStart+=bytesWriten;
        }
        if(blockArrayStart==blockSize)
            blockArrayStart=0;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Idle;
        #endif
        if(lastGoodPosition==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit writeIsStarted()");
            emit writeIsStarted();
        }
        lastGoodPosition+=bytesWriten;
        if(bytesWriten>0)
            bytesWasWriten=true;
        if(stopIt)
            return;
        if(errno!=0 && errno!=EAGAIN)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Error in writing: "+fileName+", errno: "+std::to_string(errno));
            errorString_internal="Error in writing: "+fileName+" ("+std::string(strerror(errno))+", errno: "+std::to_string(errno)+")";
            stopIt=true;
            emit error();
            return;
        }
    } while(bytesWriten>0 && blockArrayStart!=blockArrayStop);
    if(bytesWasWriten)
        blockArrayIsFull=false;
    //is empty
    if(!endDetected)
        readThread->callBack();
    //improve the performance due to drop block split
    /*if(blockArrayStart==blockArrayStop && !blockArrayIsFull)
    {
        blockArrayStart=BLOCKDEFAULTINITVAL;
        blockArrayStop=BLOCKDEFAULTINITVAL;
    } if manipulate the read thread var, need mutex and lower the performance*/
    this->blockArrayStart=blockArrayStart;
    if(endDetected && bufferIsEmpty())
        internalEndOfFile();
}

//set the read thread
void WriteThread::setReadThread(ReadThread * readThread)
{
    this->readThread=readThread;
}

#ifdef Q_OS_LINUX
void WriteThread::callBack()
{
    emit internalStartWrite();
}
#endif
