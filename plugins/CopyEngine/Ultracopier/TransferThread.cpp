//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThread.h"

#ifndef Q_OS_UNIX
    #ifdef Q_OS_WIN32
        #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            #include <windows.h>
        #endif
    #endif
#endif

TransferThread::TransferThread()
{
    start();
    moveToThread(this);
    needSkip		= false;
    transfer_stat			= TransferStat_Idle;
    stopIt			= false;
    fileExistsAction	= FileExists_NotSet;
    alwaysDoFileExistsAction= FileExists_NotSet;
    readError		= false;
    writeError		= false;
    this->mkpathTransfer	= mkpathTransfer;
    readThread.setWriteThread(&writeThread);

    maxTime=QDateTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
}

TransferThread::~TransferThread()
{
    exit();
    //disconnect(&readThread);
    //disconnect(&writeThread);
    //disconnect(this);
    wait();
}

void TransferThread::run()
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start: "+QString::number((qint64)QThread::currentThreadId()));
    transfer_stat			= TransferStat_Idle;
    stopIt			= false;
    fileExistsAction	= FileExists_NotSet;
    alwaysDoFileExistsAction= FileExists_NotSet;
    //the error push
    connect(&readThread,&ReadThread::error,			this,					&TransferThread::getReadError,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::error,			this,					&TransferThread::getWriteError,		Qt::QueuedConnection);
    //the thread change operation
    connect(this,&TransferThread::internalStartPreOperation,	this,					&TransferThread::preOperation,		Qt::QueuedConnection);
    connect(this,&TransferThread::internalStartPostOperation,	this,					&TransferThread::postOperation,		Qt::QueuedConnection);
        //the state change operation
    //connect(&readThread,&ReadThread::readIsStopped,		&readThread,				&TransferThread::postOperation,		Qt::QueuedConnection);//commented to do the checksum
    connect(&readThread,&ReadThread::opened,			this,					&TransferThread::readIsReady,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::opened,			this,					&TransferThread::writeIsReady,		Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,		this,					&TransferThread::readIsStopped,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::writeIsStopped,		this,					&TransferThread::writeIsStopped,		Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,		&writeThread,				&WriteThread::endIsDetected,		Qt::QueuedConnection);
    //connect(&writeThread,&WriteThread::writeIsStopped,		&writeThread,				&WriteThread::postOperation,		Qt::QueuedConnection);//commented to do the checksum
    connect(&readThread,&ReadThread::closed,			this,					&TransferThread::readIsClosed,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::closed,			this,					&TransferThread::writeIsClosed,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::reopened,		this,					&TransferThread::writeThreadIsReopened,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::checksumFinish,	this,					&TransferThread::readChecksumFinish,	Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::checksumFinish,this,					&TransferThread::writeChecksumFinish,	Qt::QueuedConnection);
        //error management
    connect(&readThread,&ReadThread::isSeekToZeroAndWait,	this,					&TransferThread::readThreadIsSeekToZeroAndWait,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAtTheLastPosition,	this,		&TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,                     &writeThread,	&WriteThread::flushAndSeekToZero,		Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::flushedAndSeekedToZero,                          this,           &TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(this,&TransferThread::internalTryStartTheTransfer,	this,					&TransferThread::internalStartTheTransfer,	Qt::QueuedConnection);

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&readThread,&ReadThread::debugInformation,this,&TransferThread::debugInformation,Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::debugInformation,this,&TransferThread::debugInformation,Qt::QueuedConnection);
    connect(&driveManagement,&DriveManagement::debugInformation,this,&TransferThread::debugInformation,	Qt::QueuedConnection);
    #endif

    exec();
}

TransferStat TransferThread::getStat()
{
    return transfer_stat;
}

void TransferThread::startTheTransfer()
{
    emit internalTryStartTheTransfer();
}

