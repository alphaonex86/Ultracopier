//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThreadAsync.h"
#include <string>
#include <dirent.h>

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#endif

#include "../../../../cpp11addition.h"

TransferThreadAsync::TransferThreadAsync() :
    transferProgression(0)
{
    #ifndef Q_OS_UNIX
    PSecurityD=NULL;
    dacl=NULL;
    #endif
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    //the error push

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    #endif
    TransferThread::run();
    if(!connect(this,&TransferThreadAsync::internalStartPostOperation,        this,                   &TransferThreadAsync::doFilePostOperation,  Qt::QueuedConnection))
        abort();

    start();
}

TransferThreadAsync::~TransferThreadAsync()
{
    stopIt=true;
    //else cash without this disconnect
    //disconnect(&readThread);
    //disconnect(&writeThread);
    #ifndef Q_OS_UNIX
    if(PSecurityD!=NULL)
    {
        free(PSecurityD);
        PSecurityD=NULL;
    }
    if(dacl!=NULL)
    {
        free(dacl);
        dacl=NULL;
    }
    #endif
}

void TransferThreadAsync::run()
{
    exec();
}

void TransferThreadAsync::startTheTransfer()
{
    emit internalTryStartTheTransfer();
}

void TransferThreadAsync::internalStartTheTransfer()
{
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
        {
            /// \bug can pass here because in case of direct move on same media, it return to idle stat directly
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at idle"));
        }
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at PostOperation"));
        return;
    }
    if(transfer_stat==TransferStat_Transfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at Transfer"));
        return;
    }
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] canStartTransfer is already set to true"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] check how start the transfer"));
    canStartTransfer=true;

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
    ifCanStartTransfer();
}

bool TransferThreadAsync::setFiles(const std::string& source, const int64_t &size, const std::string& destination, const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+", destination: "+destination);
        return false;
    }
    if(!TransferThread::setFiles(source,size,destination,mode))
        return false;
    return true;
}

void TransferThreadAsync::resetExtraVariable()
{
    transferProgression=0;
    TransferThread::resetExtraVariable();
}

void TransferThreadAsync::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+", destination: "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: source: "+source+", destination: "+destination);
    needRemove=false;
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same "+source+" than "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after is same");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before destination exists");
    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists: "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after destination exists");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before keep date");
    #ifdef Q_OS_WIN32
    doTheDateTransfer=!is_symlink(source);
    #else
    doTheDateTransfer=true;
    #endif
    if(doTheDateTransfer)
    {
        doTheDateTransfer=readSourceFileDateTime(source);
        #ifdef Q_OS_MAC
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] read the source time: "+std::to_string(butime.modtime));
        #endif
        if(!doTheDateTransfer)
        {
            //will have the real error at source open
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to read the source time: "+source);
            if(keepDate)
            {
                emit errorOnFile(source,tr("Wrong modification date or unable to get it, you can disable time transfer to do it").toStdString());
                return;
            }
        }
    }
    if(doRightTransfer)
        havePermission=readSourceFilePermissions(source);
    transfer_stat=TransferStat_WaitForTheTransfer;
    ifCanStartTransfer();
}

void TransferThreadAsync::ifCanStartTransfer()
{
    if(transfer_stat!=TransferStat_WaitForTheTransfer /*wait preoperation*/ || !canStartTransfer/*wait start call*/)
        return;
    transfer_stat=TransferStat_Transfer;
    emit pushStat(transfer_stat,transferId);
    if(!copy/move())
    {
        if(stopIt)
            return;//and reset?
        readError=true;
        writeError=true;
        emit errorOnFile(destination,writeThread.errorString());
        remove();
    }
    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
    transfer_stat=TransferStat_PostOperation;
    emit pushStat(transfer_stat,transferId);
    doFilePostOperation();

    source.clear();
    destination.clear();
    //don't need remove because have correctly finish (it's not in: have started)
    needRemove=false;
    needSkip=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit postOperationStopped()");
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

//stop the current copy
void TransferThreadAsync::stop()
{
    stopIt=true;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        return;
    }
    exit();
    wait();
    start();
}

