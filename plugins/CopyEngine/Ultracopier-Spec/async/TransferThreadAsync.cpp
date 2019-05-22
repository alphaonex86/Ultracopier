//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThreadAsync.h"
#include <string>
#include <dirent.h>

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#endif

#include "../../../cpp11addition.h"

TransferThreadAsync::TransferThreadAsync() :
    haveStartTime                   (false),
    transfer_stat                   (TransferStat_Idle),
    doRightTransfer                 (false),
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync                           (false),
    #endif
    keepDate                        (false),
    mkFullPath                      (false),
    stopIt                          (false),
    fileExistsAction                (FileExists_NotSet),
    alwaysDoFileExistsAction        (FileExists_NotSet),
    needSkip                        (false),
    needRemove                      (false),
    deletePartiallyTransferredFiles (true),
    writeError                      (false),
    readError                       (false),
    renameTheOriginalDestination    (false),
    havePermission                  (false)
{
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readThread.setWriteThread(&writeThread);
    writeThread.setReadThread(&readThread);
    #endif
    renameRegex=std::regex("^(.*)(\\.[a-zA-Z0-9]+)$");
    #ifdef Q_OS_WIN32
        #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            regRead=std::regex("^[a-zA-Z]:");
        #endif
    #endif

    #ifndef Q_OS_UNIX
    PSecurityD=NULL;
    dacl=NULL;
    #endif
    //if not QThread
    run();
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
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    transfer_stat			= TransferStat_Idle;
    stopIt                  = false;
    fileExistsAction        = FileExists_NotSet;
    alwaysDoFileExistsAction= FileExists_NotSet;
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    //the error push
    if(!connect(&readThread,&ReadThread::error,                     this,					&TransferThreadAsync::getReadError,      	Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::error,                   this,					&TransferThreadAsync::getWriteError,         Qt::QueuedConnection))
        abort();
    #endif
    //the thread change operation
    if(!connect(this,&TransferThreadAsync::internalStartPreOperation,	this,					&TransferThreadAsync::preOperation,          Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThreadAsync::internalStartPostOperation,	this,					&TransferThreadAsync::postOperation,         Qt::QueuedConnection))
        abort();
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    //the state change operation
    if(!connect(&readThread,&ReadThread::opened,                    this,					&TransferThreadAsync::readIsReady,           Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::opened,                  this,					&TransferThreadAsync::writeIsReady,          Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::readIsStopped,             this,					&TransferThreadAsync::readIsStopped,         Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::writeIsStopped,          this,                   &TransferThreadAsync::writeIsStopped,		Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::readIsStopped,             &writeThread,			&WriteThread::endIsDetected,            Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::closed,                    this,					&TransferThreadAsync::readIsClosed,          Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::closed,                  this,					&TransferThreadAsync::writeIsClosed,         Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::reopened,                this,					&TransferThreadAsync::writeThreadIsReopened,	Qt::QueuedConnection))
        abort();
    //error management
    if(!connect(&readThread,&ReadThread::isSeekToZeroAndWait,       this,					&TransferThreadAsync::readThreadIsSeekToZeroAndWait,	Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::resumeAfterErrorByRestartAtTheLastPosition,this,	&TransferThreadAsync::readThreadResumeAfterError,	Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,&writeThread,         &WriteThread::flushAndSeekToZero,               Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::flushedAndSeekedToZero,  this,                   &TransferThreadAsync::readThreadResumeAfterError,	Qt::QueuedConnection))
        abort();
    #endif
    if(!connect(this,&TransferThreadAsync::internalTryStartTheTransfer,	this,					&TransferThreadAsync::internalStartTheTransfer,      Qt::QueuedConnection))
        abort();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    if(!connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    #endif
    if(!connect(&driveManagement,&DriveManagement::debugInformation,this,                   &TransferThreadAsync::debugInformation,	Qt::QueuedConnection))
        abort();
    #endif
}

TransferStat TransferThreadAsync::getStat() const
{
    return transfer_stat;
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
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
        ifCanStartTransfer();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start the transfer as delayed"));
    #else
    ifCanStartTransfer();
    #endif
}

bool TransferThreadAsync::setFiles(const std::string& source, const int64_t &size, const std::string& destination, const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+", destination: "+destination);
        return false;
    }
    //to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
    transfer_stat			= TransferStat_PreOperation;
    //emit pushStat(stat,transferId);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+source+", destination: "+destination);
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
    writeError                      = false;
    writeError_source_seeked        = false;
    writeError_destination_reopened = false;
    readError                       = false;
    fileContentError                = false;
    resetExtraVariable();
    emit internalStartPreOperation();
    return true;
}