void TransferThread::internalStartTheTransfer()
{
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
        {
            /// \bug can pass here because in case of direct move on same media, it return to idle stat directly
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at idle");
        }
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at PostOperation");
        return;
    }
    if(transfer_stat==TransferStat_Transfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at Transfer");
        return;
    }
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] canStartTransfer is already set to true");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] check how start the transfer");
    canStartTransfer=true;
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start directly the transfer");
        ifCanStartTransfer();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start the transfer as delayed");
}

bool TransferThread::setFiles(const QString &source,const qint64 &size,const QString &destination,const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
        return false;
    }
    //to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
    transfer_stat			= TransferStat_PreOperation;
    //emit pushStat(stat,transferId);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start, source: "+source+", destination: "+destination);
    this->source			= source;
    this->destination		= destination;
    this->mode			= mode;
    this->size			= size;
    fileExistsAction		= FileExists_NotSet;
    canStartTransfer		= false;
    sended_state_preOperationStopped= false;
    canBeMovedDirectlyVariable	= false;
    canBeCopiedDirectlyVariable = false;
    fileContentError		= false;
    resetExtraVariable();
    emit internalStartPreOperation();
    return true;
}

void TransferThread::setFileExistsAction(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] action: "+QString::number(action));
    if(action!=FileExists_Rename)
        fileExistsAction	= action;
    else
    {
        //always rename pass here
        fileExistsAction	= action;
        alwaysDoFileExistsAction=action;
    }
    if(action==FileExists_Skip)
    {
        skip();
        return;
    }
    resetExtraVariable();
    emit internalStartPreOperation();
}

void TransferThread::setFileRename(const QString &nameForRename)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] nameForRename: "+nameForRename);
    destinationInfo.setFile(destination);
    destination=destinationInfo.absolutePath();
    destination+=QDir::separator()+nameForRename;
    destinationInfo.setFile(destination);
    fileExistsAction	= FileExists_NotSet;
    resetExtraVariable();
    emit internalStartPreOperation();
}

void TransferThread::setAlwaysFileExistsAction(const FileExistsAction &action)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] action to do always: "+QString::number(action));
    alwaysDoFileExistsAction=action;
}

void TransferThread::resetExtraVariable()
{
    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError			= false;
    readError			= false;
    readIsReadyVariable		= false;
    writeIsReadyVariable		= false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    needSkip			= false;
    retry				= false;
    readIsOpenVariable		= false;
    writeIsOpenVariable		= false;
}

void TransferThread::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start: source: "+source+", destination: "+destination);
    needRemove=false;
    sourceInfo.setFile(source);
    destinationInfo.setFile(destination);
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] is same "+source+" than "+destination);
        return;
    }
    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] destination exists: "+source);
        return;
    }
    if(keepDate)
    {
        doTheDateTransfer=readFileDateTime(source);
        if(!doTheDateTransfer)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] unable to read the source time: "+source);
    }
    else
        doTheDateTransfer=false;
    if(canBeMovedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] need moved directly: "+source);
        canBeMovedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    if(canBeCopiedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] need copied directly: "+source);
        canBeCopiedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    tryOpen();
}

void TransferThread::tryOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source and destination: "+source+" and "+destination);
    if(!readIsOpenVariable)
    {
        readError=false;
        readThread.open(source,mode);
    }
    if(!writeIsOpenVariable)
    {
        writeError=false;
        writeThread.open(destination,size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)));
    }
}

bool TransferThread::isSame()
{
    //check if source and destination is not the same
    if(sourceInfo==destinationInfo && source==destination)
    {
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        if(!sourceInfo.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+source+" not exists");
        if(!sourceInfo.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+source+" isSymLink");
        if(!destinationInfo.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+destination+" isSymLink");
        #endif
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] is same but skip");
            transfer_stat=TransferStat_Idle;
            emit postOperationStopped();
            //quit
            return true;
        }
        if(checkAlwaysRename())
            return false;
        emit fileAlreadyExists(sourceInfo,destinationInfo,true);
        return true;
    }
    return false;
}

