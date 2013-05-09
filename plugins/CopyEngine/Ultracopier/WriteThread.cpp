#include "WriteThread.h"

#include <QDir>

QMultiHash<QString,WriteThread *> WriteThread::writeFileList;
QMutex       WriteThread::writeFileListMutex;

WriteThread::WriteThread()
{
    deletePartiallyTransferredFiles = true;
    lastGoodPosition                = 0;
    stopIt                          = false;
    isOpen.release();
    moveToThread(this);
    setObjectName("write");
    this->mkpathTransfer            = mkpathTransfer;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat                            = Idle;
    #endif
    numberOfBlock                   = ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    buffer                          = false;
    putInPause                      = false;
    needRemoveTheFile               = false;
    blockSize                       = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    start();
}

WriteThread::~WriteThread()
{
    stopIt=true;
    needRemoveTheFile=true;
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
    //disconnect(this);//-> do into ~TransferThread()
    quit();
    wait();
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
    exec();
}

bool WriteThread::internalOpen()
{
    //do a bug
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] internalOpen destination: "+file.fileName());
    if(stopIt)
    {
        emit closed();
        return false;
    }
    if(file.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] already open! destination: "+file.fileName());
        return false;
    }
    if(file.fileName().isEmpty())
    {
        errorString_internal=tr("Path resolution error (Empty path)");
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
        emit error();
        return false;
    }
    //set to LISTBLOCKSIZE
    if(sequential)
    {
        while(writeFull.available()<1)
            writeFull.release();
        if(writeFull.available()>1)
            writeFull.acquire(writeFull.available()-1);
    }
    else
    {
        while(writeFull.available()<numberOfBlock)
            writeFull.release();
        if(writeFull.available()>numberOfBlock)
            writeFull.acquire(writeFull.available()-numberOfBlock);
    }
    stopIt=false;
    endDetected=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=InodeOperation;
    #endif
    //mkpath check if exists and return true if already exists
    QFileInfo destinationInfo(file);
    QDir destinationFolder;
    {
        mkpathTransfer->acquire();
        if(!destinationFolder.exists(destinationInfo.absolutePath()))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("Try create the path: %1")
                         .arg(destinationInfo.absolutePath()));
            if(!destinationFolder.mkpath(destinationInfo.absolutePath()))
            {
                if(!destinationFolder.exists(destinationInfo.absolutePath()))
                {
                    /// \todo do real folder error here
                    errorString_internal="mkpath error on destination";
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable create the folder: %1, error: %2")
                                 .arg(destinationInfo.absolutePath())
                                 .arg(errorString_internal));
                    emit error();
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG
                    stat=Idle;
                    #endif
                    mkpathTransfer->release();
                    return false;
                }
            }
        }
        mkpathTransfer->release();
    }
    if(stopIt)
    {
        emit closed();
        return false;
    }
    //try open it
    QIODevice::OpenMode flags=QIODevice::ReadWrite;
    if(!buffer)
        flags|=QIODevice::Unbuffered;
    {
        QMutexLocker lock_mutex(&writeFileListMutex);
        if(writeFileList.count(file.fileName(),this)==0)
        {
            writeFileList.insert(file.fileName(),this);
            if(writeFileList.count(file.fileName())>1)
                return false;
        }
    }
    if(file.open(flags))
    {
        {
            QMutexLocker lock_mutex(&accessList);
            if(!theBlockList.isEmpty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("General file corruption detected"));
                stopIt=true;
                return false;
            }
        }
        pauseMutex.tryAcquire(pauseMutex.available());
        if(stopIt)
        {
            file.close();
            emit closed();
            return false;
        }
        if(!file.seek(0))
        {
            file.close();
            errorString_internal=file.errorString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            file.close();
            emit closed();
            return false;
        }
        if(!file.resize(startSize))
        {
            file.close();
            errorString_internal=file.errorString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif
            return false;
        }
        if(stopIt)
        {
            file.close();
            emit closed();
            return false;
        }
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        isOpen.acquire();
        needRemoveTheFile=false;
        postOperationRequested=false;
        return true;
    }
    else
    {
        if(stopIt)
        {
            emit closed();
            return false;
        }
        errorString_internal=file.errorString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        return false;
    }
}