//retry after error
void TransferThreadAsync::retryAfterError()
{
    /// \warning skip the resetExtraVariable(); to be more exact and resolv some bug
    if(transfer_stat==TransferStat_Idle)
    {
        if(transferId==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] seam have bug, source: ")+source+", destination: "+destination);
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] restart all, source: "+source+", destination: "+destination);
        readError=false;
        //writeError=false;
        emit internalStartPreOperation();
        return;
    }
    //opening error
    if(transfer_stat==TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is not idle, source: "+source+", destination: "+destination+", stat: "+std::to_string(transfer_stat));
        readError=false;
        //writeError=false;
        emit internalStartPreOperation();
        //tryOpen();-> recheck all, because can be an error into isSame(), rename(), ...
        return;
    }
    //data streaming error
    if(transfer_stat!=TransferStat_PostOperation && transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] is not in right stat, source: ")+source+", destination: "+destination+", stat: "+std::to_string(transfer_stat));
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        emit internalStartPostOperation();
        return;
    }
    async ifCanStartTransfer();
}

//skip the copy
void TransferThreadAsync::skip()
{
    stopIt=true;
    exit();
    wait();
    start();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with stat: "+std::to_string(transfer_stat));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readIsOpeningVariable: "+std::to_string(readIsOpeningVariable)+", readIsOpenVariable: "+std::to_string(readIsOpenVariable)+", readIsReadyVariable: "+std::to_string(readIsReadyVariable)+", readIsFinishVariable: "+std::to_string(readIsFinishVariable)+", readIsClosedVariable: "+std::to_string(readIsClosedVariable));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeIsOpeningVariable: "+std::to_string(writeIsOpeningVariable)+", writeIsOpenVariable: "+std::to_string(writeIsOpenVariable)+", writeIsReadyVariable: "+std::to_string(writeIsReadyVariable)+", writeIsFinishVariable: "+std::to_string(writeIsFinishVariable)+", writeIsClosedVariable: "+std::to_string(writeIsClosedVariable));
    switch(transfer_stat)
    {
    case TransferStat_WaitForTheTransfer:
        //needRemove=true;never put that's here, can product destruction of the file
    case TransferStat_PreOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        //check if all is source and destination is closed
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        if(canBeMovedDirectlyVariable || canBeCopiedDirectlyVariable)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] Do the direct FS fake close, canBeMovedDirectlyVariable: "+std::to_string(canBeMovedDirectlyVariable)+", canBeCopiedDirectlyVariable: "+std::to_string(canBeCopiedDirectlyVariable));
            readThread.fakeReadIsStarted();
            writeThread.fakeWriteIsStarted();
            readThread.fakeReadIsStopped();
            writeThread.fakeWriteIsStopped();
            return;
        }
        writeThread.flushBuffer();
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_PostOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        writeThread.flushBuffer();
        emit internalStartPostOperation();
        break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] can skip in this state: "+std::to_string(transfer_stat));
        return;
    }
}

//return info about the copied size
int64_t TransferThreadAsync::copiedSize()
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostOperation:
    case TransferStat_PostTransfer:
        return transferProgression;
    default:
        return 0;
    }
}

//retry after error
void TransferThreadAsync::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void TransferThreadAsync::setRsync(const bool rsync)
{
    this->rsync=rsync;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThreadAsync::setId(int id)
{
    TransferThread::setId(id);
    readThread.setId(id);
    writeThread.setId(id);
}

char TransferThreadAsync::readingLetter() const
{
    switch(readThread.stat)
    {
    case ReadThread::Idle:
        return '_';
    break;
    case ReadThread::InodeOperation:
        return 'I';
    break;
    case ReadThread::Read:
        return 'R';
    break;
    case ReadThread::WaitWritePipe:
        return 'W';
    break;
    default:
        return '?';
    }
}

char TransferThreadAsync::writingLetter() const
{
    switch(writeThread.status)
    {
    case WriteThread::Idle:
        return '_';
    break;
    case WriteThread::InodeOperation:
        return 'I';
    break;
    case WriteThread::Write:
        return 'W';
    break;
    case WriteThread::Close:
        return 'C';
    break;
    case WriteThread::Read:
        return 'R';
    break;
    default:
        return '?';
    }
}

#endif

//not copied size, ...
uint64_t TransferThreadAsync::realByteTransfered() const
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
        return transferProgression;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

//first is read, second is write
std::pair<uint64_t, uint64_t> TransferThreadAsync::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
        returnVar.first=transferProgression;
        returnVar.second=transferProgression;
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostTransfer:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostOperation:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
    break;
    default:
        returnVar.first=0;
        returnVar.second=0;
    }
    return returnVar;
}