bool TransferThread::destinationExists()
{
    //check if destination exists
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] overwrite: "+QString::number(fileExistsAction)+", always action: "+QString::number(alwaysDoFileExistsAction));
    if(alwaysDoFileExistsAction==FileExists_Overwrite || readError || writeError)
        return false;
    if(destinationInfo.exists())
    {
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            transfer_stat=TransferStat_Idle;
            emit postOperationStopped();
            //quit
            return true;
        }
        if(checkAlwaysRename())
            return false;
        if(fileExistsAction==FileExists_OverwriteIfNewer || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNewer))
        {
            if(destinationInfo.lastModified()<sourceInfo.lastModified())
                return false;
            else
            {
                transfer_stat=TransferStat_Idle;
                emit postOperationStopped();
                return true;
            }
        }
        if(fileExistsAction==FileExists_OverwriteIfNotSameModificationDate || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNotSameModificationDate))
        {
            if(destinationInfo.lastModified()!=sourceInfo.lastModified())
                return false;
            else
            {
                transfer_stat=TransferStat_Idle;
                emit postOperationStopped();
                return true;
            }
        }
        if(fileExistsAction==FileExists_NotSet)
        {
            emit fileAlreadyExists(sourceInfo,destinationInfo,false);
            return true;
        }
    }
    return false;
}

//return true if has been renamed
bool TransferThread::checkAlwaysRename()
{
    if(alwaysDoFileExistsAction==FileExists_Rename)
    {
        QString absolutePath=destinationInfo.absolutePath();
        QString fileName=destinationInfo.fileName();
        QString suffix="";
        QString newFileName;
        //resolv the suffix
        if(fileName.contains(QRegularExpression("^(.*)(\\.[a-z0-9]+)$")))
        {
            suffix=fileName;
            suffix.replace(QRegularExpression("^(.*)(\\.[a-z0-9]+)$"),"\\2");
            fileName.replace(QRegularExpression("^(.*)(\\.[a-z0-9]+)$"),"\\1");
        }
        //resolv the new name
        int num=1;
        do
        {
            if(num==1)
            {
                if(firstRenamingRule=="")
                    newFileName=tr("%1 - copy").arg(fileName);
                else
                {
                    newFileName=firstRenamingRule;
                    newFileName.replace("%name%",fileName);
                }
            }
            else
            {
                if(otherRenamingRule=="")
                    newFileName=tr("%1 - copy (%2)").arg(fileName).arg(num);
                else
                {
                    newFileName=otherRenamingRule;
                    newFileName.replace("%name%",fileName);
                    newFileName.replace("%number%",QString::number(num));
                }
            }
            destination=absolutePath+QDir::separator()+newFileName+suffix;
            destinationInfo.setFile(destination);
            num++;
        }
        while(destinationInfo.exists());
        return true;
    }
    return false;
}

