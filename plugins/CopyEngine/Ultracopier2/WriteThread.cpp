#include "WriteThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif
#include <QDir>

QMultiHash<QString,WriteThread *> WriteThread::writeFileList;

WriteThread::WriteThread()
{
    deletePartiallyTransferredFiles = true;
    lastGoodPosition                = 0;
    stopIt                          = false;
    setObjectName(QStringLiteral("write"));
    //this->mkpathTransfer            = mkpathTransfer;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat                            = Idle;
    #endif
    buffer                          = false;
    needRemoveTheFile               = false;
    blockArraySize                  = 0;
    file=NULL;
}

WriteThread::~WriteThread()
{
    stopIt=true;
    needRemoveTheFile=true;
    // useless because stopIt will close all thread, but if thread not runing run it
    //endIsDetected();
    emit internalStartClose();
    if(!file.fileName().isEmpty())
        resumeNotStarted();
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
    connect(this,&WriteThread::internalStartChecksum,           this,&WriteThread::checkSum,			Qt::QueuedConnection);
}

bool WriteThread::internalOpen()
{
    //do a bug
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] internalOpen destination: "+file.fileName().toStdString());
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        emit closed();
        return false;
    }
    if(file.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already open! destination: "+file.fileName().toStdString());
        return false;
    }
    if(file.fileName().isEmpty())
    {
        errorString_internal=tr("Path resolution error (Empty path)").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
        emit error();
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before the mutex");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mutex");
    stopIt=false;
    endDetected=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=InodeOperation;
    #endif
    //mkpath check if exists and return true if already exists
    QFileInfo destinationInfo(file);
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
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the mkpath");
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
        emit closed();
        return false;
    }
    //try open it
    QIODevice::OpenMode flags=QIODevice::ReadWrite;
    if(!buffer)
        flags|=QIODevice::Unbuffered;
    {
        if(writeFileList.count(file.fileName(),this)==0)
        {
            writeFileList.insert(file.fileName(),this);
            if(writeFileList.count(file.fileName())>1)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] in waiting because same file is found");
                return false;
            }
        }
    }
    bool fileWasExists=file.exists();
    if(file.open(flags))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the open");
        {
            if(!blockArray.isEmpty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] General file corruption detected");
                stopIt=true;
                file.close();
                resumeNotStarted();
                file.setFileName(QStringLiteral(""));
                return false;
            }
        }
        #ifdef Q_OS_LINUX
        const int intfd=file.handle();
        if(intfd!=-1)
            posix_fadvise(intfd, 0, 0, POSIX_FADV_SEQUENTIAL);
        #endif
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after the pause mutex");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            file.close();
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            emit closed();
            return false;
        }
        if(!file.seek(0))
        {
            file.close();
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            errorString_internal=file.errorString().toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            file.close();
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            emit closed();
            return false;
        }
        if(!file.resize(startSize))
        {
            file.close();
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            errorString_internal=file.errorString().toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to resize to %1 after open: %2, error: %3").arg(startSize).arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            file.close();
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            emit closed();
            return false;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit opened()");
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        needRemoveTheFile=false;
        postOperationRequested=false;
        return true;
    }
    else
    {
        if(!fileWasExists && file.exists())
            if(!file.remove())
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] file created but can't be removed");
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close because stopIt is at true");
            resumeNotStarted();
            file.setFileName(QStringLiteral(""));
            emit closed();
            return false;
        }
        errorString_internal=file.errorString().toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        return false;
    }
}

void WriteThread::open(const std::string &file, const uint64_t &startSize)
{
    if(this->file.isOpen())
    {
        if(file.absoluteFilePath()==this->file.fileName())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try reopen already opened same file: "+file.absoluteFilePath().toStdString());
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already open: "+file.absoluteFilePath().toStdString());
        emit internalStartClose();
    }
    if(numberOfBlock<1 || (numberOfBlock>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK && numberOfBlock>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] numberOfBlock wrong, set to default");
        this->numberOfBlock=ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    }
    else
        this->numberOfBlock=numberOfBlock;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("open destination: %1, sequential: %2").arg(file.absoluteFilePath()).arg(sequential).toStdString());
    stopIt=false;
    fakeMode=false;
    lastGoodPosition=0;
    this->file.setFileName(file.absoluteFilePath());
    this->startSize=startSize;
    this->buffer=buffer;
    this->sequential=sequential;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    blockArray.clear();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop");
}

/// \brief buffer is empty
bool WriteThread::bufferIsEmpty()
{
    return blockArray.isEmpty();
}

void WriteThread::internalEndOfFile()
{
    if(!bufferIsEmpty())
    {
        if(sequential)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start the write");
            emit internalStartWrite();
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] buffer is not empty!");
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeIsStopped");
        emit writeIsStopped();
    }
}

