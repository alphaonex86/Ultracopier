//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThreadSync.h"
#include <string>
#include <dirent.h>

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#endif

#include "../../../../cpp11addition.h"

TransferThreadSync::TransferThreadSync()
{
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readThread.setWriteThread(&writeThread);
    writeThread.setReadThread(&readThread);
    #endif

    #ifndef Q_OS_UNIX
    PSecurityD=NULL;
    dacl=NULL;
    #endif
    //if not QThread
    run();
}

TransferThreadSync::~TransferThreadSync()
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

void TransferThreadSync::run()
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    //the error push
    if(!connect(&readThread,&ReadThread::error,                     this,					&TransferThreadSync::getReadError,      	Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::error,                   this,					&TransferThreadSync::getWriteError,         Qt::QueuedConnection))
        abort();
    //the state change operation
    if(!connect(&readThread,&ReadThread::opened,                    this,					&TransferThreadSync::readIsReady,           Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::opened,                  this,					&TransferThreadSync::writeIsReady,          Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::readIsStopped,             this,					&TransferThreadSync::readIsStopped,         Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::writeIsStopped,          this,                   &TransferThreadSync::writeIsStopped,		Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::readIsStopped,             &writeThread,			&WriteThread::endIsDetected,            Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::closed,                    this,					&TransferThreadSync::readIsClosed,          Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::closed,                  this,					&TransferThreadSync::writeIsClosed,         Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::reopened,                this,					&TransferThreadSync::writeThreadIsReopened,	Qt::QueuedConnection))
        abort();
    //error management
    if(!connect(&readThread,&ReadThread::isSeekToZeroAndWait,       this,					&TransferThreadSync::readThreadIsSeekToZeroAndWait,	Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,&writeThread,         &WriteThread::flushAndSeekToZero,               Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::flushedAndSeekedToZero,  this,                   &TransferThreadSync::readThreadResumeAfterError,	Qt::QueuedConnection))
        abort();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThreadSync::debugInformation,  Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThreadSync::debugInformation,  Qt::QueuedConnection))
        abort();
    #endif
    TransferThread::run();
}

void TransferThreadSync::startTheTransfer()
{
    emit internalTryStartTheTransfer();
}

void TransferThreadSync::internalStartTheTransfer()
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
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
        ifCanStartTransfer();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start the transfer as delayed"));
}

bool TransferThreadSync::setFiles(const std::string& source, const int64_t &size, const std::string& destination, const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+", destination: "+destination);
        return false;
    }
    if(!TransferThread::setFiles(source,size,destination,mode))
        return false;
    writeError_destination_reopened = false;
    return true;
}

void TransferThreadSync::resetExtraVariable()
{
    TransferThread::resetExtraVariable();
    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    readIsReadyVariable         = false;
    writeIsReadyVariable		= false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    readIsOpenVariable          = false;
    writeIsOpenVariable         = false;
    readIsOpeningVariable       = false;
    writeIsOpeningVariable      = false;
}

void TransferThreadSync::preOperation()
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
    if(canBeMovedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] need moved directly: "+source+" to "+destination);
        canBeMovedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    if(canBeCopiedDirectly())// is is_symlink(source);
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] need copied directly: "+source+" to "+destination);
        canBeCopiedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    tryOpen();
}

void TransferThreadSync::tryOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source and destination: "+source+" and "+destination);
    if(!readIsOpenVariable)
    {
        if(!readIsOpeningVariable)
        {
            readError=false;
            readThread.open(source,mode);
            readIsOpeningVariable=true;

            if(doRightTransfer)
                havePermission=readSourceFilePermissions(source);
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] readIsOpeningVariable is true when try open");
            emit errorOnFile(source,tr("Internal error: Already opening").toStdString());
            readError=true;
            return;
        }
    }
    if(!writeIsOpenVariable)
    {
        if(!writeIsOpeningVariable)
        {
            writeError=false;
            writeThread.open(destination,size);
            writeIsOpeningVariable=true;
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+"writeIsOpeningVariable is true when try open");
            emit errorOnFile(destination,tr("Internal error: Already opening").toStdString());
            writeError=true;
            return;
        }
    }
}