void TransferThread::tryMoveDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start the system move");

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(sourceInfo.absoluteFilePath());
    QFile destinationFile(destinationInfo.absoluteFilePath());
    if(destinationFile.exists() && !destinationFile.remove())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", error: "+destinationFile.errorString());
        readError=true;
        emit errorOnFile(destinationInfo,destinationFile.errorString());
        return;
    }
    QDir dir(destinationInfo.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destinationInfo.absolutePath());
        mkpathTransfer->release();
    }
    if(!sourceFile.rename(destinationFile.fileName()))
    {
        readError=true;
        if(!sourceFile.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
        else if(!dir.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do real move %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
        emit errorOnFile(sourceFile,sourceFile.errorString());
        return;
    }
    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

void TransferThread::tryCopyDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start copy directly");

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(sourceInfo.absoluteFilePath());
    QFile destinationFile(destinationInfo.absoluteFilePath());
    if(destinationFile.exists() && !destinationFile.remove())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", error: "+destinationFile.errorString());
        readError=true;
        emit errorOnFile(destinationInfo,destinationFile.errorString());
        return;
    }
    QDir dir(destinationInfo.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destinationInfo.absolutePath());
        mkpathTransfer->release();
    }
    /** on windows, symLink is normal file, can be copied
     * on unix not, should be created **/
    #ifdef Q_OS_WIN32
    if(!sourceFile.copy(destinationFile.fileName()))
    {
        readError=true;
        if(!sourceFile.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else if(destinationFile.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination already exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else if(!dir.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do sym link copy %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        emit errorOnFile(sourceFile,sourceFile.errorString());
        return;
    }
    #else
    if(!QFile::link(sourceFile.symLinkTarget(),destinationFile.fileName()))
    {
        readError=true;
        if(!sourceFile.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else if(destinationFile.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination already exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else if(!dir.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do sym link copy %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        emit errorOnFile(destinationFile,destinationFile.errorString());
        return;
    }
    #endif

    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

bool TransferThread::canBeMovedDirectly()
{
    if(mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("mode!=Ultracopier::Move"));
        return false;
    }
    return sourceInfo.isSymLink() || driveManagement.isSameDrive(destinationInfo.absoluteFilePath(),sourceInfo.absoluteFilePath());
}

bool TransferThread::canBeCopiedDirectly()
{
    return sourceInfo.isSymLink();
}

void TransferThread::readIsReady()
{
    if(readIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    readIsReadyVariable=true;
    readIsOpenVariable=true;
    readIsClosedVariable=false;
    ifCanStartTransfer();
}

void TransferThread::ifCanStartTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] readIsReadyVariable: "+QString::number(readIsReadyVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable));
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        transfer_stat=TransferStat_WaitForTheTransfer;
        sended_state_readStopped	= false;
        sended_state_writeStopped	= false;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stat=WaitForTheTransfer");
        if(!sended_state_preOperationStopped)
        {
            sended_state_preOperationStopped=true;
            emit preOperationStopped();
        }
        if(canStartTransfer)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] stat=Transfer, "+QString("canBeMovedDirectlyVariable: %1, canBeCopiedDirectlyVariable: %2").arg(canBeMovedDirectlyVariable).arg(canBeCopiedDirectlyVariable));
            transfer_stat=TransferStat_Transfer;
            if(canBeMovedDirectlyVariable)
                tryMoveDirectly();
            else if(canBeCopiedDirectlyVariable)
                tryCopyDirectly();
            else
            {
                needRemove=true;
                readThread.startRead();
            }
            emit pushStat(transfer_stat,transferId);
        }
        //else
            //emit pushStat(stat,transferId);
    }
}

void TransferThread::writeIsReady()
{
    if(writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    writeIsReadyVariable=true;
    writeIsOpenVariable=true;
    writeIsClosedVariable=false;
    ifCanStartTransfer();
}


//set the copy info and options before runing
void TransferThread::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
}

//set keep date
void TransferThread::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
}

//set the current max speed in KB/s
void TransferThread::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    readThread.setMultiForBigSpeed(multiForBigSpeed);
}

//set block size in KB
bool TransferThread::setBlockSize(const unsigned int blockSize)
{
    this->blockSize=blockSize;
    return readThread.setBlockSize(blockSize) && writeThread.setBlockSize(blockSize);
}

//pause the copy
void TransferThread::pause()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer)
        return;
    readThread.pause();
}

//resume the copy
void TransferThread::resume()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer)
        return;
    readThread.resume();
}

//stop the current copy
void TransferThread::stop()
{
    stopIt=true;
    if(transfer_stat==TransferStat_Idle)
        return;
    readThread.stop();
    writeThread.stop();
}