void WriteThread::resumeNotStarted()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!writeFileList.contains(file.fileName()))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] file: \""+file.fileName().toStdString()+"\" for similar inode is not located into the list of "+std::to_string(writeFileList.size())+" items!");
    #endif
    writeFileList.remove(file.fileName(),this);
    if(writeFileList.contains(file.fileName()))
    {
        QList<WriteThread *> writeList=writeFileList.values(file.fileName());
        if(!writeList.isEmpty())
           writeList.first()->reemitStartOpen();
        return;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] close for file: "+file.fileName().toStdString());
    /// \note never send signal here, because it's called by the destructor
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Close;
    #endif
    bool emit_closed=false;
    if(!fakeMode)
    {
        if(file.isOpen())
        {
            if(!needRemoveTheFile)
            {
                if(startSize!=lastGoodPosition)
                    if(!file.resize(lastGoodPosition))
                    {
                        if(emitSignal)
                        {
                            errorString_internal=file.errorString().toStdString();
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
                            emit error();
                        }
                        else
                            needRemoveTheFile=true;
                    }
            }
            file.close();
            if(needRemoveTheFile || stopIt)
            {
                if(deletePartiallyTransferredFiles)
                {
                    if(!file.remove())
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
    resumeNotStarted();
    //warning: file.setFileName(""); need be after resumeNotStarted()
    file.setFileName(QStringLiteral(""));
    if(emit_closed)
        emit closed();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Idle;
    #endif
}

void WriteThread::internalReopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    QString tempFile=file.fileName();
    internalClose(false);
    flushBuffer();
    stopIt=false;
    lastGoodPosition=0;
    file.setFileName(tempFile);
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

/// do the checksum
void WriteThread::startCheckSum()
{
    emit internalStartChecksum();
}

/** \brief set block size
\param block the new block size in B
\return Return true if succes */
bool WriteThread::setBlockSize(const int blockSize)
{
    //can be smaller than min block size to do correct speed limitation
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

/// \brief get the last good position
int64_t WriteThread::getLastGoodPosition() const
{
    return lastGoodPosition;
}

void WriteThread::flushAndSeekToZero()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flushAndSeekToZero: "+std::to_string(blockSize));
    stopIt=true;
    emit internalStartFlushAndSeekToZero();
}


void WriteThread::checkSum()
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    endDetected=false;
    lastGoodPosition=0;
    if(!file.seek(0))
    {
        errorString_internal=file.errorString().toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
        emit error();
        return;
    }
    int sizeReaden=0;
    do
    {
        //read one block
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Read;
        #endif
        blockArray=file.read(blockSize);
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif

        if(file.error()!=QFile::NoError)
        {
            errorString_internal=tr("Unable to read the source file: ").toStdString()+file.errorString().toStdString()+" ("+std::to_string(file.error())+")";
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("file.error()!=QFile::NoError: %1, error: %2").arg(QString::number(file.error())).arg(QString::fromStdString(errorString_internal)).toStdString());
            emit error();
            return;
        }
        sizeReaden=blockArray.size();
        if(sizeReaden>0)
        {
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Checksum;
            #endif
            hash.addData(blockArray);
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif

            if(stopIt)
                break;

            lastGoodPosition+=blockArray.size();
        }
    }
    while(sizeReaden>0 && !stopIt);
    if(lastGoodPosition>(quint64)file.size())
    {
        errorString_internal=tr("File truncated during read, possible data change").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Source truncated during the read: %1 (%2)").arg(file.errorString()).arg(QString::number(file.error())).toStdString());
        emit error();
        return;
    }
    if(stopIt)
    {
/*		if(putInPause)
            emit isInPause();*/
        stopIt=false;
        return;
    }
    emit checksumFinish(hash.result());
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop the read");
}

void WriteThread::internalFlushAndSeekToZero()
{
    flushBuffer();
    if(!file.seek(0))
    {
        errorString_internal=file.errorString().toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(QString::fromStdString(errorString_internal)).toStdString());
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

bool WriteThread::write(const void * const data,const size_t &size)
{
    if(stopIt)
        return false;
    bool atMax;
    if(sequential)
    {
        if(stopIt)
            return false;
        {
            blockArray.append(data);
            atMax=(blockArray.size()>=numberOfBlock);
        }
        if(atMax)
            emit internalStartWrite();
    }
    else
    {
        if(stopIt)
            return false;
        {
            blockArray.append(data);
            atMax=(blockArray.size()>=numberOfBlock);
        }
        emit internalStartWrite();
    }
    if(atMax)
    {
        writeFullBlocked=true;
        writeFullBlocked=false;
    }
    if(stopIt)
        return false;
    if(stopIt)
        return false;
    return true;
}

void WriteThread::internalWrite()
{
    bool haveBlock;
    do
    {
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt");
            return;
        }
        if(stopIt)
            return;
        //read one block
        {
            blockArray.clear();
            haveBlock=true;
        }
        if(stopIt)
            return;
        if(!haveBlock)
        {
            if(sequential)
            {
                if(endDetected)
                    internalEndOfFile();
                return;
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] End detected of the file");
            return;
        }
        if(stopIt)
            return;
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Write;
        #endif
        bytesWriten=file.write(blockArray);
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        //mutex for stream this data
        if(lastGoodPosition==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit writeIsStarted()");
            emit writeIsStarted();
        }
        if(stopIt)
            return;
        if(file.error()!=QFile::NoError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("Error in writing: %1 (%2)").arg(file.errorString()).arg(file.error()).toStdString());
            errorString_internal=QStringLiteral("Error in writing: %1 (%2)").arg(file.errorString()).arg(file.error()).toStdString();
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
    } while(sequential);
}