void TransferThreadSync::tryMoveDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] need moved directly: "+source+" to "+destination);

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    #endif
    //move if on same mount point
    #ifndef Q_OS_WIN32
    if(is_file(destination) || is_symlink(destination))
    {
        if(!is_file(source) && !is_symlink(source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destination+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source file doesn't exist").toStdString());
            return;
        }
        else if(unlink(destination.c_str())!=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destination+", "+std::string(strerror(errno))+", error: "+std::to_string(errno));
            readError=true;
            emit errorOnFile(destination,std::string(strerror(errno))+", errno: "+std::to_string(errno));
            return;
        }
    }
    #endif
    {
        const std::string &dir=FSabsolutePath(destination);
        if(mkFullPath)
        {
            if(!mkpath(dir.c_str()))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder can't be created: "+dir+", error: "+std::to_string(errno));
                emit errorOnFile(dir,tr("Unable to do the folder").toStdString());
                return;
            }
        }
        else
        {
            if(!mkdir(dir.c_str()))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder can't be created: "+dir+", error: "+std::to_string(errno));
                emit errorOnFile(dir,tr("Unable to do the folder").toStdString());
                return;
            }
        }
    }
    #ifdef Q_OS_WIN32
    std::wstring wsource=std::wstring(source.cbegin(),source.cend());
    std::wstring wdestination=std::wstring(destination.cbegin(),destination.cend());
    if(MoveFileExW(
            wsource.c_str(),
            wdestination.c_str(),
            MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING
        )==0)
    #else
    if(rename(source.c_str(),destination.c_str())!=0)
    #endif
    {
        const std::string &dir=FSabsolutePath(destination);
        readError=true;
        if(!is_file(source) && !is_symlink(source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+source+", error: "+std::to_string(errno));
            emit errorOnFile(source,tr("File not found").toStdString());
            return;
        }
        else if(!is_dir(dir))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder not exists "+destination+", error: "+std::to_string(errno));
            emit errorOnFile(destination,tr("Unable to do the folder").toStdString());
            return;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do real move "+destination+", error: "+std::to_string(errno));
        emit errorOnFile(source,std::string(strerror(errno))+", errno: "+std::to_string(errno));
        return;
    }
    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

void TransferThreadSync::tryCopyDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] need copied directly: "+source+" to "+destination);

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    #endif
    //move if on same mount point
    #ifndef Q_OS_WIN32
    if(is_file(destination) || is_symlink(destination))
    {
        if(!is_file(source) && !is_symlink(source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destination+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source file doesn't exist").toStdString());
            return;
        }
        else if(unlink(destination.c_str())!=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destination+", error: "+std::to_string(errno));
            readError=true;
            emit errorOnFile(destination,std::string(strerror(errno))+", errno: "+std::to_string(errno));
            return;
        }
    }
    #endif
    {
        const std::string &dir=FSabsolutePath(destination);
        if(mkFullPath)
        {
            if(!mkpath(dir.c_str()))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder can't be created: "+dir+", error: "+std::to_string(errno));
                emit errorOnFile(dir,tr("Unable to do the folder").toStdString());
                return;
            }
        }
        else
        {
            if(!mkdir(dir.c_str()))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder can't be created: "+dir+", error: "+std::to_string(errno));
                emit errorOnFile(dir,tr("Unable to do the folder").toStdString());
                return;
            }
        }
    }
    /** on windows, symLink is normal file, can be copied
     * on unix not, should be created **/
    #ifdef Q_OS_WIN32
    std::wstring wsource=std::wstring(source.cbegin(),source.cend());
    std::wstring wdestination=std::wstring(destination.cbegin(),destination.cend());
    if(CopyFileExW(
                wsource.c_str(),
                wdestination.c_str(),
            NULL,
            NULL,
            FALSE,
            0
        )==0)
    #else
    char buf[PATH_MAX];
    const ssize_t &sizeLink=readlink(source.c_str(),buf,sizeof(buf));
    if(sizeLink!=0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source resolution failerd "+source+", error: "+std::to_string(errno));
        emit errorOnFile(source,tr("The source symlink can't be read").toStdString());
        return;
    }
    if(!symlink(destination.c_str(),buf))
    #endif
    {
        const std::string &dir=FSabsolutePath(destination);
        readError=true;
        if(!is_file(source) && !is_symlink(source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+source+", error: "+std::to_string(errno));
            emit errorOnFile(source,tr("The source file doesn't exist").toStdString());
            return;
        }
        else if(is_file(destination) || is_symlink(destination))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination already exists "+source+", error: "+std::to_string(errno));
            emit errorOnFile(source,tr("Another file exists at same place").toStdString());
            return;
        }
        else if(!is_dir(dir))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination folder not exists "+source+", error: "+std::to_string(errno));
            emit errorOnFile(source,tr("Unable to do the folder").toStdString());
            return;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do sym link copy "+source+", error: "+std::to_string(errno));
        emit errorOnFile(source,std::string(strerror(errno))+", errno: "+std::to_string(errno));
        return;
    }
    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