void TransferThread::readIsFinish()
{
    if(readIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    readIsFinishVariable=true;
    canStartTransfer=false;
    //check here if need start checksuming or not
    real_doChecksum=doChecksum && (!checksumOnlyOnError || fileContentError) && !canBeMovedDirectlyVariable;
    if(real_doChecksum)
    {
        readIsFinishVariable=false;
        transfer_stat=TransferStat_Checksum;
        sourceChecksum=QByteArray();
        destinationChecksum=QByteArray();
        readThread.startCheckSum();
    }
    else
    {
        transfer_stat=TransferStat_PostTransfer;
        if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then readIsClosed() already call
            readThread.postOperation();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] in skip, don't start postOperation");
    }
    emit pushStat(transfer_stat,transferId);
}

void TransferThread::writeIsFinish()
{
    if(writeIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    writeIsFinishVariable=true;
    //check here if need start checksuming or not
    if(real_doChecksum)
    {
        writeIsFinishVariable=false;
        transfer_stat=TransferStat_Checksum;
        writeThread.startCheckSum();
    }
    else
    {
        if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then writeIsClosed() already call
            writeThread.postOperation();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] in skip, don't start postOperation");
    }
}

void TransferThread::readChecksumFinish(const QByteArray& checksum)
{
    sourceChecksum=checksum;
    compareChecksum();
}

void TransferThread::writeChecksumFinish(const QByteArray& checksum)
{
    destinationChecksum=checksum;
    compareChecksum();
}

void TransferThread::compareChecksum()
{
    if(sourceChecksum.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] the checksum of source is missing");
        return;
    }
    if(destinationChecksum.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] the checksum of destination is missing");
        return;
    }
    if(sourceChecksum==destinationChecksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] the checksum match");
        readThread.postOperation();
        writeThread.postOperation();
        transfer_stat=TransferStat_PostTransfer;
        emit pushStat(transfer_stat,transferId);
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] the checksum not match");
        //emit error here, and wait to resume
        emit errorOnFile(destinationInfo,tr("The checksums not match"));
    }
}

void TransferThread::readIsClosed()
{
    if(readIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    readIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
}

void TransferThread::writeIsClosed()
{
    if(writeIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    writeIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
}

// return true if all is closed, and do some operations, don't use into condition to check if is closed!
bool TransferThread::checkIfAllIsClosedAndDoOperations()
{
    if((readError || writeError) && !needSkip)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] resolve error before progress");
        return false;
    }
    if((!readIsOpenVariable || readIsClosedVariable) && (!writeIsOpenVariable || writeIsClosedVariable))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit internalStartPostOperation() to do the real post operation");
        transfer_stat=TransferStat_PostOperation;
        //emit pushStat(stat,transferId);
        emit internalStartPostOperation();
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("wait self close: readIsReadyVariable: %1, readIsClosedVariable: %2, writeIsReadyVariable: %3, writeIsClosedVariable: %4")
                     .arg(readIsReadyVariable)
                     .arg(readIsClosedVariable)
                     .arg(writeIsReadyVariable)
                     .arg(writeIsClosedVariable)
                     );
        return false;
    }
}

/// \todo found way to retry that's
/// \todo the rights copy
void TransferThread::postOperation()
{
    if(transfer_stat!=TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] need be in transfer, source: "+source+", destination: "+destination+", stat:"+QString::number(transfer_stat));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    //all except closing
    if((readError || writeError) && !needSkip)//normally useless by checkIfAllIsFinish()
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] resume after error");
        return;
    }

    if(!needSkip)
    {
        if(!doFilePostOperation())
            return;

        //remove source in moving mode
        if(mode==Ultracopier::Move && !canBeMovedDirectlyVariable)
        {
            if(QFile::exists(destination))
            {
                QFile sourceFile(source);
                if(!sourceFile.remove())
                {
                    emit errorOnFile(sourceInfo,sourceFile.errorString());
                    return;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] try remove source but destination not exists!");
        }
    }
    else//do difference skip a file and skip this error case
    {
        if(needRemove && QFile::exists(destination))
        {
            QFile destinationFile(destination);
            if(!destinationFile.remove())
            {
                //emit errorOnFile(sourceInfo,destinationFile.errorString());
                //return;
            }
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] try remove destination but not exists!");
    }
    transfer_stat=TransferStat_Idle;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit postOperationStopped()");
    emit postOperationStopped();
}

