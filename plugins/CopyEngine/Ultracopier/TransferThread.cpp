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
    deletePartiallyTransferredFiles = true;
    needSkip                        = false;
    transfer_stat                   = TransferStat_Idle;
    stopIt                          = false;
    fileExistsAction                = FileExists_NotSet;
    alwaysDoFileExistsAction        = FileExists_NotSet;
    readError                       = false;
    writeError                      = false;
    renameTheOriginalDestination    = false;
    needRemove                      = false;
    this->mkpathTransfer            = mkpathTransfer;
    readThread.setWriteThread(&writeThread);
    source.setCaching(false);
    destination.setCaching(false);

    maxTime=QDateTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
}

TransferThread::~TransferThread()
{
    stopIt=true;
    readThread.exit();
    readThread.wait();
    writeThread.exit();
    writeThread.wait();
    exit();
    //else cash without this disconnect
    //disconnect(&readThread);
    //disconnect(&writeThread);
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
    connect(&readThread,&ReadThread::error,                     this,					&TransferThread::getReadError,      	Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::error,                   this,					&TransferThread::getWriteError,         Qt::QueuedConnection);
    //the thread change operation
    connect(this,&TransferThread::internalStartPreOperation,	this,					&TransferThread::preOperation,          Qt::QueuedConnection);
    connect(this,&TransferThread::internalStartPostOperation,	this,					&TransferThread::postOperation,         Qt::QueuedConnection);
    //the state change operation
    connect(&readThread,&ReadThread::opened,                    this,					&TransferThread::readIsReady,           Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::opened,                  this,					&TransferThread::writeIsReady,          Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,             this,					&TransferThread::readIsStopped,         Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::writeIsStopped,          this,                   &TransferThread::writeIsStopped,		Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,             &writeThread,			&WriteThread::endIsDetected,            Qt::QueuedConnection);
    connect(&readThread,&ReadThread::closed,                    this,					&TransferThread::readIsClosed,          Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::closed,                  this,					&TransferThread::writeIsClosed,         Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::reopened,                this,					&TransferThread::writeThreadIsReopened,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::checksumFinish,            this,					&TransferThread::readChecksumFinish,	Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::checksumFinish,          this,					&TransferThread::writeChecksumFinish,	Qt::QueuedConnection);
    //error management
    connect(&readThread,&ReadThread::isSeekToZeroAndWait,       this,					&TransferThread::readThreadIsSeekToZeroAndWait,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAtTheLastPosition,this,	&TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,&writeThread,         &WriteThread::flushAndSeekToZero,               Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::flushedAndSeekedToZero,  this,                   &TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(this,&TransferThread::internalTryStartTheTransfer,	this,					&TransferThread::internalStartTheTransfer,      Qt::QueuedConnection);

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThread::debugInformation,  Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThread::debugInformation,  Qt::QueuedConnection);
    connect(&driveManagement,&DriveManagement::debugInformation,this,                   &TransferThread::debugInformation,	Qt::QueuedConnection);
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

bool TransferThread::setFiles(const QFileInfo& source,const qint64 &size,const QFileInfo& destination,const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        return false;
    }
    //to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
    transfer_stat			= TransferStat_PreOperation;
    //emit pushStat(stat,transferId);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
    this->source                    = source;
    this->destination               = destination;
    this->mode                      = mode;
    this->size                      = size;
    stopIt                          = false;
    fileExistsAction                = FileExists_NotSet;
    canStartTransfer                = false;
    sended_state_preOperationStopped= false;
    canBeMovedDirectlyVariable      = false;
    canBeCopiedDirectlyVariable     = false;
    fileContentError                = false;
    real_doChecksum                 = false;
    writeError                      = false;
    writeError_source_seeked        = false;
    writeError_destination_reopened = false;
    readError                       = false;
    fileContentError                = false;
    resetExtraVariable();
    emit internalStartPreOperation();
    return true;
}

void TransferThread::setFileExistsAction(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        return;
    }
    if(nameForRename.contains(QRegularExpression("[/\\\\\\*]")))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] can't use this kind of name, internal error");
        emit errorOnFile(destination,tr("Try rename with using special characters"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] nameForRename: "+nameForRename);
    if(!renameTheOriginalDestination)
        destination.setFile(destination.absolutePath()+QDir::separator()+nameForRename);
    else
    {
        QFile destinationFile(destination.absoluteFilePath());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("rename %1: to: %2").arg(destination.absoluteFilePath()).arg(destination.absolutePath()+QDir::separator()+nameForRename));
        if(!destinationFile.rename(destination.absolutePath()+QDir::separator()+nameForRename))
        {
            if(!destinationFile.exists())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1: destination: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()));
                emit errorOnFile(destinationFile,tr("File not found"));
                return;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do real move %1: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()));
            emit errorOnFile(destinationFile,destinationFile.errorString());
            return;
        }
        destination.refresh();
    }
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
    sended_state_preOperationStopped=false;
    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsReadyVariable         = false;
    writeIsReadyVariable		= false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    needRemove                  = false;
    needSkip                    = false;
    retry                       = false;
    readIsOpenVariable          = false;
    writeIsOpenVariable         = false;
}