void TransferThreadSync::readIsReady()
{
    if(readIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsReadyVariable=true;
    readIsOpenVariable=true;
    readIsClosedVariable=false;
    readIsOpeningVariable=false;
    ifCanStartTransfer();
}

void TransferThreadSync::ifCanStartTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readIsReadyVariable: "+std::to_string(readIsReadyVariable)+", writeIsReadyVariable: "+std::to_string(writeIsReadyVariable));
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        transfer_stat=TransferStat_WaitForTheTransfer;
        sended_state_readStopped	= false;
        sended_state_writeStopped	= false;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stat=WaitForTheTransfer");
        if(!sended_state_preOperationStopped)
        {
            sended_state_preOperationStopped=true;
            emit preOperationStopped();
        }
        if(canStartTransfer)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stat=Transfer, "+QStringLiteral("canBeMovedDirectlyVariable: %1, canBeCopiedDirectlyVariable: %2").arg(canBeMovedDirectlyVariable).arg(canBeCopiedDirectlyVariable).toStdString());
            transfer_stat=TransferStat_Transfer;
            if(canBeMovedDirectlyVariable)
                tryMoveDirectly();
            else if(canBeCopiedDirectlyVariable)
                tryCopyDirectly();
            else
            {
                needRemove=deletePartiallyTransferredFiles;
                readThread.startRead();
            }
            emit pushStat(transfer_stat,transferId);
        }
        //else
            //emit pushStat(stat,transferId);
    }
}

void TransferThreadSync::writeIsReady()
{
    if(writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsReadyVariable=true;
    writeIsOpenVariable=true;
    writeIsClosedVariable=false;
    writeIsOpeningVariable=false;
    ifCanStartTransfer();
}

//stop the current copy
void TransferThreadSync::stop()
{
    stopIt=true;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        return;
    }
    if(remainSourceOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"remainSourceOpen()");
        readThread.stop();
    }
    if(remainDestinationOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"remainDestinationOpen()");
        writeThread.stop();
    }
    if(!remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        if(needRemove && source!=destination)
        {
            if(is_file(source))
            {
                if(!unlink(destination.c_str()))
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] destination: "+destination+", errno: "+std::to_string(errno)));
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination when the source don't exists"));
        }
        transfer_stat=TransferStat_PostOperation;
        emit internalStartPostOperation();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("transfer_stat==%1 && remainFileOpen()").arg(transfer_stat).toStdString());
}

bool TransferThreadSync::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

bool TransferThreadSync::remainSourceOpen() const
{
    return (readIsOpenVariable || readIsOpeningVariable) && !readIsClosedVariable;
}

bool TransferThreadSync::remainDestinationOpen() const
{
    return (writeIsOpenVariable || writeIsOpeningVariable) && !writeIsClosedVariable;
}

