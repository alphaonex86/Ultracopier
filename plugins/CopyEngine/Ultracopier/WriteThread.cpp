#include "WriteThread.h"

#include <QDir>

WriteThread::WriteThread()
{
    stopIt=false;
    isOpen.release();
    start();
    moveToThread(this);
    setObjectName("write");
    this->mkpathTransfer	= mkpathTransfer;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Idle;
    #endif
    CurentCopiedSize=0;
    buffer=false;
    putInPause=false;
    needRemoveTheFile=false;
    blockSize=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
}

WriteThread::~WriteThread()
{
    stopIt=true;
    needRemoveTheFile=true;
    freeBlock.release();
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
    connect(this,&WriteThread::internalStartOpen,			this,&WriteThread::internalOpen,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartReopen,			this,&WriteThread::internalReopen,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartWrite,			this,&WriteThread::internalWrite,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartClose,			this,&WriteThread::internalCloseSlot,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartEndOfFile,		this,&WriteThread::internalEndOfFile,		Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartFlushAndSeekToZero,	this,&WriteThread::internalFlushAndSeekToZero,	Qt::QueuedConnection);
    connect(this,&WriteThread::internalStartChecksum,		this,&WriteThread::checkSum,			Qt::QueuedConnection);
    exec();
}

bool WriteThread::internalOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] internalOpen destination: "+name);
    if(stopIt)
        return false;
    if(file.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] already open! destination: "+file.fileName());
        return false;
    }
    //set to LISTBLOCKSIZE
    while(freeBlock.available()<ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK)
        freeBlock.release();
    if(freeBlock.available()>ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK)
        freeBlock.acquire(freeBlock.available()-ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK);
        stopIt=false;
    CurentCopiedSize=0;
    endDetected=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=InodeOperation;
    #endif
    file.setFileName(name);
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
                    return false;
                }
            }
        }
        mkpathTransfer->release();
    }
    if(stopIt)
        return false;
    //try open it
    QIODevice::OpenMode flags=QIODevice::ReadWrite;
    if(!buffer)
        flags|=QIODevice::Unbuffered;
    if(file.open(flags))
    {
        flushBuffer();
        if(stopIt)
            return false;
        file.seek(0);
        if(stopIt)
            return false;
        file.resize(startSize);
        if(stopIt)
            return false;
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
            return false;
        errorString_internal=file.errorString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to open: %1, error: %2").arg(name).arg(errorString_internal));
        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        return false;
    }
}

void WriteThread::open(const QString &name,const quint64 &startSize,const bool &buffer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] open destination: "+name);
    if(stopIt)
        return;
    fakeMode=false;
    this->name=name;
    this->startSize=startSize;
    this->buffer=buffer;
    endDetected=false;
    emit internalStartOpen();
}

void WriteThread::endIsDetected()
{
    if(endDetected)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    endDetected=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    emit internalStartEndOfFile();
}

QString WriteThread::errorString()
{
    return errorString_internal;
}

bool WriteThread::write(const QByteArray &data)
{
    if(stopIt)
                return false;
    freeBlock.acquire();
    if(stopIt)
                return false;
    {
        QMutexLocker lock_mutex(&accessList);
        theBlockList.append(data);
    }
    emit internalStartWrite();
        return true;
}

void WriteThread::stop()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stop()");
    needRemoveTheFile=true;
    stopIt=true;
    if(isOpen.available()>0)
        return;
    freeBlock.release();
    // useless because stopIt will close all thread, but if thread not runing run it
    endIsDetected();
    //for the stop for skip: void TransferThread::skip()
    emit internalStartClose();
}

void WriteThread::flushBuffer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    freeBlock.release();
    freeBlock.acquire();
    {
        QMutexLocker lock_mutex(&accessList);
        theBlockList.clear();
    }
}

void WriteThread::internalEndOfFile()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] writeIsStopped");
    emit writeIsStopped();
}

void WriteThread::internalWrite()
{
    if(stopIt)
        return;
    //read one block
    if(theBlockList.size()<=0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] End detected of the file");
        return;
    }
    else
    {
        QMutexLocker lock_mutex(&accessList);
        blockArray=theBlockList.first();
        theBlockList.removeFirst();
    }
    //write one block
    freeBlock.release();

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
    if(CurentCopiedSize==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit writeIsStarted()");
        emit writeIsStarted();
    }
    CurentCopiedSize+=bytesWriten;
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
                if(startSize!=CurentCopiedSize)
                    file.resize(CurentCopiedSize);
            }
            file.close();
            if(needRemoveTheFile)
            {
                if(file.remove())
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] unable to remove the destination file");
            }
            needRemoveTheFile=false;
            flushBuffer();
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
    CurentCopiedSize=0;
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
\param block the new block size in KB
\return Return true if succes */
bool WriteThread::setBlockSize(const int blockSize)
{
    if(blockSize<1 || blockSize>16384)
    {
        this->blockSize=blockSize*1024;
        return true;
    }
    else
        return false;
}

void WriteThread::flushAndSeekToZero()
{
        stopIt=true;
        emit internalStartFlushAndSeekToZero();
}


void WriteThread::checkSum()
{
    //QByteArray blockArray;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    endDetected=false;
    lastGoodPosition=0;
    file.seek(0);
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
    if(lastGoodPosition>file.size())
    {
        errorString_internal=tr("File truncated during the read, possible data change");
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
    file.seek(0);
        stopIt=false;
        emit flushedAndSeekedToZero();
}

void WriteThread::setMkpathTransfer(QSemaphore *mkpathTransfer)
{
    this->mkpathTransfer=mkpathTransfer;
}
