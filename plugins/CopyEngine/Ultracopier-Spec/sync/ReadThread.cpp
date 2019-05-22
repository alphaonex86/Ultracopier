#include "ReadThread.h"

#ifdef Q_OS_LINUX
#include <fcntl.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include "TransferThread.h"
#include "WriteThread.h"
#include "EventLoop.h"

ReadThread::ReadThread()
{
    stopIt=false;
    blockSize=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    setObjectName(QStringLiteral("read"));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Idle;
    #endif
    isInReadLoop=false;
    tryStartRead=false;
    lastGoodPosition=0;
    file=NULL;
    //if not QThread
    run();
}

ReadThread::~ReadThread()
{
    stopIt=true;
    //disconnect(this);//-> do into ~TransferThread()
    //if(isOpen.available()<=0)
        emit internalStartClose();
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
}

void ReadThread::open(const std::string &file, const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] open source: "+file);
    if(this->file!=NULL)
    {
        if(file==this->fileName)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try reopen already opened same file: "+this->fileName);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already open: "+this->fileName);
        emit internalStartClose();
    }
    if(isInReadLoop)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already readding: "+this->fileName);
        return;
    }
    if(tryStartRead)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] previous file is already try read: "+this->fileName);
        return;
    }
    stopIt=false;
    fakeMode=false;
    lastGoodPosition=0;
    this->fileName=file;
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
    emit internalStartClose();
}

bool ReadThread::seek(const int64_t &position)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with: "+std::to_string(position));
    struct stat p_statbuf;
    if(fstat(fileno(file), &p_statbuf)<0)
        return false;
    const off_t fsize = p_statbuf.st_size;
    if(position>fsize)
        return false;
    return (TransferThread::fseeko64(file, position, SEEK_SET)==0);
}

int64_t ReadThread::size() const
{
    if(file==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] file not open to size it");
        return -1;
    }
    struct stat p_statbuf;
    if(fstat(fileno(file), &p_statbuf)<0)
        return -1;
    return p_statbuf.st_size;
}

void ReadThread::postOperation()
{
    emit internalStartClose();
}

bool ReadThread::internalOpenSlot()
{
    return internalOpen();
}

bool ReadThread::internalOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] internalOpen source: "+fileName+", open in write because move: "+std::to_string(mode==Ultracopier::Move));
    if(stopIt)
    {
        emit closed();
        return false;
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=InodeOperation;
    #endif
    if(file!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] this file is already open: "+fileName);
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        emit closed();
        return false;
    }
    file=fopen(fileName.c_str(),"rb");
    if(file!=NULL)
    {
        if(stopIt)
        {
            fclose(file);
            file=NULL;
            emit closed();
            return false;
        }
        #ifdef Q_OS_LINUX
        const int intfd=fileno(file);
        if(intfd!=-1)
        {
            //posix_fadvise(intfd, 0, 0, POSIX_FADV_WILLNEED);
            posix_fadvise(intfd, 0, 0, POSIX_FADV_SEQUENTIAL);
            /*int flags = fcntl(intfd, F_GETFL, 0);
            if(fcntl(intfd, F_SETFL, flags | O_NONBLOCK)<0)
                abort();*/
        }
        #endif
        if(stopIt)
        {
            fclose(file);
            file=NULL;
            emit closed();
            return false;
        }
        lastGoodPosition=0;
        if(!seek(lastGoodPosition))
        {
            fclose(file);
            file=NULL;
            errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to seek after open: "+fileName+", error: "+std::to_string(errno));
            emit error();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            stat=Idle;
            #endif
            return false;
        }
        emit opened();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        return true;
    }
    else
    {
        errorString_internal=std::string(strerror(errno))+", errno: "+std::to_string(errno);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to open: "+fileName+", error: "+std::to_string(errno));
        emit error();
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif
        return false;
    }
}