bool TransferThread::doFilePostOperation()
{
    //do operation needed by copy
    //set the time if no write thread used
    if(doTheDateTransfer)
        //source,destination
        writeFileDateTime(destination);//can't do that's after move because after move the source not exist
        /*
          ignore it, because need correct management, mainly with move
        if(!)
        {
            emit errorOnFile(destinationInfo,tr("Unable to change the date"));//destination.errorString()
            return false;
        }*/
    if(stopIt)
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::getWriteError()
{
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] already in write error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    fileContentError		= true;
    writeError			= true;
    writeIsReadyVariable		= false;
    writeError_source_seeked	= false;
    writeError_destination_reopened	= false;
    if(!readError)//already display error for the read
        emit errorOnFile(destinationInfo,writeThread.errorString());
}

void TransferThread::getReadError()
{
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] already in read error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    fileContentError	= true;
    readError		= true;
    writeIsReadyVariable	= false;
    readIsReadyVariable	= false;
    if(!writeError)//already display error for the write
        emit errorOnFile(sourceInfo,readThread.errorString());
}

//retry after error
void TransferThread::retryAfterError()
{
    //opening error
    if(transfer_stat==TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] is not idle, source: "+source+", destination: "+destination+", stat: "+QString::number(transfer_stat));
        tryOpen();
        return;
    }
    //data streaming error
    if(transfer_stat!=TransferStat_PostOperation && transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] is not idle, source: "+source+", destination: "+destination+", stat: "+QString::number(transfer_stat));
        return;
    }
    if(canBeMovedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] retry the system move");
        tryMoveDirectly();
        return;
    }
    if(canBeCopiedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] retry the copy directly");
        tryCopyDirectly();
        return;
    }
    if(transfer_stat==TransferStat_Checksum)
    {
        if(writeError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start and resume the write error");
            writeThread.reopen();
        }
        else if(readError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start and resume the read error");
            readThread.reopen();
        }
        else //only checksum difference
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] retry all the transfer");
            canStartTransfer=true;
            ifCanStartTransfer();
        }
        return;
    }
    //can have error on source and destination at the same time
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start and resume the write error");
        if(readError)
            readThread.reopen();
        else
            readThread.seekToZeroAndWait();
        writeThread.reopen();
    }
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start and resume the read error");
        readThread.reopen();
    }
    if(!writeError && !readError)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] unknow error resume");
}

void TransferThread::writeThreadIsReopened()
{
    if(writeError_destination_reopened)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    writeError_destination_reopened=true;
    if(transfer_stat==TransferStat_Checksum)
    {
        writeThread.startCheckSum();
        return;
    }
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThread::readThreadIsSeekToZeroAndWait()
{
    if(writeError_source_seeked)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    writeError_source_seeked=true;
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThread::resumeTransferAfterWriteError()
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

void TransferThread::readThreadResumeAfterError()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
        readError=false;
        writeIsReady();
        readIsReady();
}

//////////////////////////////////////////////////////////////////
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::readIsStopped()
{
    if(!sended_state_readStopped)
    {
        sended_state_readStopped=true;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit readIsStopped()");
        emit readStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] drop dual read stopped");
        return;
    }
    readIsFinish();
}

void TransferThread::writeIsStopped()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start");
    if(!sended_state_writeStopped)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit writeStopped()");
        sended_state_writeStopped=true;
        emit writeStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
        return;
    }
    writeIsFinish();
}

void TransferThread::timeOfTheBlockCopyFinished()
{
    readThread.timeOfTheBlockCopyFinished();
}