void WriteThread::open(const QFileInfo &file,const quint64 &startSize,const bool &buffer,const int &numberOfBlock,const bool &sequential)
{
    if(!isRunning())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] the thread not running to open destination: "+file.absoluteFilePath()+", numberOfBlock: "+QString::number(numberOfBlock));
        errorString_internal=tr("Internal error, please report it!");
        emit error();
        return;
    }
    if(this->file.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] already open! destination: "+file.fileName());
        errorString_internal=tr("Internal error, please report it!");
        emit error();
        return;
    }
    if(numberOfBlock<1 || (numberOfBlock>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK && numberOfBlock>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] numberOfBlock wrong, set to default");
        this->numberOfBlock=ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    }
    else
        this->numberOfBlock=numberOfBlock;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("open destination: %1, sequential: %2").arg(file.absoluteFilePath()).arg(sequential));
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
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    numberOfBlockCopied=0;
    #endif
}

void WriteThread::endIsDetected()
{
    if(endDetected)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    endDetected=true;
    pauseMutex.release();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    emit internalStartEndOfFile();
}

QString WriteThread::errorString()
{
    return errorString_internal;
}

void WriteThread::stop()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stop()");
    needRemoveTheFile=true;
    stopIt=true;
    if(isOpen.available()>0)
        return;
    writeFull.release();
    pauseMutex.release();
    pauseMutex.release();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    waitNewClockForSpeed.release();
    #endif
    waitNewClockForSpeed.release();
    waitNewClockForSpeed2.release();
    // useless because stopIt will close all thread, but if thread not runing run it
    endIsDetected();
    //for the stop for skip: void TransferThread::skip()
    emit internalStartClose();
}

void WriteThread::flushBuffer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    writeFull.release();
    writeFull.acquire();
    pauseMutex.release();
    {
        QMutexLocker lock_mutex(&accessList);
        theBlockList.clear();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stop");
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
    {
        if(sequential)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start the write");
            emit internalStartWrite();
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] buffer is not empty!");
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] writeIsStopped");
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
    QMutexLocker lock_mutex(&writeFileListMutex);
    writeFileList.remove(file.fileName(),this);
    if(writeFileList.contains(file.fileName()))
    {
        writeFileList.values(file.fileName()).first()->reemitStartOpen();
        return;
    }
}

void WriteThread::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] try put read thread in pause");
    pauseMutex.tryAcquire(pauseMutex.available());
    putInPause=true;
    return;
}

void WriteThread::resume()
{
    if(putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
        putInPause=false;
        stopIt=false;
    }
    else
        return;
    if(!file.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] file is not open");
        return;
    }
    pauseMutex.release();
}

void WriteThread::reemitStartOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] start");
    emit internalStartOpen();
}

void WriteThread::postOperation()
{
    if(postOperationRequested)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
        return;
    }
    postOperationRequested=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    emit internalStartClose();
}

void WriteThread::internalCloseSlot()
{
    internalClose();
}

void WriteThread::internalClose(bool emitSignal)
{
    /// \note never send signal here, because it's called by the destructor
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Close;
    #endif
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
                            errorString_internal=file.errorString();
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
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
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] unable to remove the destination file");
                }
            }
            //here and not after, because the transferThread don't need try close if not open
            if(emitSignal)
                emit closed();
        }
    }
    else
    {
        //here and not after, because the transferThread don't need try close if not open
        if(emitSignal)
            emit closed();
    }
    needRemoveTheFile=false;
    resumeNotStarted();
    file.setFileName("");

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Idle;
    #endif

    /// \note always the last of this function
    if(!fakeMode)
        isOpen.release();
}

void WriteThread::internalReopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    internalClose(false);
    flushBuffer();
    stopIt=false;
    lastGoodPosition=0;
    if(internalOpen())
        emit reopened();
}

void WriteThread::reopen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"block size out of range: "+QString::number(blockSize));
        return false;
    }
}

/// \brief get the last good position
qint64 WriteThread::getLastGoodPosition()
{
    return lastGoodPosition;
}

void WriteThread::flushAndSeekToZero()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flushAndSeekToZero: "+QString::number(blockSize));
    stopIt=true;
    emit internalStartFlushAndSeekToZero();
}


void WriteThread::checkSum()
{
    //QByteArray blockArray;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    endDetected=false;
    lastGoodPosition=0;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    numberOfBlockCopied=0;
    #endif
    if(!file.seek(0))
    {
        errorString_internal=file.errorString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
        emit error();
        return;
    }
    int sizeReaden=0;
    do
    {
        if(putInPause)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+QString::number(id)+"] write put in pause");
            if(stopIt)
                return;
            pauseMutex.acquire();
            if(stopIt)
                return;
        }
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
            errorString_internal=tr("Unable to read the source file: ")+file.errorString()+" ("+QString::number(file.error())+")";
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("file.error()!=QFile::NoError: %1, error: %2").arg(QString::number(file.error())).arg(errorString_internal));
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
        errorString_internal=tr("File truncated during read, possible data change");
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Source truncated during the read: %1 (%2)").arg(file.errorString()).arg(QString::number(file.error())));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stop the read");
}