void TransferThread::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start: source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
    needRemove=false;
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] is same "+source.absoluteFilePath()+" than "+destination.absoluteFilePath());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] after is same");
    if(readError)
    {
        readError=false;
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] before destination exists");
    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] destination exists: "+destination.absoluteFilePath());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] after destination exists");
    if(readError)
    {
        readError=false;
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] before keep date");
    if(keepDate)
    {
        if(maxTime>=source.lastModified())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources is older to copy the time: "+source.absoluteFilePath()+": "+maxTime.toString("dd.MM.yyyy hh:mm:ss.zzz")+">="+source.lastModified().toString("dd.MM.yyyy hh:mm:ss.zzz"));
            doTheDateTransfer=false;
        }
        else
        {
            doTheDateTransfer=readFileDateTime(source);
            if(!doTheDateTransfer)
            {
                //will have the real error at source open
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] unable to read the source time: "+source.absoluteFilePath());
            }
        }
    }
    else
        doTheDateTransfer=false;
    if(canBeMovedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("need moved directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
        canBeMovedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    if(canBeCopiedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("need copied directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
        canBeCopiedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    tryOpen();
}

void TransferThread::tryOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source and destination: "+source.absoluteFilePath()+" and "+destination.absoluteFilePath());
    TransferAlgorithm transferAlgorithm=this->transferAlgorithm;
    if(transferAlgorithm==TransferAlgorithm_Automatic)
    {
        if(driveManagement.isSameDrive(destination.absoluteFilePath(),source.absoluteFilePath()))
        {
            QStorageInfo::DriveType type=driveManagement.getDriveType(driveManagement.getDrive(source.absoluteFilePath()));
            if(type==QStorageInfo::RemoteDrive || type==QStorageInfo::RamDrive)
                transferAlgorithm=TransferAlgorithm_Parallel;
            else
                transferAlgorithm=TransferAlgorithm_Sequential;
        }
        else
            transferAlgorithm=TransferAlgorithm_Parallel;
    }
    if(!readIsOpenVariable)
    {
        readError=false;
        readThread.open(source.absoluteFilePath(),mode);
    }
    if(!writeIsOpenVariable)
    {
        if(transferAlgorithm==TransferAlgorithm_Sequential)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] transferAlgorithm==TransferAlgorithm_Sequential");
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] transferAlgorithm==TransferAlgorithm_Parallel");
        writeError=false;
        if(transferAlgorithm==TransferAlgorithm_Sequential)
            writeThread.open(destination.absoluteFilePath(),size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)),sequentialBuffer,true);
        else
            writeThread.open(destination.absoluteFilePath(),size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)),parallelBuffer,false);
    }
}

bool TransferThread::isSame()
{
    //check if source and destination is not the same
    //source.absoluteFilePath()==destination.absoluteFilePath() not work is source don't exists
    if(source.absoluteFilePath()==destination.absoluteFilePath())
    {
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        if(!source.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+source.absoluteFilePath()+" not exists");
        if(!source.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+source.absoluteFilePath()+" isSymLink");
        if(!destination.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] start source: "+destination.absoluteFilePath()+" isSymLink");
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
        emit fileAlreadyExists(source,destination,true);
        return true;
    }
    return false;
}

bool TransferThread::destinationExists()
{
    //check if destination exists
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("overwrite: %1, alwaysDoFileExistsAction: %2, readError: %3, writeError: %4")
                             .arg(fileExistsAction)
                             .arg(alwaysDoFileExistsAction)
                             .arg(readError)
                             .arg(writeError)
                             );
    if(alwaysDoFileExistsAction==FileExists_Overwrite || readError || writeError)
        return false;
    bool destinationExists;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] time to first FS access");
    destination.refresh();
    destinationExists=destination.exists();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] finish first FS access");
    if(destinationExists)
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
        if(source.exists())
        {
            if(fileExistsAction==FileExists_OverwriteIfNewer || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNewer))
            {
                if(destination.lastModified()<source.lastModified())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
            if(fileExistsAction==FileExists_OverwriteIfOlder || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfOlder))
            {
                if(destination.lastModified()>source.lastModified())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
            if(fileExistsAction==FileExists_OverwriteIfNotSame || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNotSame))
            {
                if(destination.lastModified()!=source.lastModified() || destination.size()!=source.size())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
        }
        else
        {
            if(fileExistsAction!=FileExists_NotSet)
            {
                transfer_stat=TransferStat_Idle;
                emit postOperationStopped();
                return true;
            }
        }
        if(fileExistsAction==FileExists_NotSet)
        {
            emit fileAlreadyExists(source,destination,false);
            return true;
        }
    }
    return false;
}