void ReadThread::internalRead()
{
    isInReadLoop=true;
    tryStartRead=false;
    if(stopIt)
    {
        if(file!=NULL)
        {
            stopIt=false;
            lastGoodPosition=0;
            if(TransferThread::fseeko64(file,0,SEEK_SET)!=0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] seek to 0 failed");
                isInReadLoop=false;
                internalClose();
                return;
            }
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
    stat=InodeOperation;
    #endif
    int readSize=0;
    if(file==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] is not open!");
        isInReadLoop=false;
        return;
    }
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start the copy");
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    stat=Idle;
    #endif
    if(stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stopIt == true, then quit");
        isInReadLoop=false;
        internalClose();
        return;
    }
    isInReadLoop=true;
    const size_t blockSize=sizeof(writeThread->blockArray);
    ssize_t requestedSize=-1;
    uint32_t blockArrayStart=writeThread->blockArrayStart;//load value out of atomic
    uint32_t blockArrayStop=writeThread->blockArrayStop;//load value out of atomic
    do
    {
        //read one block
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Read;
        #endif
        readSize=-1;
        // if writeThread->blockArrayStart == writeThread->blockArrayStop then is empty
        errno=0;
        if(blockArrayStart<=blockArrayStop)
        {
            if(blockArrayStop>=blockSize)
            {
                if(blockArrayStart==0)
                {
                    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] blockArrayStart==0, buffer full at "+std::to_string(lastGoodPosition));
                    readSize=0;
                    errno=EAGAIN;
                }
                else
                {
                    requestedSize=blockArrayStart;
                    readSize=fread(writeThread->blockArray,1,requestedSize,file);
                    if(readSize>=0 && (errno==0 || errno==EAGAIN))
                        blockArrayStop=readSize;
                }
            }
            else
            {
                requestedSize=blockSize-blockArrayStop;
                if(requestedSize==0)
                {
                    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] blockArrayStart==0, buffer full at "+std::to_string(lastGoodPosition));
                    readSize=0;
                    errno=EAGAIN;
                }
                else
                {
                    void * ptr=NULL;
                    if(blockArrayStop<blockSize)
                        ptr=writeThread->blockArray+blockArrayStop;
                    else
                        ptr=writeThread->blockArray;
                    readSize=fread(ptr,1,requestedSize,file);
                    if(readSize>=0 && (errno==0 || errno==EAGAIN))
                        blockArrayStop+=readSize;
                }
            }
        }
        else
        {
            void * ptr=NULL;
            if(blockArrayStop<blockSize)
                ptr=writeThread->blockArray+blockArrayStop;
            else
                ptr=writeThread->blockArray;
            requestedSize=blockArrayStart;
            readSize=fread(ptr,1,requestedSize,file);
            if(readSize>=0 && (errno==0 || errno==EAGAIN))
                blockArrayStop+=readSize;
        }
        lastGoodPosition+=readSize;
        if(blockArrayStop==blockSize)
        {
            #if BLOCKDEFAULTINITVAL == 0
            blockArrayStop=0;
            if(blockArrayStart==0 && readSize>0)
            {
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] blockArrayStart==0, buffer full at "+std::to_string(lastGoodPosition));
                errno=EAGAIN;
                writeThread->blockArrayIsFull=true;
            }
            #endif
        }
        #if BLOCKDEFAULTINITVAL > 0
        if(blockArrayStart==blockArrayStop && readSize>0)
        {
            //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] blockArrayStart==0, buffer full at "+std::to_string(lastGoodPosition));
            errno=EAGAIN;
            writeThread->blockArrayIsFull=true;
        }
        #endif
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        stat=Idle;
        #endif

        if(errno!=0 && errno!=EAGAIN)
        {
            errorString_internal=tr("Unable to read the source file: ").toStdString()+std::string(strerror(errno))+", errno: "+std::to_string(errno);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] errno: "+std::to_string(errno));
            isInReadLoop=false;
            emit error();
            return;
        }
    }
    while(readSize>0 && errno!=EAGAIN && !stopIt && requestedSize<=readSize && blockArrayStart!=blockArrayStop);
    writeThread->blockArrayStop=blockArrayStop;
    if(stopIt)
    {
        stopIt=false;
        return;
    }
    if(readSize==0 || requestedSize>readSize)
    {
        if(lastGoodPosition>size())
        {
            errorString_internal=tr("File truncated during the read, possible data change").toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Source truncated during the read: "+std::to_string(lastGoodPosition)+">"+std::to_string(size()));
            isInReadLoop=false;
            emit error();
            return;
        }
        if(errno!=EAGAIN)//if not buffer full (errno=EAGAIN set in case of buffer full, see above)
        {
            isInReadLoop=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readIsStopped");
            emit readIsStopped();//will product by signal connection writeThread->endIsDetected();
        }
    }
    if(readSize>0 || errno==EAGAIN)
        writeThread->callBack();
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop the read at "+std::to_string(lastGoodPosition));
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
        emit readIsStarted();
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
    if(!fakeMode)
    {
        if(file!=NULL)
        {
            fclose(file);
            file=NULL;
            isInReadLoop=false;
        }
    }
    if(!callByTheDestructor)
        emit closed();
}

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
    if(file!=NULL)
    {
        fclose(file);
        file=NULL;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source file have changed since the last open, restart all");
    //fix this function like the close function
    if(internalOpen())
    {
        emit resumeAfterErrorByRestartAll();
        return true;
    }
    else
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
    if(file!=NULL)
    {
        lastGoodPosition=0;
        seek(0);
    }
    emit checkIfIsWait();
}

void ReadThread::isInWait()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    stopIt=false;
    if(file!=NULL)
    {
        lastGoodPosition=0;
        seek(0);
    }
    else
        internalOpen();
    emit isSeekToZeroAndWait();
}

bool ReadThread::isReading() const
{
    return isInReadLoop;
}

void ReadThread::callBack()
{
    if(isInReadLoop)
        emit internalStartRead();
}