void TransferThreadSync::readIsFinish()
{
    if(readIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] double event dropped"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsFinishVariable=true;
    canStartTransfer=false;
    transfer_stat=TransferStat_PostTransfer;
    if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then readIsClosed() already call
        readThread.postOperation();
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] in skip, don't start postOperation");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] pushStat("+std::to_string((int)transfer_stat)+","+std::to_string(transferId)+")");
    emit pushStat(transfer_stat,transferId);
}

void TransferThreadSync::writeIsFinish()
{
    if(writeIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsFinishVariable=true;
    if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then writeIsClosed() already call
        writeThread.postOperation();
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] in skip, don't start postOperation");
}

void TransferThreadSync::readIsClosed()
{
    if(readIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] double event dropped"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsClosedVariable=true;
    readIsOpeningVariable=false;
    checkIfAllIsClosedAndDoOperations();
}

void TransferThreadSync::writeIsClosed()
{
    if(writeIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsClosedVariable=true;
    writeIsOpeningVariable=false;
    if(stopIt && needRemove && source!=destination)
    {
        if(is_file(source))
        {
            if(unlink(destination.c_str())!=-1)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination "+destination+"failed, errno: "+std::to_string(errno)));
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination when the source don't exists"));
    }
    checkIfAllIsClosedAndDoOperations();
}

// return true if all is closed, and do some operations, don't use into condition to check if is closed!
bool TransferThreadSync::checkIfAllIsClosedAndDoOperations()
{
    if((readError || writeError) && !needSkip && !stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resolve error before progress");
        return false;
    }
    if(!remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit internalStartPostOperation() to do the real post operation");
        transfer_stat=TransferStat_PostOperation;
        //emit pushStat(stat,transferId);
        emit internalStartPostOperation();
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("wait self close: readIsReadyVariable: %1, readIsClosedVariable: %2, writeIsReadyVariable: %3, writeIsClosedVariable: %4")
                     .arg(readIsReadyVariable)
                     .arg(readIsClosedVariable)
                     .arg(writeIsReadyVariable)
                     .arg(writeIsClosedVariable)
                                 .toStdString()
                     );
        return false;
    }
}

/// \todo found way to retry that's
/// \todo the rights copy
void TransferThreadSync::postOperation()
{
    if(transfer_stat!=TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] need be in transfer, source: "+source+", destination: "+destination+", stat:"+std::to_string(transfer_stat));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    //all except closing
    if((readError || writeError) && !needSkip && !stopIt)//normally useless by checkIfAllIsFinish()
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resume after error");
        return;
    }

    if(!needSkip && !stopIt)
    {
        if(!canBeCopiedDirectlyVariable && !canBeMovedDirectlyVariable)
        {
            if(writeIsOpenVariable && !writeIsClosedVariable)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't pass in post operation if write is not closed"));
                emit errorOnFile(destination,tr("Internal error: The destination is not closed").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(readThread.getLastGoodPosition()!=writeThread.getLastGoodPosition())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+QString("] readThread.getLastGoodPosition(%1)!=writeThread.getLastGoodPosition(%2)")
                                         .arg(readThread.getLastGoodPosition())
                                         .arg(writeThread.getLastGoodPosition())
                                         .toStdString()
                                         );
                emit errorOnFile(destination,tr("Internal error: The size transfered doesn't match").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(!writeThread.bufferIsEmpty())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] buffer is not empty"));
                emit errorOnFile(destination,tr("Internal error: The buffer is not empty").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            //in normal mode, without copy/move syscall
            if(!doFilePostOperation())
                return;
        }

        //remove source in moving mode
        if(mode==Ultracopier::Move && !canBeMovedDirectlyVariable)
        {
            if(is_file(destination))
            {
                if(unlink(source.c_str())!=0)
                {
                    needSkip=false;
                    emit errorOnFile(source,std::string(strerror(errno))+", errno: "+std::to_string(errno));
                    return;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try remove source but destination not exists!"));
        }
    }
    else//do difference skip a file and skip this error case
    {
        if(needRemove && is_file(destination) && exists(source) && source!=destination)
        {
            if(!unlink(destination.c_str()))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try remove destination "+destination+" failed: errno"+std::to_string(errno)));
                //emit errorOnFile(source,destinationFile.errorString());
                //return;
            }
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try remove destination but not exists!");
    }
    source.clear();
    destination.clear();
    //don't need remove because have correctly finish (it's not in: have started)
    needRemove=false;
    needSkip=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit postOperationStopped()");
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThreadSync::getWriteError()
{
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in write error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError                = true;
    writeError                      = true;
    writeIsReadyVariable            = false;
    writeError_destination_reopened	= false;
    writeIsOpeningVariable          = false;
    if(!readError)//already display error for the read
        emit errorOnFile(destination,writeThread.errorString());
}

void TransferThreadSync::getReadError()
{
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in read error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError	= true;
    readError		= true;
    //writeIsReadyVariable	= false;//wrong because write can be ready here
    readIsReadyVariable	= false;
    readIsOpeningVariable=false;
    if(!writeError)//already display error for the write
        emit errorOnFile(source,readThread.errorString());
}

//retry after error
void TransferThreadSync::retryAfterError()
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
        if(readError || writeError)
        {
            readError=false;
            //writeError=false;
            resumeTransferAfterWriteError();
            writeThread.flushBuffer();
            transfer_stat=TransferStat_PreOperation;
            emit internalStartPreOperation();
            return;
        }
        emit internalStartPostOperation();
        return;
    }
    if(canBeMovedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retry the system move");
        tryMoveDirectly();
        return;
    }
    if(canBeCopiedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retry the copy directly");
        tryCopyDirectly();
        return;
    }
    //can have error on source and destination at the same time
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the write error: "+std::to_string(readError));
        if(readError)
            readThread.reopen();
        else
        {
            readIsClosedVariable=false;
            readThread.seekToZeroAndWait();
        }
        writeThread.reopen();
    }
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the read error");
        readThread.reopen();
    }
    if(!writeError && !readError)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unknow error resume");
}

