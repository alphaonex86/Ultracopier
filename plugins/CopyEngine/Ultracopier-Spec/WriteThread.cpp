#include "WriteThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>
#include <stdio.h>

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
    blockArrayStart                 = 0;
    blockArrayStop                  = 0;
    file                            = NULL;
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
    connect(this,&WriteThread::internalStartOpen,               this,&WriteThread::internalOpen,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartReopen,             this,&WriteThread::internalReopen,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartWrite,              this,&WriteThread::internalWrite,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartClose,              this,&WriteThread::internalCloseSlot,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartEndOfFile,          this,&WriteThread::internalEndOfFile,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartFlushAndSeekToZero,	this,&WriteThread::internalFlushAndSeekToZero,	Qt::QueuedConnection);
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
            if(blockArrayStart!=0 || blockArrayStop!=0)
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
        #ifdef Q_OS_LINUX
        const int intfd=fileno(file);
        if(intfd!=-1)
            posix_fadvise(intfd, 0, 0, POSIX_FADV_SEQUENTIAL);
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
        if(fseeko64(file, 0, SEEK_SET)!=0)
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
        if(fseeko64(file,startSize,SEEK_SET)!=0)
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
    blockArrayStart=0;
    blockArrayStop=0;
}

/// \brief buffer is empty
bool WriteThread::bufferIsEmpty()
{
    return blockArrayStart==blockArrayStop;
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
    if(stopIt)
        return false;
    emit internalStartWrite();
    if(stopIt)
        return false;
    return true;
}

void WriteThread::internalWrite()
{
    int32_t              bytesWriten=0;		///< temp data for block writing, the bytes writen
    do
    {
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt");
            return;
        }
        //read one block
        const size_t blockSize=sizeof(blockArray);
        uint32_t blockArrayStop=this->blockArrayStop;//load value out of atomic
        if(blockArrayStart==blockArrayStop)
            return;
        if(stopIt)
            return;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        status=Write;
        #endif
        if(blockArrayStop>blockArrayStart)
        {
            bytesWriten=fwrite(blockArray+blockArrayStart,blockArrayStop-blockArrayStart,1,file);
            if(bytesWriten>0 && (errno==0 || errno==EAGAIN))
                blockArrayStart+=bytesWriten;
        }
        else //blockArrayStop<blockArrayStart because == is previously checked
        {
            if(blockArrayStart>=blockSize)
            {
                bytesWriten=fwrite(blockArray,blockArrayStop,1,file);
                if(bytesWriten>0 && (errno==0 || errno==EAGAIN))
                    blockArrayStart=bytesWriten;
            }
            else
            {
                bytesWriten=fwrite(blockArray+blockArrayStart,blockSize-blockArrayStart,1,file);
                if(bytesWriten>0 && (errno==0 || errno==EAGAIN))
                    blockArrayStart+=bytesWriten;
            }
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
        if(errno!=0 && errno!=EAGAIN)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Error in writing: "+fileName+", errno: "+std::to_string(errno));
            errorString_internal="Error in writing: "+fileName+" ("+std::string(strerror(errno))+", errno: "+std::to_string(errno)+")";
            stopIt=true;
            emit error();
            return;
        }
        lastGoodPosition+=bytesWriten;
    } while(bytesWriten>0);
}