//fonction to edit the file date time
bool TransferThread::readFileDateTime(const QString &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] readFileDateTime("+source+")");
    if(maxTime>=sourceInfo.lastModified())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] the sources is older to copy the time: "+source+": "+sourceInfo.lastModified().toString());
        return false;
    }
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            struct stat info;
            if(stat(source.toLatin1().data(),&info)!=0)
                return false;
            time_t ctime=info.st_ctim.tv_sec;
            time_t actime=info.st_atim.tv_sec;
            time_t modtime=info.st_mtim.tv_sec;
            //this function avalaible on unix and mingw
            butime.actime=actime;
            butime.modtime=modtime;
            Q_UNUSED(ctime);
            return true;
        #else //mainly for mac
            QFileInfo fileInfo(destination);
            time_t ctime=fileInfo.created().toTime_t();
            time_t actime=fileInfo.lastRead().toTime_t();
            time_t modtime=fileInfo.lastModified().toTime_t();
            //this function avalaible on unix and mingw
            utimbuf butime;
            butime.actime=actime;
            butime.modtime=modtime;
            Q_UNUSED(ctime);
            return true;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(source.toLatin1().data(),&info)!=0)
                    return false;
                time_t ctime=info.st_ctim.tv_sec;
                time_t actime=info.st_atim.tv_sec;
                time_t modtime=info.st_mtim.tv_sec;
                //this function avalaible on unix and mingw
                butime.actime=actime;
                butime.modtime=modtime;
                Q_UNUSED(ctime);
                return true;
            #else
                wchar_t filePath[65535];
                filePath[source.toWCharArray(filePath)]=L'\0';
                HANDLE hFileSouce = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] open failed to read: "+QString::fromWCharArray(filePath)+", error: "+QString::number(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileSouce);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] unable to get the file time");
                    return false;
                }
                this->ftCreateL=ftCreate.dwLowDateTime;
                this->ftCreateH=ftCreate.dwHighDateTime;
                this->ftAccessL=ftAccess.dwLowDateTime;
                this->ftAccessH=ftAccess.dwHighDateTime;
                this->ftWriteL=ftWrite.dwLowDateTime;
                this->ftWriteH=ftWrite.dwHighDateTime;
                CloseHandle(hFileSouce);
                return true;
            #endif
        #else
            return false;
        #endif
    #endif
    return false;
}

bool TransferThread::writeFileDateTime(const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] writeFileDateTime("+destination+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            return utime(destination.toLatin1().data(),&butime)==0;
        #else //mainly for mac
            return utime(destination.toLatin1().data(),&butime)==0;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                return utime(destination.toLatin1().data(),&butime)==0;
            #else
                wchar_t filePath[65535];
                filePath[destination.toWCharArray(filePath)]=L'\0';
                HANDLE hFileDestination = CreateFile(filePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if(hFileDestination == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] open failed to write: "+QString::fromWCharArray(filePath)+", error: "+QString::number(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                ftCreate.dwLowDateTime=this->ftCreateL;
                ftCreate.dwHighDateTime=this->ftCreateH;
                ftAccess.dwLowDateTime=this->ftAccessL;
                ftAccess.dwHighDateTime=this->ftAccessH;
                ftWrite.dwLowDateTime=this->ftWriteL;
                ftWrite.dwHighDateTime=this->ftWriteH;
                if(!SetFileTime(hFileDestination, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileDestination);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] unable to set the file time");
                    return false;
                }
                CloseHandle(hFileDestination);
                return true;
            #endif
        #else
            return false;
        #endif
    #endif
    return false;
}