void TransferThreadSync::writeThreadIsReopened()
{
    if(writeError_destination_reopened)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeError_destination_reopened=true;
    if(writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThreadSync::readThreadIsSeekToZeroAndWait()
{
    if(writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThreadSync::resumeTransferAfterWriteError()
{
    writeError=false;
/********************************
 if(canStartTransfer)
     readThread.startRead();
useless, because the open destination event
will restart the transfer as normal
*********************************/
/*********************************
if(!canStartTransfer)
    stat=WaitForTheTransfer;
useless because already do at open event
**********************************/
    //if is in wait
    if(!canStartTransfer)
        emit checkIfItCanBeResumed();
}

void TransferThreadSync::readThreadResumeAfterError()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readError=false;
    writeIsReady();
    readIsReady();
}

//////////////////////////////////////////////////////////////////
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThreadSync::readIsStopped()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] TransferThreadSync::readIsStopped()");
    if(!sended_state_readStopped)
    {
        sended_state_readStopped=true;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit readIsStopped()");
        emit readStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] drop dual read stopped");
        return;
    }
    readIsFinish();
}

void TransferThreadSync::writeIsStopped()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    if(!sended_state_writeStopped)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit writeStopped()");
        sended_state_writeStopped=true;
        emit writeStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeIsFinish();
}

//skip the copy
void TransferThreadSync::skip()
{
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
int64_t TransferThreadSync::copiedSize()
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostOperation:
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    default:
        return 0;
    }
}

//retry after error
void TransferThreadSync::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void TransferThreadSync::setRsync(const bool rsync)
{
    this->rsync=rsync;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThreadSync::setId(int id)
{
    TransferThread::setId(id);
    readThread.setId(id);
    writeThread.setId(id);
}

char TransferThreadSync::readingLetter() const
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

char TransferThreadSync::writingLetter() const
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
uint64_t TransferThreadSync::realByteTransfered() const
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

//first is read, second is write
std::pair<uint64_t, uint64_t> TransferThreadSync::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
        returnVar.first=readThread.getLastGoodPosition();
        returnVar.second=writeThread.getLastGoodPosition();
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostTransfer:
        returnVar.first=transferSize;
        returnVar.second=writeThread.getLastGoodPosition();
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