QString TransferThread::resolvedName(const QFileInfo &inode)
{
    QString fileName=inode.fileName();
    #ifdef Q_OS_WIN32
    if(fileName.isEmpty())
    {
        fileName=inode.absolutePath();
        fileName.replace(QRegularExpression("^([a-zA-Z]+):.*$"),"\\1");
        if(inode.absolutePath().contains(QRegularExpression("^[a-zA-Z]+:[/\\\\]?$")))
            fileName=tr("Drive %1").arg(fileName);
        else
            fileName=tr("Unknown folder");
    }
    #else
    if(fileName.isEmpty())
        fileName=tr("root");
    #endif
    return fileName;
}

//return true if has been renamed
bool TransferThread::checkAlwaysRename()
{
    if(alwaysDoFileExistsAction==FileExists_Rename)
    {
        QFileInfo newDestination=destination;
        QString fileName=resolvedName(newDestination);
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
            newDestination.setFile(newDestination.absolutePath()+QDir::separator()+newFileName+suffix);
            num++;
        }
        while(newDestination.exists());
        if(!renameTheOriginalDestination)
            destination=newDestination;
        else
        {
            QFile destinationFile(destination.absoluteFilePath());
            if(!destinationFile.rename(newDestination.absoluteFilePath()))
            {
                if(!destinationFile.exists())
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1: destination: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()));
                    emit errorOnFile(destinationFile,tr("File not found"));
                    readError=true;
                    return true;
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do real move %1: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()));
                readError=true;
                emit errorOnFile(destinationFile,destinationFile.errorString());
                return true;
            }
        }
        return true;
    }
    return false;
}

void TransferThread::tryMoveDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("need moved directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(source.absoluteFilePath());
    QFile destinationFile(destination.absoluteFilePath());
    if(destinationFile.exists())
    {
        if(!sourceFile.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source file doesn't exist"));
            return;
        }
        else if(!destinationFile.remove())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", error: "+destinationFile.errorString());
            readError=true;
            emit errorOnFile(destination,destinationFile.errorString());
            return;
        }
    }
    QDir dir(destination.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destination.absolutePath());
        mkpathTransfer->release();
    }
    if(!sourceFile.rename(destinationFile.fileName()))
    {
        readError=true;
        if(!sourceFile.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1: destination: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
            emit errorOnFile(sourceFile,tr("File not found"));
            return;
        }
        else if(!dir.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
            emit errorOnFile(destination.absolutePath(),tr("Unable to do the folder"));
            return;
        }
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] "+QString("need copied directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(source.absoluteFilePath());
    QFile destinationFile(destination.absoluteFilePath());
    if(destinationFile.exists())
    {
        if(!sourceFile.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source doesn't exist"));
            return;
        }
        else if(!destinationFile.remove())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", error: "+destinationFile.errorString());
            readError=true;
            emit errorOnFile(destination,destinationFile.errorString());
            return;
        }
    }
    QDir dir(destination.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destination.absolutePath());
        mkpathTransfer->release();
    }
    /** on windows, symLink is normal file, can be copied
     * on unix not, should be created **/
    #ifdef Q_OS_WIN32
    if(!sourceFile.copy(destinationFile.fileName()))
    #else
    if(!QFile::link(sourceFile.symLinkTarget(),destinationFile.fileName()))
    #endif
    {
        readError=true;
        if(!sourceFile.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("source not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
            emit errorOnFile(sourceFile,tr("The source file doesn't exist"));
            return;
        }
        else if(destinationFile.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination already exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
            emit errorOnFile(sourceFile,tr("Another file exists at same place"));
            return;
        }
        else if(!dir.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
            emit errorOnFile(sourceFile,tr("Unable to do the folder"));
            return;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do sym link copy %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()));
        emit errorOnFile(sourceFile,sourceFile.errorString());
        return;
    }
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
    return source.isSymLink() || driveManagement.isSameDrive(destination.absoluteFilePath(),source.absoluteFilePath());
}