void TransferThreadAsync::setFileExistsAction(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+(", destination: ")+destination);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] action: ")+std::to_string(action));
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

void TransferThreadAsync::setFileRename(const std::string &nameForRename)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+(", destination: ")+destination);
        return;
    }
    if(QString::fromStdString(nameForRename).contains(QRegularExpression(QStringLiteral("[/\\\\\\*]"))))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't use this kind of name, internal error"));
        emit errorOnFile(destination,tr("Try rename with using special characters").toStdString());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] nameForRename: "+nameForRename);
    if(!renameTheOriginalDestination)
        destination=destination+'/'+nameForRename;
    else
    {
        std::string tempDestination=destination;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] rename "+destination+": to: "+destination+'/'+nameForRename);
        if(!rename(destination.c_str(),(destination+'/'+nameForRename).c_str()))
        {
            if(!is_file(destination))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+destination+": destination: "+destination+", error: "+std::to_string(errno));
                emit errorOnFile(destination,tr("File not found").toStdString());
                return;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do real move "+destination+": "+destination+", error: "+std::to_string(errno));
            emit errorOnFile(destination,"errno: "+std::to_string(errno));
            return;
        }
        if(source==destination)
            source=destination+'/'+nameForRename;
        destination=tempDestination;
    }
    fileExistsAction	= FileExists_NotSet;
    resetExtraVariable();
    emit internalStartPreOperation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination is: "+destination);
}

void TransferThreadAsync::setAlwaysFileExistsAction(const FileExistsAction &action)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] action to do always: ")+QString::number(action)));
    alwaysDoFileExistsAction=action;
}

void TransferThreadAsync::resetExtraVariable()
{
    sended_state_preOperationStopped=false;
    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readIsReadyVariable         = false;
    writeIsReadyVariable		= false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    #endif
    needRemove                  = false;
    needSkip                    = false;
    retry                       = false;
    #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
    readIsOpenVariable          = false;
    writeIsOpenVariable         = false;
    readIsOpeningVariable       = false;
    writeIsOpeningVariable      = false;
    #endif
    havePermission              = false;
}

void TransferThreadAsync::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source+", destination: "+destination);
        return;
    }
    haveStartTime=true;
    startTransferTime.restart();
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
        #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
        readThread.fakeOpen();
        writeThread.fakeOpen();
        #else
        MoveFileExA();
        #endif
        return;
    }
    if(canBeCopiedDirectly())// is is_symlink(source);
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] need copied directly: "+source+" to "+destination);
        canBeCopiedDirectlyVariable=true;
        #if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
        readThread.fakeOpen();
        writeThread.fakeOpen();
        #else
        CopyFileExA();//copy symblink
        #endif
        return;
    }
#ifdef FSCOPYASYNC
    CopyFileExA();
#else
    tryOpen();
#endif
}

void TransferThreadAsync::tryOpen()
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

bool TransferThreadAsync::isSame()
{
    //check if source and destination is not the same
    //source.absoluteFilePath()==destination.absoluteFilePath() not work is source don't exists
    if(source==destination)
    {
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        if(!is_file(destination))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+source+" not exists");
        if(is_symlink(source))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+source+" isSymLink");
        if(is_symlink(destination))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start destination: "+destination+" isSymLink");
        #endif
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same but skip");
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

