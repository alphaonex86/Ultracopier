//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThread.h"
#include <string>
#include <dirent.h>
#include <limits.h>
#ifdef WIDESTRING
#include <locale>
#include <codecvt>
#endif

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#endif

#include "../../../cpp11addition.h"

TransferThread::TransferThread() :
    mode(Ultracopier::CopyMode::Copy),
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
    renameTheOriginalDestination    (false),
    writeError                      (false),
    readError                       (false),
    havePermission                  (false),
    haveTransferTime                (false)
{
    id=0;
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
    #ifdef Q_OS_Win32
    stopItWin=0;
    #endif
    //if not QThread
    run();
}

TransferThread::~TransferThread()
{
    #ifdef Q_OS_WIN32
    stopItWin=1;
    #endif
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
        //free(dacl);
        dacl=NULL;
    }
    #endif
}

void TransferThread::run()
{
    moveToThread(this);

    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    transfer_stat			= TransferStat_Idle;
    stopIt                  = false;
    fileExistsAction        = FileExists_NotSet;
    alwaysDoFileExistsAction= FileExists_NotSet;

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&driveManagement,&DriveManagement::debugInformation,this,                   &TransferThread::debugInformation,	Qt::QueuedConnection))
        abort();
    #endif
}

TransferStat TransferThread::getStat() const
{
    return transfer_stat;
}

#ifdef WIDESTRING
std::wstring TransferThread::stringToWstring(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(utf8);
}

std::string TransferThread::wstringTostring(const std::wstring& utf16)
{

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
    return conv1.to_bytes(utf16);
}
#else
std::string TransferThread::stringToWstring(const std::string& utf8)
{
    return utf8;
}

std::string TransferThread::wstringTostring(const std::string& utf16)
{

    return utf16;
}
#endif

bool TransferThread::setFiles(const INTERNALTYPEPATH& source, const int64_t &size, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+
                                 TransferThread::wstringTostring(source)+", destination: "+TransferThread::wstringTostring(destination));
        return false;
    }
    //to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
    transfer_stat			= TransferStat_PreOperation;
    //emit pushStat(stat,transferId);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+TransferThread::wstringTostring(source)+", destination: "+TransferThread::wstringTostring(destination));
    this->source                    = source;
    this->destination               = destination;
    this->mode                      = mode;
    this->size                      = size;
    stopIt                          = false;
    #ifdef Q_OS_WIN32
    stopItWin=0;
    #endif
    fileExistsAction                = FileExists_NotSet;
    canStartTransfer                = false;
    sended_state_preOperationStopped= false;
    fileContentError                = false;
    writeError                      = false;
    readError                       = false;
    haveTransferTime                = false;
    resetExtraVariable();
    emit internalStartPreOperation();
    startTransferTime.restart();
    return true;
}

void TransferThread::setFileRename(const std::string &nameForRename)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+
                                 TransferThread::wstringTostring(source)+(", destination: ")+TransferThread::wstringTostring(destination));
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
        destination=destination+TransferThread::stringToWstring("/")+TransferThread::stringToWstring(nameForRename);
    else
    {
        INTERNALTYPEPATH tempDestination=destination;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] rename "+TransferThread::wstringTostring(destination)+
                                 ": to: "+TransferThread::wstringTostring(destination)+"/"+nameForRename);
        if(!rename(destination,(destination+TransferThread::stringToWstring("/")+TransferThread::stringToWstring(nameForRename))))
        {
            if(!is_file(destination))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+TransferThread::wstringTostring(destination)+
                                         ": destination: "+TransferThread::wstringTostring(destination)+", error: "+std::to_string(errno));
                emit errorOnFile(destination,tr("File not found").toStdString());
                return;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do real move "+TransferThread::wstringTostring(destination)+
                                         ": "+TransferThread::wstringTostring(destination)+", error: "+std::to_string(errno));
            emit errorOnFile(destination,"errno: "+std::string(strerror(errno)));
            return;
        }
        if(source==destination)
            source=destination+TransferThread::stringToWstring("/")+TransferThread::stringToWstring(nameForRename);
        destination=tempDestination;
    }
    fileExistsAction	= FileExists_NotSet;
    resetExtraVariable();
    emit internalStartPreOperation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination is: "+TransferThread::wstringTostring(destination));
}

bool TransferThread::rename(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination)
{
    return ::rename(TransferThread::wstringTostring(source).c_str(),
                    TransferThread::wstringTostring(destination).c_str())==0;
}

void TransferThread::setAlwaysFileExistsAction(const FileExistsAction &action)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] action to do always: ")+QString::number(action)));
    alwaysDoFileExistsAction=action;
}