bool TransferThread::canBeCopiedDirectly()
{
    return source.isSymLink();
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
                needRemove=deletePartiallyTransferredFiles;
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

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
//set the current max speed in KB/s
void TransferThread::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    readThread.setMultiForBigSpeed(multiForBigSpeed);
    writeThread.setMultiForBigSpeed(multiForBigSpeed);
}
#endif

//set block size in Bytes
bool TransferThread::setBlockSize(const unsigned int blockSize)
{
    bool read=readThread.setBlockSize(blockSize);
    bool write=writeThread.setBlockSize(blockSize);
    return (read && write);
}

//pause the copy
void TransferThread::pause()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] wrong stat to put in pause");
        return;
    }
    readThread.pause();
    writeThread.pause();
}

//resume the copy
void TransferThread::resume()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] wrong stat to put in pause");
        return;
    }
    readThread.resume();
    writeThread.resume();
}

//stop the current copy
void TransferThread::stop()
{
    stopIt=true;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("transfer_stat==TransferStat_Idle"));
        return;
    }
    if(readIsOpenVariable && !readIsClosedVariable)
        readThread.stop();
    if(writeIsOpenVariable && !writeIsClosedVariable)
        writeThread.stop();
    if(!(readIsOpenVariable && !readIsClosedVariable) && !(writeIsOpenVariable && !writeIsClosedVariable))
    {
        if(needRemove && source.absoluteFilePath()!=destination.absoluteFilePath() && source.exists())
            QFile(destination.absoluteFilePath()).remove();
        emit internalStartPostOperation();
    }
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
        emit errorOnFile(destination,tr("The checksums do not match"));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] need be in transfer, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath()+", stat:"+QString::number(transfer_stat));
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
        if(!canBeCopiedDirectlyVariable && !canBeMovedDirectlyVariable)
        {
            if(writeIsOpenVariable && !writeIsClosedVariable)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] can't pass in post operation if write is not closed");
                emit errorOnFile(destination,tr("Internal error: The destination is not closed"));
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(readThread.getLastGoodPosition()!=writeThread.getLastGoodPosition())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("["+QString::number(id)+"] readThread.getLastGoodPosition(%1)!=writeThread.getLastGoodPosition(%2)")
                                         .arg(readThread.getLastGoodPosition())
                                         .arg(writeThread.getLastGoodPosition())
                                         );
                emit errorOnFile(destination,tr("Internal error: The size transfered doesn't match"));
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(!writeThread.bufferIsEmpty())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("["+QString::number(id)+"] buffer is not empty"));
                emit errorOnFile(destination,tr("Internal error: The buffer is not empty"));
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
        }

        if(!doFilePostOperation())
            return;

        //remove source in moving mode
        if(mode==Ultracopier::Move && !canBeMovedDirectlyVariable)
        {
            if(destination.exists() && destination.isFile())
            {
                QFile sourceFile(source.absoluteFilePath());
                if(!sourceFile.remove())
                {
                    needSkip=false;
                    emit errorOnFile(source,sourceFile.errorString());
                    return;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] try remove source but destination not exists!");
        }
    }
    else//do difference skip a file and skip this error case
    {
        if(needRemove && destination.exists() && source.exists() && source.absoluteFilePath()!=destination.absoluteFilePath() && destination.isFile())
        {
            QFile destinationFile(destination.absoluteFilePath());
            if(!destinationFile.remove())
            {
                //emit errorOnFile(source,destinationFile.errorString());
                //return;
            }
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] try remove destination but not exists!");
    }
    source.setFile("");
    destination.setFile("");
    //don't need remove because have correctly finish (it's not in: have started)
    needRemove=false;
    needSkip=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] emit postOperationStopped()");
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

bool TransferThread::doFilePostOperation()
{
    //do operation needed by copy
    //set the time if no write thread used

    if(doTheDateTransfer)
    {
        destination.refresh();
        if(!destination.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to change the date: File not found"));
            //emit errorOnFile(destination,tr("Unable to change the date")+": "+tr("File not found"));
            //return false;
        }
        if(!writeFileDateTime(destination))
        {
            if(!destination.isFile())
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to change the date (is not a file)"));
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to change the date"));
            /* error with virtual folder under windows */
            //emit errorOnFile(destination,tr("Unable to change the date"));
            //return false;
        }
    }
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
        emit errorOnFile(destination,writeThread.errorString());
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
        emit errorOnFile(source,readThread.errorString());
}