//skip the copy
void TransferThread::skip()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start with stat: "+QString::number(transfer_stat));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] readIsOpenVariable: "+QString::number(readIsOpenVariable)+", readIsReadyVariable: "+QString::number(readIsReadyVariable)+", readIsFinishVariable: "+QString::number(readIsFinishVariable)+", readIsClosedVariable: "+QString::number(readIsClosedVariable));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] writeIsOpenVariable: "+QString::number(writeIsOpenVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable)+", writeIsFinishVariable: "+QString::number(writeIsFinishVariable)+", writeIsClosedVariable: "+QString::number(writeIsClosedVariable));
    switch(transfer_stat)
    {
    case TransferStat_PreOperation:
    case TransferStat_WaitForTheTransfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        //check if all is source and destination is closed
        if((readIsOpenVariable && !readIsClosedVariable) || (writeIsOpenVariable && !writeIsClosedVariable))
        {
            if(readIsOpenVariable && !readIsClosedVariable)
                readThread.stop();
            if(writeIsOpenVariable && !writeIsClosedVariable)
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_Transfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        if(canBeMovedDirectlyVariable || canBeCopiedDirectlyVariable)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] Do the direct FS fake close, canBeMovedDirectlyVariable: "+QString::number(canBeMovedDirectlyVariable)+", canBeCopiedDirectlyVariable: "+QString::number(canBeCopiedDirectlyVariable));
            readThread.fakeReadIsStarted();
            writeThread.fakeWriteIsStarted();
            readThread.fakeReadIsStopped();
            writeThread.fakeWriteIsStopped();
            return;
        }
        if((readIsOpenVariable && !readIsClosedVariable) || (writeIsOpenVariable && !writeIsClosedVariable))
        {
            if(readIsOpenVariable && !readIsClosedVariable)
                readThread.stop();
            if(writeIsOpenVariable && !writeIsClosedVariable)
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_Checksum:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        if((readIsOpenVariable && !readIsClosedVariable) || (writeIsOpenVariable && !writeIsClosedVariable))
        {
            if(readIsOpenVariable && !readIsClosedVariable)
                readThread.stop();
            if(writeIsOpenVariable && !writeIsClosedVariable)
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_PostOperation:
        //do nothing because here is closing...
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] is already in post op");
        break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] can skip in this state!");
        return;
    }
}

//return info about the copied size
qint64 TransferThread::copiedSize()
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostOperation:
        return readThread.getLastGoodPosition();
    case TransferStat_Checksum:
        return transferSize;
    default:
        return 0;
    }
}

//retry after error
void TransferThread::putAtBottom()
{
    emit tryPutAtBottom();
}

void TransferThread::setDrive(QStringList mountSysPoint)
{
    driveManagement.setDrive(mountSysPoint);
}

void TransferThread::set_osBufferLimit(unsigned int osBufferLimit)
{
    this->osBufferLimit=osBufferLimit;
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThread::setId(int id)
{
    this->id=id;
    readThread.setId(id);
    writeThread.setId(id);
}

QChar TransferThread::readingLetter()
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
    case ReadThread::Checksum:
        return 'S';
    break;
    default:
        return '?';
    }
}

QChar TransferThread::writingLetter()
{
    switch(writeThread.stat)
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
    case WriteThread::Checksum:
        return 'S';
    break;
    default:
        return '?';
    }
}

#endif

void TransferThread::setMkpathTransfer(QSemaphore *mkpathTransfer)
{
    this->mkpathTransfer=mkpathTransfer;
    writeThread.setMkpathTransfer(mkpathTransfer);
}

void TransferThread::set_doChecksum(bool doChecksum)
{
    this->doChecksum=doChecksum;
}

void TransferThread::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void TransferThread::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    this->checksumOnlyOnError=checksumOnlyOnError;
}

void TransferThread::set_osBuffer(bool osBuffer)
{
    this->osBuffer=osBuffer;
}

void TransferThread::set_osBufferLimited(bool osBufferLimited)
{
    this->osBufferLimited=osBufferLimited;
}

//not copied size, because that's count to the checksum, ...
quint64 TransferThread::realByteTransfered()
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostOperation:
    case TransferStat_Checksum:
        return readThread.getLastGoodPosition();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] transferSize: "+QString::number(transferSize));
        return transferSize;
    default:
        return 0;
    }
}

void TransferThread::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}