void TransferThread::resetExtraVariable()
{
    sended_state_preOperationStopped=false;
    writeError                  = false;
    readError                   = false;
    needRemove                  = false;
    needSkip                    = false;
    retry                       = false;
    havePermission              = false;
}

bool TransferThread::isSame()
{
    //check if source and destination is not the same
    //source.absoluteFilePath()==destination.absoluteFilePath() not work is source don't exists
    if(source==destination)
    {
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        if(!is_file(destination))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+TransferThread::wstringTostring(source)+" not exists");
        if(is_symlink(source))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+TransferThread::wstringTostring(source)+" isSymLink");
        if(is_symlink(destination))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start destination: "+TransferThread::wstringTostring(destination)+" isSymLink");
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

bool TransferThread::destinationExists()
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] finish first FS access: "+std::to_string(destinationExists));
    if(destinationExists)
    {
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            transfer_stat=TransferStat_Idle;
            emit postOperationStopped();
            //quit
            return true;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
        if(checkAlwaysRename())
            return false;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
        if(is_file(source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
            if(fileExistsAction!=FileExists_NotSet)
            {
                transfer_stat=TransferStat_Idle;
                emit postOperationStopped();
                return true;
            }
        }
        if(fileExistsAction==FileExists_NotSet)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] FS access");
            emit fileAlreadyExists(source,destination,false);
            return true;
        }
    }
    return false;
}

/** \example
 * /dir1/dir2/file -> file
 * /file -> file
 * /dir1/dir2/ -> dir2
 * /dir1/ -> dir1
 * / -> root */
#ifdef Q_OS_WIN32
std::string TransferThread::resolvedName(std::string inode)
#else
std::string TransferThread::resolvedName(const std::string &inode)
#endif
{
    #ifdef Q_OS_WIN32
    stringreplaceAll(inode,"\\","/");
    #endif
    const std::string::size_type &lastPos=inode.rfind('/');
    if(lastPos == std::string::npos || (lastPos==0 && inode.size()==1))
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

#ifdef Q_OS_WIN32
std::wstring TransferThread::resolvedName(std::wstring inode)
#else
std::wstring TransferThread::resolvedName(const std::wstring &inode)
#endif
{
    #ifdef Q_OS_WIN32
    stringreplaceAll(inode,L"\\",L"/");
    #endif
    const std::wstring::size_type &lastPos=inode.rfind(L'/');
    if(lastPos == std::wstring::npos || (lastPos==0 && inode.size()==1))
        return L"root";
    if((lastPos+1)!=inode.size())
        return inode.substr(lastPos+1);
    if(inode.size()==1)
        return L"root";
    const std::wstring::size_type &previousLastPos=inode.rfind(L'/',inode.size()-2);
    if((lastPos-2)==previousLastPos || previousLastPos == std::wstring::npos)
        return L"root";
    return inode.substr(previousLastPos+1,lastPos-previousLastPos-1);
}

INTERNALTYPEPATH TransferThread::getSourcePath() const
{
    return source;
}

INTERNALTYPEPATH TransferThread::getDestinationPath() const
{
    return destination;
}

Ultracopier::CopyMode TransferThread::getMode() const
{
    return mode;
}

//return true if has been renamed
bool TransferThread::checkAlwaysRename()
{
    if(alwaysDoFileExistsAction==FileExists_Rename)
    {
        INTERNALTYPEPATH newDestination=destination;
        std::string fileName=resolvedName(TransferThread::wstringTostring(newDestination));
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
            newDestination=FSabsolutePath(newDestination)+
                    TransferThread::stringToWstring("/")+TransferThread::stringToWstring(newFileName);
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] source not exists "+TransferThread::wstringTostring(destination)+
                                             ": destination: "+TransferThread::wstringTostring(newDestination)+", error: "+std::to_string(errno));
                    emit errorOnFile(destination,tr("File not found").toStdString());
                    readError=true;
                    return true;
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to do real move "+TransferThread::wstringTostring(destination)+
                                             ": "+TransferThread::wstringTostring(newDestination)+", error: "+std::to_string(errno));
                readError=true;
                emit errorOnFile(destination,std::string(strerror(errno))+", errno: "+std::string(strerror(errno)));
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
bool TransferThread::mkpath(const INTERNALTYPEPATH &path, const mode_t &mode)
#else
bool TransferThread::mkpath(const INTERNALTYPEPATH &path)
#endif
{
    char pathC[PATH_MAX];
    strcpy(pathC,TransferThread::wstringTostring(path).c_str());
    if(!mkdir(path))
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

bool TransferThread::mkdir(const INTERNALTYPEPATH &file_path, const mode_t &mode)
{
    #ifdef Q_OS_UNIX
    return ::mkdir(TransferThread::wstringTostring(file_path).c_str(),mode)==0;
    #else
    (void)mode;
    return ::mkdir(TransferThread::wstringTostring(file_path).c_str())==0;
    #endif
}

bool TransferThread::canBeMovedDirectly() const
{
    if(mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] mode!=Ultracopier::Move");
        return false;
    }
    return is_symlink(source) || driveManagement.isSameDrive(
                TransferThread::wstringTostring(destination),
                TransferThread::wstringTostring(source)
                );
}