//retry after error
void TransferThread::retryAfterError()
{
    if(transfer_stat==TransferStat_Idle)
    {
        if(transferId==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] seam have bug, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] restart all, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        resetExtraVariable();
        emit internalStartPreOperation();
        return;
    }
    //opening error
    if(transfer_stat==TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] is not idle, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath()+", stat: "+QString::number(transfer_stat));
        resetExtraVariable();
        emit internalStartPreOperation();
        //tryOpen();-> recheck all, because can be an error into isSame(), rename(), ...
        return;
    }
    //data streaming error
    if(transfer_stat!=TransferStat_PostOperation && transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+QString::number(id)+"] is not in right stat, source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath()+", stat: "+QString::number(transfer_stat));
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        if(readError || writeError)
        {
            resumeTransferAfterWriteError();
            writeThread.flushBuffer();
            transfer_stat=TransferStat_PreOperation;
            resetExtraVariable();
            emit internalStartPreOperation();
            return;
        }
        emit internalStartPostOperation();
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

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
void TransferThread::timeOfTheBlockCopyFinished()
{
    readThread.timeOfTheBlockCopyFinished();
    writeThread.timeOfTheBlockCopyFinished();
}
#endif

bool TransferThread::setParallelBuffer(int parallelBuffer)
{
    if(parallelBuffer<1 || parallelBuffer>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] wrong parallelBuffer: "+QString::number(parallelBuffer));
        return false;
    }
    else
    {
        this->parallelBuffer=parallelBuffer;
        return true;
    }
}

bool TransferThread::setSequentialBuffer(int sequentialBuffer)
{
    if(sequentialBuffer<1 || sequentialBuffer>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] wrong sequentialBuffer: "+QString::number(sequentialBuffer));
        return false;
    }
    else
    {
        this->sequentialBuffer=sequentialBuffer;
        return true;
    }
}

void TransferThread::setTransferAlgorithm(TransferAlgorithm transferAlgorithm)
{
    this->transferAlgorithm=transferAlgorithm;
    if(transferAlgorithm==TransferAlgorithm_Sequential)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"]transferAlgorithm==TransferAlgorithm_Sequential");
    else if(transferAlgorithm==TransferAlgorithm_Automatic)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"]transferAlgorithm==TransferAlgorithm_Automatic");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"]transferAlgorithm==TransferAlgorithm_Parallel");
}

//fonction to edit the file date time
bool TransferThread::readFileDateTime(const QFileInfo &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] readFileDateTime("+source.absoluteFilePath()+")");
    if(maxTime>=source.lastModified())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] the sources is older to copy the time: "+source.absoluteFilePath()+": "+source.lastModified().toString());
        return false;
    }
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            struct stat info;
            if(stat(source.absoluteFilePath().toLatin1().data(),&info)!=0)
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
            time_t ctime=source.created().toTime_t();
            time_t actime=source.lastRead().toTime_t();
            time_t modtime=source.lastModified().toTime_t();
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
                filePath[source.absoluteFilePath().toWCharArray(filePath)]=L'\0';
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

bool TransferThread::writeFileDateTime(const QFileInfo &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] writeFileDateTime("+destination.absoluteFilePath()+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            return utime(destination.absoluteFilePath().toLatin1().data(),&butime)==0;
        #else //mainly for mac
            return utime(destination.absoluteFilePath().toLatin1().data(),&butime)==0;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                return utime(destination.toLatin1().data(),&butime)==0;
            #else
                wchar_t filePath[65535];
                filePath[destination.absoluteFilePath().toWCharArray(filePath)]=L'\0';
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
    case TransferStat_WaitForTheTransfer:
        //needRemove=true;never put that's here, can product destruction of the file
    case TransferStat_PreOperation:
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
        //needRemove=true;never put that's here, can product destruction of the file
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
        //needRemove=true;never put that's here, can product destruction of the file
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
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+QString::number(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        emit internalStartPostOperation();
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
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
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

void TransferThread::setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType)
{
    driveManagement.setDrive(mountSysPoint,driveType);
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
    case TransferStat_Checksum:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

//first is read, second is write
QPair<quint64,quint64> TransferThread::progression()
{
    QPair<quint64,quint64> returnVar;
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
        returnVar.first=readThread.getLastGoodPosition();
        returnVar.second=writeThread.getLastGoodPosition();
        if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] read is smaller than write");
    break;
    case TransferStat_Checksum:
        returnVar.first=readThread.getLastGoodPosition();
        returnVar.second=writeThread.getLastGoodPosition();
    break;
    case TransferStat_PostTransfer:
        returnVar.first=transferSize;
        returnVar.second=writeThread.getLastGoodPosition();
        if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+QString::number(id)+"] read is smaller than write");
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

void TransferThread::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}

void TransferThread::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
}

void TransferThread::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    this->renameTheOriginalDestination=renameTheOriginalDestination;
}