bool TransferThreadAsync::destinationExists()
{
    //check if destination exists
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("overwrite: %1, alwaysDoFileExistsAction: %2, readError: %3, writeError: %4")
                             .arg(fileExistsAction)
                             .arg(alwaysDoFileExistsAction)
                             .arg(readError)
                             .arg(writeError)
                             .toStdString()
                             );
    if(alwaysDoFileExistsAction==FileExists_Overwrite || readError || writeError
            #ifdef ULTRACOPIER_PLUGIN_RSYNC
            || rsync
            #endif
            )
        return false;
    bool destinationExists;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] time to first FS access");
    destinationExists=is_file(destination);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] finish first FS access");
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
        if(is_file(source))
        {
            if(fileExistsAction==FileExists_OverwriteIfNewer || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNewer))
            {
                if(readFileMDateTime(destination)<readFileMDateTime(source))
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
                if(readFileMDateTime(destination)>readFileMDateTime(source))
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
                if(readFileMDateTime(destination)!=readFileMDateTime(source) || destination.size()!=source.size())
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

/** \example
 * /dir1/dir2/file -> file
 * /dir1/dir2/ -> dir2
 * /dir1/ -> dir1
 * / -> root */
std::string TransferThreadAsync::resolvedName(const std::string &inode)
{
    const std::string::size_type &lastPos=inode.rfind('/');
    if(lastPos == std::string::npos || lastPos==0)
        return "root";
    if((lastPos+1)!=inode.size())
        return inode.substr(lastPos+1);
    if(inode.size()==1)
        return "root";
    const std::string::size_type &previousLastPos=inode.rfind('/',inode.size()-2);
    if((lastPos-2)==previousLastPos || previousLastPos == std::string::npos)
        return "root";
    return inode.substr(previousLastPos+1,lastPos-previousLastPos-1);
}

std::string TransferThreadAsync::getSourcePath() const
{
    return source;
}

std::string TransferThreadAsync::getDestinationPath() const
{
    return destination;
}

Ultracopier::CopyMode TransferThreadAsync::getMode() const
{
    return mode;
}

//return true if has been renamed
bool TransferThreadAsync::checkAlwaysRename()
{
    if(alwaysDoFileExistsAction==FileExists_Rename)
    {
        std::string newDestination=destination;
        std::string fileName=resolvedName(newDestination);
        std::string suffix;
        std::string newFileName;
        //resolv the suffix
        if(std::regex_match(fileName,renameRegex))
        {
            suffix=fileName;
            suffix=std::regex_replace(suffix,renameRegex,"$2");
            fileName=std::regex_replace(fileName,renameRegex,"$1");
        }
        //resolv the new name
        int num=1;
        do
        {
            if(num==1)
            {
                if(firstRenamingRule.empty())
                    newFileName=tr("%name% - copy").toStdString();
                else
                    newFileName=firstRenamingRule;
            }
            else
            {
                if(otherRenamingRule.empty())
                    newFileName=tr("%name% - copy (%number%)").toStdString();
                else
                    newFileName=otherRenamingRule;
                stringreplaceAll(newFileName,"%number%",std::to_string(num));
            }
            stringreplaceAll(newFileName,"%name%",fileName);
            stringreplaceAll(newFileName,"%suffix%",suffix);
            newDestination=FSabsolutePath(newDestination)+'/'+newFileName;
            num++;
        }
        while(is_file(newDestination));
        if(!renameTheOriginalDestination)
            destination=newDestination;
        else
        {
            if(rename(destination.c_str(),newDestination.c_str())!=0)
            {
                if(!is_file(destination))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+destination+": destination: "+newDestination+", error: "+std::to_string(errno));
                    emit errorOnFile(destination,tr("File not found").toStdString());
                    readError=true;
                    return true;
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do real move "+destination+": "+newDestination+", error: "+std::to_string(errno));
                readError=true;
                emit errorOnFile(destination,std::string(strerror(errno))+", errno: "+std::to_string(errno));
                return true;
            }
        }
        return true;
    }
    return false;
}