void WriteThread::internalFlushAndSeekToZero()
{
    flushBuffer();
    if(!file.seek(0))
    {
        errorString_internal=file.errorString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(file.fileName()).arg(errorString_internal));
        emit error();
        return;
    }
    stopIt=false;
    emit flushedAndSeekedToZero();
}

void WriteThread::setMkpathTransfer(QSemaphore *mkpathTransfer)
{
    this->mkpathTransfer=mkpathTransfer;
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
    if(sequential)
    {
        if(stopIt)
            return false;
        {
            QMutexLocker lock_mutex(&accessList);
            theBlockList.append(data);
            atMax=(theBlockList.size()>=numberOfBlock);
        }
        if(atMax)
            emit internalStartWrite();
    }
    else
    {
        if(stopIt)
            return false;
        {
            QMutexLocker lock_mutex(&accessList);
            theBlockList.append(data);
            atMax=(theBlockList.size()>=numberOfBlock);
        }
        emit internalStartWrite();
    }
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
        if(sequential)
        {
            numberOfBlockCopied++;
            if(numberOfBlockCopied>=(multiForBigSpeed*2))
            {
                numberOfBlockCopied=0;
                waitNewClockForSpeed.acquire();
            }
        }
        else
        {
            numberOfBlockCopied2++;
            if(numberOfBlockCopied2>=multiForBigSpeed)
            {
                numberOfBlockCopied2=0;
                waitNewClockForSpeed2.acquire();
            }
        }
    }
    #endif
    if(stopIt)
        return false;
    return true;
}

void WriteThread::internalWrite()
{
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    if(sequential)
    {
        multiForBigSpeed=0;
        QMutexLocker lock_mutex(&accessList);
        if(theBlockList.size()<numberOfBlock && !endDetected)
            return;
    }
    #endif
    bool haveBlock;
    do
    {
        if(putInPause)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+QString::number(id)+"] write put in pause");
            if(stopIt)
                return;
            pauseMutex.acquire();
            if(stopIt)
                return;
        }
        if(stopIt)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] stopIt");
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
                if(multiForBigSpeed>0)
                {
                    if(blockArray.size()==blockSize)
                    {
                        theBlockList.removeFirst();
                        //if remove one block
                        if(!sequential)
                            writeFull.release();
                    }
                    else
                    {
                        blockArray.clear();
                        while(blockArray.size()!=blockSize)
                        {
                            //if larger
                            if(theBlockList.first().size()>blockSize)
                            {
                                blockArray+=theBlockList.first().mid(0,blockSize);
                                theBlockList.first().remove(0,blockSize);
                                if(!sequential)
                                {
                                    //do write in loop to finish the actual block
                                    emit internalStartWrite();
                                }
                                break;
                            }
                            //if smaller
                            else
                            {
                                blockArray+=theBlockList.first();
                                theBlockList.removeFirst();
                                //if remove one block
                                if(!sequential)
                                    writeFull.release();
                                if(theBlockList.isEmpty())
                                    break;
                            }
                        }
                    }
                    haveBlock=!blockArray.isEmpty();
                }
                else
                {
                    theBlockList.removeFirst();
                    //if remove one block
                    if(!sequential)
                        writeFull.release();
                }
                haveBlock=true;
            }
        }
        if(stopIt)
            return;
        if(!haveBlock)
        {
            if(sequential)
            {
                if(endDetected)
                    internalEndOfFile();
                else
                    writeFull.release();
                return;
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] End detected of the file");
            return;
        }
        #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
        //wait for limitation speed if stop not query
        if(multiForBigSpeed>0)
        {
            numberOfBlockCopied++;
            if(sequential || (!sequential && writeFullBlocked))
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
        stat=Write;
        #endif
        bytesWriten=file.write(blockArray);
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        //mutex for stream this data
        if(lastGoodPosition==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit writeIsStarted()");
            emit writeIsStarted();
        }
        if(stopIt)
            return;
        if(file.error()!=QFile::NoError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Error in writing: %1 (%2)").arg(file.errorString()).arg(file.error()));
            errorString_internal=QString("Error in writing: %1 (%2)").arg(file.errorString()).arg(file.error());
            stopIt=true;
            emit error();
            return;
        }
        if(bytesWriten!=blockArray.size())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size()));
            errorString_internal=QString("Error in writing, bytesWriten: %1, blockArray.size(): %2").arg(bytesWriten).arg(blockArray.size());
            stopIt=true;
            emit error();
            return;
        }
        lastGoodPosition+=bytesWriten;
    } while(sequential);
}