bool TransferThread::canBeCopiedDirectly() const
{
    return is_symlink(source);
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

bool TransferThread::doFilePostOperation()
{
    //do operation needed by copy
    //set the time if no write thread used

    if(/*not implied by is_symlink, exist can be false because symlink dest not exists*/
            !exists(destination) && !is_symlink(destination))
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
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

int64_t TransferThread::readFileMDateTime(const INTERNALTYPEPATH &source)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"readFileDateTime("+source+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        struct stat info;
        if(stat(TransferThread::wstringTostring(source).c_str(),&info)!=0)
            return -1;
        #ifdef Q_OS_MAC
        return info.st_mtimespec.tv_sec;
        #else
        return info.st_mtim.tv_sec;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(TransferThread::wstringTostring(source).c_str(),&info)!=0)
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to get the file time: "+std::to_string(GetLastError()));
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
bool TransferThread::readSourceFileDateTime(const INTERNALTYPEPATH &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readFileDateTime("+TransferThread::wstringTostring(source)+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        struct stat info;
        if(stat(TransferThread::wstringTostring(source).c_str(),&info)!=0)
            return false;
        #ifdef Q_OS_MAC
        time_t ctime=info.st_ctimespec.tv_sec;
        time_t actime=info.st_atimespec.tv_sec;
        time_t modtime=info.st_mtimespec.tv_sec;
        //this function avalaible on unix and mingw
        butime.actime=actime;
        butime.modtime=modtime;
        #else
        time_t ctime=info.st_ctim.tv_sec;
        time_t actime=info.st_atim.tv_sec;
        time_t modtime=info.st_mtim.tv_sec;
        //this function avalaible on unix and mingw
        butime.actime=actime;
        butime.modtime=modtime;
        #endif
        if((uint64_t)modtime<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+TransferThread::wstringTostring(source));
            return false;
        }
        Q_UNUSED(ctime);
        return true;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(TransferThread::wstringTostring(source).c_str(),&info)!=0)
                    return false;
                time_t ctime=info.st_ctime;
                time_t actime=info.st_atime;
                time_t modtime=info.st_mtime;
                //this function avalaible on unix and mingw
                butime.actime=actime;
                butime.modtime=modtime;
                if((uint64_t)modtime<ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+TransferThread::wstringTostring(source));
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

bool TransferThread::writeDestinationFileDateTime(const INTERNALTYPEPATH &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeFileDateTime("+TransferThread::wstringTostring(destination)+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        return utime(TransferThread::wstringTostring(destination).c_str(),&butime)==0;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                return utime(TransferThread::wstringTostring(destination).c_str(),&butime)==0;
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

bool TransferThread::readSourceFilePermissions(const INTERNALTYPEPATH &source)
{
    #ifdef Q_OS_UNIX
    if(stat(TransferThread::wstringTostring(source).c_str(), &permissions)!=0)
        return false;
    else
        return true;
    #else
    HANDLE hFile = CreateFileW(source.c_str(), GENERIC_READ ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateFile() failed. Error: INVALID_HANDLE_VALUE: "+std::to_string(GetLastError()));
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

bool TransferThread::writeDestinationFilePermissions(const INTERNALTYPEPATH &destination)
{
    #ifdef Q_OS_UNIX
    if(chmod(TransferThread::wstringTostring(destination).c_str(), permissions.st_mode)!=0)
        return false;
    if(chown(TransferThread::wstringTostring(destination).c_str(), permissions.st_uid, permissions.st_gid)!=0)
        return false;
    return true;
    #else
    HANDLE hFile = CreateFileW(destination.c_str(),READ_CONTROL | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY,0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] CreateFile() failed. Error: INVALID_HANDLE_VALUE: "+std::to_string(GetLastError()));
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
        //free(dacl);
        dacl=NULL;
    }
    return true;
    #endif
}

//retry after error
void TransferThread::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void TransferThread::setRsync(const bool rsync)
{
    this->rsync=rsync;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThread::setId(int id)
{
    this->id=id;
}
#endif

void TransferThread::setRenamingRules(const std::string &firstRenamingRule, const std::string &otherRenamingRule)
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

void TransferThread::set_updateMount()
{
    driveManagement.tryUpdate();
}

bool TransferThread::is_symlink(const INTERNALTYPEPATH &filename)
{
    return is_symlink(TransferThread::wstringTostring(filename).c_str());
}

bool TransferThread::is_symlink(const char * const filename)
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

bool TransferThread::is_file(const INTERNALTYPEPATH &filename)
{
    return is_file(TransferThread::wstringTostring(filename).c_str());
}

bool TransferThread::is_file(const char * const filename)
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

bool TransferThread::is_dir(const INTERNALTYPEPATH &filename)
{
    return is_dir(TransferThread::wstringTostring(filename).c_str());
}

bool TransferThread::is_dir(const char * const filename)
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

bool TransferThread::exists(const INTERNALTYPEPATH &filename)
{
    return is_dir(TransferThread::wstringTostring(filename).c_str());
}

bool TransferThread::exists(const char * const filename)
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

int64_t TransferThread::file_stat_size(const INTERNALTYPEPATH &filename)
{
    return file_stat_size(TransferThread::wstringTostring(filename).c_str());
}

int64_t TransferThread::file_stat_size(const char * const filename)
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

bool TransferThread::entryInfoList(const INTERNALTYPEPATH &path,std::vector<INTERNALTYPEPATH> &list)
{
    std::vector<dirent_uc> listTemp;
    if(!TransferThread::entryInfoList(path,listTemp))
        return false;
    for(const dirent_uc &u : listTemp)
        list.push_back(u.d_name);
    return true;
}

bool TransferThread::rmdir(const INTERNALTYPEPATH &path)
{
    return ::rmdir(TransferThread::wstringTostring(path).c_str())==0;
}

#ifdef Q_OS_UNIX
bool TransferThread::entryInfoList(const INTERNALTYPEPATH &path,std::vector<dirent_uc> &list)
{
    DIR *dp;
    struct dirent *ep;
    dp=opendir(TransferThread::wstringTostring(path).c_str());
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
                    tempValue.d_name=TransferThread::stringToWstring(ep->d_name);
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
bool TransferThread::entryInfoList(const INTERNALTYPEPATH &path,std::vector<dirent_uc> &list)
{
    HANDLE hFind = NULL;
    #ifdef WIDESTRING
    WIN32_FIND_DATAW fdFile;
    wchar_t finalpath[MAX_PATH];
    wcscpy(finalpath,path.c_str());
    wcscat(finalpath,L"\\*");
    #else
    WIN32_FIND_DATAA fdFile;
    char finalpath[MAX_PATH];
    strcpy(finalpath,path.c_str());
    strcat(finalpath,"\\*");
    #endif
    if((hFind = FindFirstFileW(finalpath, &fdFile)) == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        #ifdef WIDESTRING
        if(wcscmp(fdFile.cFileName, L".")!=0 && wcscmp(fdFile.cFileName, L"..")!=0)
        #else
        if(strcmp(fdFile.cFileName, ".")!=0 && strcmp(fdFile.cFileName, "..")!=0)
        #endif
        {
            dirent_uc tempValue;
            tempValue.isFolder=fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            tempValue.d_name=fdFile.cFileName;
            tempValue.size=(fdFile.nFileSizeHigh*(MAXDWORD+1))+fdFile.nFileSizeLow;
            list.push_back(tempValue);
        }
    }
    while(FindNextFileW(hFind, &fdFile));
    FindClose(hFind);
    return true;
}
#endif

void TransferThread::setMkFullPath(const bool mkFullPath)
{
    this->mkFullPath=mkFullPath;
}

int TransferThread::fseeko64(FILE *__stream, uint64_t __off, int __whence)
{
    #if defined(__HAIKU__) || defined(Q_OS_MAC) || defined(ANDROID) || defined(__ANDROID_API__)
    return ::fseeko(__stream,__off,__whence);
    #else
    return ::fseeko64(__stream,__off,__whence);
    #endif
}

int TransferThread::ftruncate64(int __fd, uint64_t __length)
{
    #if defined(__HAIKU__) || defined(Q_OS_MAC) || defined(ANDROID) || defined(__ANDROID_API__)
    return ::ftruncate(__fd,__length);
    #else
    return ::ftruncate64(__fd,__length);
    #endif
}

int64_t TransferThread::transferTime() const
{
    return startTransferTime.elapsed();
}