/// \warning not check if path have double //, if have do bug return failed
/// \warning check mkpath() call should not exists because only existing dest is allowed now
#ifdef Q_OS_UNIX
bool TransferThreadAsync::mkpath(const std::string &path, const mode_t &mode)
#else
bool TransferThreadAsync::mkpath(const std::string &path)
#endif
{
    char pathC[PATH_MAX];
    strcpy(pathC,path.c_str());
    #ifdef Q_OS_UNIX
    if(::mkdir(pathC, mode)==-1)
    #else
    if(::mkdir(pathC)==-1)
    #endif
        if(errno==EEXIST)
            return true;

    printf("%i",errno);
    //use reverse method to performance, more complex to code but less system call/fs call
    std::string::size_type previouspos=path.size();
    std::string::size_type lastpos=std::string::npos;

    char pathCedit[PATH_MAX];
    strcpy(pathCedit,pathC);
    std::vector<std::string::size_type> pathSplit;
    pathSplit.push_back(path.size());
    do
    {
        lastpos=path.rfind('/',previouspos-1);
        if(lastpos == std::string::npos)
            return false;

        while(pathC[lastpos-1]=='/')
            if(lastpos>0)
                lastpos--;
            else
                return false;

        //buggy case?
        #ifdef Q_OS_UNIX
        if(lastpos<2)
            return false;
        #else
        if(lastpos<4)
            return false;
        #endif

        pathCedit[lastpos]='\0';
        previouspos=lastpos;

        errno=0;
        #ifdef Q_OS_UNIX
        if(::mkdir(pathCedit, mode)==-1)
        #else
        if(::mkdir(pathCedit)==-1)
        #endif
            if(errno!=EEXIST && errno!=ENOENT)
                return false;
        //here errno can be: EEXIST, ENOENT, 0
        if(errno==ENOENT)
            pathSplit.push_back(lastpos);
    } while(lastpos>0 && errno==ENOENT);

    do
    {
        strcpy(pathCedit,pathC);
        lastpos=pathSplit.back();
        pathSplit.pop_back();
        pathCedit[lastpos]='\0';
        #ifdef Q_OS_UNIX
        if(::mkdir(pathCedit, mode)==-1)
        #else
        if(::mkdir(pathCedit)==-1)
        #endif
            if(errno!=EEXIST)
                return false;
    } while(!pathSplit.empty());
    return true;
}

bool TransferThreadAsync::mkdir(const std::string &file_path, const mode_t &mode)
{
    #ifdef Q_OS_UNIX
    return ::mkdir(file_path.c_str(),mode);
    #else
    (void)mode;
    return ::mkdir(file_path.c_str());
    #endif
}

void TransferThreadAsync::tryMoveDirectly()
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

#if defined(POSIXFILEMANIP) && defined(SYNCFILEMANIP)
void TransferThreadAsync::tryCopyDirectly()
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
endif

bool TransferThreadAsync::canBeMovedDirectly() const
{
    if(mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] mode!=Ultracopier::Move");
        return false;
    }
    return is_symlink(source) || driveManagement.isSameDrive(destination,source);
}

bool TransferThreadAsync::canBeCopiedDirectly() const
{
    return is_symlink(source);
}

void TransferThreadAsync::readIsReady()
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

void TransferThreadAsync::ifCanStartTransfer()
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

void TransferThreadAsync::writeIsReady()
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


//set the copy info and options before runing
void TransferThreadAsync::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
}

//set keep date
void TransferThreadAsync::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
}

//stop the current copy
void TransferThreadAsync::stop()
{
    stopIt=true;
    haveStartTime=false;
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

bool TransferThreadAsync::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

bool TransferThreadAsync::remainSourceOpen() const
{
    return (readIsOpenVariable || readIsOpeningVariable) && !readIsClosedVariable;
}

bool TransferThreadAsync::remainDestinationOpen() const
{
    return (writeIsOpenVariable || writeIsOpeningVariable) && !writeIsClosedVariable;
}

void TransferThreadAsync::readIsFinish()
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

void TransferThreadAsync::writeIsFinish()
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

void TransferThreadAsync::readIsClosed()
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

void TransferThreadAsync::writeIsClosed()
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
bool TransferThreadAsync::checkIfAllIsClosedAndDoOperations()
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
void TransferThreadAsync::postOperation()
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

bool TransferThreadAsync::doFilePostOperation()
{
    //do operation needed by copy
    //set the time if no write thread used

    if(!exists(destination) && !is_symlink(destination))
    {
        if(!stopIt)
            if(/*true when the destination have been remove but not the symlink:*/!is_symlink(source))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date: File not found");
                emit errorOnFile(destination,tr("Unable to change the date").toStdString()+": "+tr("File not found").toStdString());
                return false;
            }
    }
    else
    {
        if(doRightTransfer)
        {
            //should be never used but...
            /*source.refresh();
            if(source.exists())*/
            if(havePermission)
            {
                if(!writeDestinationFilePermissions(destination))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to set the destination file permission");
                    //emit errorOnFile(destination,tr("Unable to set the destination file permission"));
                    //return false;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try doRightTransfer when source not exists");
        }
        //date at end because previous change can touch the file
        if(doTheDateTransfer)
        {
            if(!writeDestinationFileDateTime(destination))
            {
                if(!is_file(destination))
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date (is not a file)");
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date");
                /* error with virtual folder under windows */
                #ifndef Q_OS_WIN32
                if(keepDate)
                {
                    emit errorOnFile(destination,tr("Unable to change the date").toStdString());
                    return false;
                }
                #endif
            }
            /*else -> need be done at source m time read
            {
                #ifndef Q_OS_WIN32
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] read the destination time: "+destination.lastModified().toString().toStdString());
                if(destination.lastModified()<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] read the destination time lower than min time: "+destination.lastModified().toString().toStdString());
                    if(keepDate)
                    {
                        emit errorOnFile(destination,tr("Unable to change the date").toStdString());
                        return false;
                    }
                }
                #endif
            }*/
        }

    }
    if(stopIt)
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThreadAsync::getWriteError()
{
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in write error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError		= true;
    writeError			= true;
    writeIsReadyVariable		= false;
    writeError_source_seeked	= false;
    writeError_destination_reopened	= false;
    writeIsOpeningVariable=false;
    if(!readError)//already display error for the read
        emit errorOnFile(destination,writeThread.errorString());
}

void TransferThreadAsync::getReadError()
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

void TransferThreadAsync::writeThreadIsReopened()
{
    if(writeError_destination_reopened)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeError_destination_reopened=true;
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThreadAsync::readThreadIsSeekToZeroAndWait()
{
    if(writeError_source_seeked)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeError_source_seeked=true;
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThreadAsync::resumeTransferAfterWriteError()
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

void TransferThreadAsync::readThreadResumeAfterError()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readError=false;
    writeIsReady();
    readIsReady();
}

//////////////////////////////////////////////////////////////////
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThreadAsync::readIsStopped()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] TransferThreadAsync::readIsStopped()");
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

void TransferThreadAsync::writeIsStopped()
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

int64_t TransferThreadAsync::readFileMDateTime(const std::string &source)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"readFileDateTime("+source+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        struct stat info;
        if(stat(source.c_str(),&info)!=0)
            return -1;
        return info.st_mtim.tv_sec;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(source.c_str(),&info)!=0)
                    return -1;
                return info.st_mtime;
            #else
                /*wchar_t filePath[65535];
                if(std::regex_match(source,std::regex("^[a-zA-Z]:")))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';*/
                std::wstring wsource=std::wstring(source.cbegin(),source.cend());
                HANDLE hFileSouce = CreateFileW(wsource.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                    return -1;
                FILETIME ftCreate, ftAccess, ftWrite;
                if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileSouce);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to get the file time");
                    return -1;
                }
                CloseHandle(hFileSouce);
                return this->ftWriteL=ftWrite.dwLowDateTime + 2^32 * ftWrite.dwHighDateTime;
            #endif
        #else
            return -1;
        #endif
    #endif
    return -1;
}

//fonction to read the file date time
bool TransferThreadAsync::readSourceFileDateTime(const std::string &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readFileDateTime("+source+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        struct stat info;
        if(stat(source.c_str(),&info)!=0)
            return false;
        time_t ctime=info.st_ctim.tv_sec;
        time_t actime=info.st_atim.tv_sec;
        time_t modtime=info.st_mtim.tv_sec;
        //this function avalaible on unix and mingw
        butime.actime=actime;
        butime.modtime=modtime;
        if((uint64_t)modtime<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+source+": "+source);
            return false;
        }
        Q_UNUSED(ctime);
        return true;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(source.c_str(),&info)!=0)
                    return false;
                time_t ctime=info.st_ctime;
                time_t actime=info.st_atime;
                time_t modtime=info.st_mtime;
                //this function avalaible on unix and mingw
                butime.actime=actime;
                butime.modtime=modtime;
                if((uint64_t)modtime<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+source+": "+source);
                    return false;
                }
                Q_UNUSED(ctime);
                return true;
            #else
                wchar_t filePath[65535];
                if(std::regex_match(source,regRead))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                HANDLE hFileSouce = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] open failed to read: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileSouce);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to get the file time");
                    return false;
                }
                this->ftCreateL=ftCreate.dwLowDateTime;
                this->ftCreateH=ftCreate.dwHighDateTime;
                this->ftAccessL=ftAccess.dwLowDateTime;
                this->ftAccessH=ftAccess.dwHighDateTime;
                this->ftWriteL=ftWrite.dwLowDateTime;
                this->ftWriteH=ftWrite.dwHighDateTime;
                CloseHandle(hFileSouce);
                const uint64_t modtime=(uint64_t)ftWrite.dwLowDateTime + (uint64_t)2^32 * (uint64_t)ftWrite.dwHighDateTime;
                if(modtime<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+source+": "+source.lastModified().toString().toStdString());
                    return false;
                }
                return true;
            #endif
        #else
            return false;
        #endif
    #endif
    return false;
}

bool TransferThreadAsync::writeDestinationFileDateTime(const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeFileDateTime("+destination+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        return utime(destination.c_str(),&butime)==0;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                return utime(destination.c_str(),&butime)==0;
            #else
                wchar_t filePath[65535];
                if(std::regex_match(destination,regRead))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+destination.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(destination.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                HANDLE hFileDestination = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if(hFileDestination == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] open failed to write: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to set the file time");
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

bool TransferThreadAsync::readSourceFilePermissions(const std::string &source)
{
    #ifdef Q_OS_UNIX
    if(stat(source.c_str(), &permissions)!=0)
        return false;
    else
        return true;
    #else
    HANDLE hFile = CreateFileA(source.c_str(), READ_CONTROL | ACCESS_SYSTEM_SECURITY ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateFile() failed. Error: INVALID_HANDLE_VALUE");
      return false;
    }
    DWORD lasterror = GetSecurityInfo(hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                NULL, NULL, &dacl, NULL, &PSecurityD);
    if (lasterror != ERROR_SUCCESS) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] GetSecurityInfo() failed. Error"+std::to_string(lasterror));
      return false;
    }
    CloseHandle(hFile);
    return true;
    #endif
}

bool TransferThreadAsync::writeDestinationFilePermissions(const std::string &destination)
{
    #ifdef Q_OS_UNIX
    if(chmod(destination.c_str(), permissions.st_mode)!=0)
        return false;
    if(chown(destination.c_str(), permissions.st_uid, permissions.st_gid)!=0)
        return false;
    return true;
    #else
    HANDLE hFile = CreateFileA(destination.c_str(),READ_CONTROL | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY,0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateFile() failed. Error: INVALID_HANDLE_VALUE");
      return false;
    }
    DWORD lasterror = SetSecurityInfo(hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION , NULL, NULL, dacl, NULL);
    if (lasterror != ERROR_SUCCESS) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] SetSecurityInfo() failed. Error"+std::to_string(lasterror));
      return false;
    }
    CloseHandle(hFile);
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
    return true;
    #endif
}

//skip the copy
void TransferThreadAsync::skip()
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
int64_t TransferThreadAsync::copiedSize()
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
    this->id=id;
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
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
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

void TransferThreadAsync::setRenamingRules(const std::string &firstRenamingRule, const std::string &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}

void TransferThreadAsync::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
}

void TransferThreadAsync::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    this->renameTheOriginalDestination=renameTheOriginalDestination;
}

void TransferThreadAsync::set_updateMount()
{
    driveManagement.tryUpdate();
}

bool TransferThreadAsync::is_symlink(const std::string &filename)
{
    return is_symlink(filename.c_str());
}

bool TransferThreadAsync::is_symlink(const char * const filename)
{
    #ifdef Q_OS_WIN32
    (void)filename;
    return true;
    #else
    struct stat p_statbuf;
    if (lstat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    if (S_ISLNK(p_statbuf.st_mode) == 1)
        return true;
    #endif
    return false;
}

bool TransferThreadAsync::is_file(const std::string &filename)
{
    return is_file(filename.c_str());
}

bool TransferThreadAsync::is_file(const char * const filename)
{
    struct stat p_statbuf;
    #ifdef Q_OS_WIN32
    if (stat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #else
    if (lstat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #endif
    if (S_ISREG(p_statbuf.st_mode) == 1)
        return true;
    return true;
}

bool TransferThreadAsync::is_dir(const std::string &filename)
{
    return is_dir(filename.c_str());
}

bool TransferThreadAsync::is_dir(const char * const filename)
{
    struct stat p_statbuf;
    #ifdef Q_OS_WIN32
    if (stat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #else
    if (lstat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #endif
    if (S_ISDIR(p_statbuf.st_mode) == 1)
        return true;
    return true;
}

bool TransferThreadAsync::exists(const std::string &filename)
{
    return is_dir(filename.c_str());
}

bool TransferThreadAsync::exists(const char * const filename)
{
    struct stat p_statbuf;
    #ifdef Q_OS_WIN32
    if (stat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #else
    if (lstat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return false;
    #endif
    return true;
}

int64_t TransferThreadAsync::file_stat_size(const std::string &filename)
{
    return file_stat_size(filename.c_str());
}

int64_t TransferThreadAsync::file_stat_size(const char * const filename)
{
    struct stat p_statbuf;
    #ifdef Q_OS_WIN32
    if (stat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return -1;
    #else
    if (lstat(filename, &p_statbuf) < 0)
        //if error or file not exists, considere as regular file
        return -1;
    #endif
    return p_statbuf.st_size;
}

bool TransferThreadAsync::entryInfoList(const std::string &path,std::vector<std::string> &list)
{
    DIR *dp;
    struct dirent *ep;
    dp=opendir(path.c_str());
    if(dp!=NULL)
    {
        do {
            ep=readdir(dp);
            const std::string name(ep->d_name);
            if(name!="." && name!="..")
                list.push_back(ep->d_name);
        } while(ep);
        (void) closedir(dp);
        return true;
    }
    return false;
}

#ifdef Q_OS_UNIX
bool TransferThreadAsync::entryInfoList(const std::string &path,std::vector<dirent_uc> &list)
{
    DIR *dp;
    struct dirent *ep;
    dp=opendir(path.c_str());
    if(dp!=NULL)
    {
        do {
            ep=readdir(dp);
            if(ep!=NULL)
            {
                const std::string name(ep->d_name);
                if(name!="." && name!="..")
                {
                    dirent_uc tempValue;
                    #if defined(__HAIKU__)
                    struct stat sp;
                    memset(&sp,0,sizeof(sp));
                    if(stat(ep->d_name, &sp))
                        tempValue.isFolder=S_ISDIR(sp.st_mode);
                    else
                        tempValue.isFolder=false;
                    #else
                    tempValue.isFolder=ep->d_type==DT_DIR;
                    #endif
                    strcpy(tempValue.d_name,ep->d_name);
                    list.push_back(tempValue);
                }
            }
        } while(ep!=NULL);
        (void) closedir(dp);
        return true;
    }
    return false;
}
#else
bool TransferThreadAsync::entryInfoList(const std::string &path,std::vector<dirent_uc> &list)
{
    WIN32_FIND_DATAA fdFile;
    HANDLE hFind = NULL;
    char finalpath[MAX_PATH];
    strcpy(finalpath,path.c_str());
    strcat(finalpath,"\\*");
    if((hFind = FindFirstFileA(finalpath, &fdFile)) == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        if(strcmp(fdFile.cFileName, ".")!=0 && strcmp(fdFile.cFileName, "..")!=0)
        {
            dirent_uc tempValue;
            tempValue.isFolder=fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            strcpy(tempValue.d_name,fdFile.cFileName);
            tempValue.size=(fdFile.nFileSizeHigh*(MAXDWORD+1))+fdFile.nFileSizeLow;
            list.push_back(tempValue);
        }
    }
    while(FindNextFileA(hFind, &fdFile));
    FindClose(hFind);
    return true;
}
#endif

void TransferThreadAsync::setMkFullPath(const bool mkFullPath)
{
    this->mkFullPath=mkFullPath;
}

int TransferThreadAsync::fseeko64(FILE *__stream, uint64_t __off, int __whence)
{
    #ifdef __HAIKU__
    return ::fseeko(__stream,__off,__whence);
    #else
    return ::fseeko64(__stream,__off,__whence);
    #endif
}

int TransferThreadAsync::ftruncate64(int __fd, uint64_t __length)
{
    #ifdef __HAIKU__
    return ftruncate(__fd,__length);
    #else
    return ftruncate64(__fd,__length);
    #endif
}
