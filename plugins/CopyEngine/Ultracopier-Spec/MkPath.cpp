#include "MkPath.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#endif

#ifdef Q_OS_WIN32
    #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
    #endif
#endif

std::string MkPath::text_slash="/";

MkPath::MkPath() :
    waitAction(false),
    stopIt(false),
    doRightTransfer(false),
    keepDate(false),
    mkFullPath(false),
    doTheDateTransfer(false)
{
    setObjectName("MkPath");
    moveToThread(this);
    start();
    #ifdef Q_OS_WIN32
        #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            regRead=std::regex("^[a-zA-Z]:");
        #endif
    #endif
}

MkPath::~MkPath()
{
    stopIt=true;
    quit();
    wait();
}

void MkPath::addPath(const std::string& source, const std::string& destination, const ActionType &actionType)
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(destination.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
    if(source.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source+", destination: "+destination);
    if(stopIt)
        return;
    emit internalStartAddPath(source,destination,actionType);
}

void MkPath::skip()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit internalStartSkip();
}

void MkPath::retry()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit internalStartRetry();
}

void MkPath::run()
{
    connect(this,&MkPath::internalStartAddPath,     this,&MkPath::internalAddPath,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartDoThisPath,	this,&MkPath::internalDoThisPath,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartSkip,        this,&MkPath::internalSkip,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartRetry,       this,&MkPath::internalRetry,Qt::QueuedConnection);
    exec();
}

void MkPath::internalDoThisPath()
{
    if(waitAction || pathList.empty())
        return;
    const Item &item=pathList.front();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(item.destination.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
    if(item.source.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+item.source+", destination: "+
                             item.destination+", move: "+std::to_string(item.actionType));
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(item.actionType==ActionType_RmSync)
    {
        if(item.destination.isFile())
        {
            QFile removedFile(item.destination.absoluteFilePath());
            if(!removedFile.remove())
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the inode: "+item.destination+", error: "+removedFile.errorString().toStdString());
                emit errorOnFolder(item.destination,removedFile.errorString().toStdString());
                return;
            }
        }
        else if(!rmpath(item.destination.absoluteFilePath()))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the inode: "+item.destination);
            emit errorOnFolder(item.destination,tr("Unable to remove").toStdString());
            return;
        }
        pathList.removeFirst();
        emit firstFolderFinish();
        checkIfCanDoTheNext();
        return;
    }
    #endif
    doTheDateTransfer=false;
    if(keepDate)
    {
        const int64_t &sourceLastModified=TransferThread::readFileMDateTime(item.source);
        if(!TransferThread::exists(item.source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources not exists: "+item.source);
            doTheDateTransfer=false;
        }
        else if(ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS>=sourceLastModified)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources is older to copy the time: "+item.source+
                                     ": "+QDateTime::fromSecsSinceEpoch(ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS).toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString()+
                                     ">="+QDateTime::fromSecsSinceEpoch(sourceLastModified).toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString());
            doTheDateTransfer=false;
        }
        else
        {
            doTheDateTransfer=readFileDateTime(item.source);
            /*if(!doTheDateTransfer)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get source folder time: "+item.source.absoluteFilePath());
                emit errorOnFolder(item.source,tr("Unable to get time"));
                return;
            }*/
        }
    }
    if(TransferThread::is_dir(item.destination) && item.actionType==ActionType_RealMove)
        pathList.front().actionType=ActionType_MovePath;
    if(item.actionType!=ActionType_RealMove)
    {
        if(!TransferThread::is_dir(item.destination))
        {
            if(mkFullPath)
            {
                if(!TransferThread::mkpath(item.destination))
                {
                    if(!TransferThread::is_dir(item.destination))
                    {
                        if(stopIt)
                            return;
                        waitAction=true;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+item.destination+", errno: "+std::to_string(errno));
                        emit errorOnFolder(item.destination,tr("Unable to create the folder").toStdString());
                        return;
                    }
                }
            }
            else
            {
                if(!TransferThread::mkdir(item.destination))
                {
                    if(!TransferThread::is_dir(item.destination))
                    {
                        if(stopIt)
                            return;
                        waitAction=true;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+item.destination+", errno: "+std::to_string(errno));
                        emit errorOnFolder(item.destination,tr("Unable to create the folder").toStdString());
                        return;
                    }
                }
            }
        }
    }
    else
    {
        if(!TransferThread::is_dir(item.source))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The source folder don't exists: "+item.source);
            emit errorOnFolder(item.destination,tr("The source folder don't exists").toStdString());
            return;
        }
        if(!TransferThread::is_dir(item.source))//it's really an error?
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The source is not a folder: "+item.source);
            /*if(stopIt)
                return;
            waitAction=true;
            emit errorOnFolder(item.destination,tr("The source is not a folder"));
            return;*/
        }
        if(stringStartWith(item.destination,(item.source+text_slash)))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"move into it self: "+item.destination);
            int random=rand();
            std::string tempFolder=FSabsolutePath(item.source)+text_slash+std::to_string(random);
            while(TransferThread::is_dir(tempFolder))
            {
                random=rand();
                tempFolder=FSabsolutePath(item.source)+text_slash+std::to_string(random);
            }
            if(rename(item.source.c_str(),tempFolder.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to temporary rename the folder: "+item.destination);
                emit errorOnFolder(item.destination,tr("Unable to temporary rename the folder").toStdString());
                return;
            }
            /* http://doc.qt.io/qt-5/qdir.html#rename
             * On most file systems, rename() fails only if oldName does not exist, or if a file with the new name already exists.
            if(!dir.mkpath(FSabsolutePath(item.destination)))
            {
                if(!dir.exists(FSabsolutePath(item.destination)))
                {
                    if(stopIt)
                        return;
                    waitAction=true;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+item.destination.absoluteFilePath());
                    emit errorOnFolder(item.destination,tr("Unable to create the folder"));
                    return;
                }
            }*/
            if(rename(tempFolder.c_str(),item.destination.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to do the final real move the folder: "+item.destination);
                emit errorOnFolder(item.destination,tr("Unable to do the final real move the folder").toStdString());
                return;
            }
        }
        else
        {
            /* http://doc.qt.io/qt-5/qdir.html#rename
             * On most file systems, rename() fails only if oldName does not exist, or if a file with the new name already exists.
            if(!dir.mkpath(FSabsolutePath(item.destination)))
            {
                if(!dir.exists(FSabsolutePath(item.destination)))
                {
                    if(stopIt)
                        return;
                    waitAction=true;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+item.destination.absoluteFilePath());
                    emit errorOnFolder(item.destination,tr("Unable to create the folder"));
                    return;
                }
            }*/
            if(rename(item.source.c_str(),item.destination.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: from: "+item.source+", soruce exists: "+
                                         std::to_string(TransferThread::is_dir(item.source))+", to: "+item.destination
                                         +", destination exist: "+std::to_string(TransferThread::is_dir(item.destination))
                                         );
                emit errorOnFolder(item.destination,tr("Unable to move the folder").toStdString());
                return;
            }
        }
    }
    if(doTheDateTransfer)
        if(!writeFileDateTime(item.destination))
        {
            if(!TransferThread::exists(item.destination))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time (not exists): "+item.destination);
            else if(!TransferThread::is_dir(item.destination))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time (not a dir): "+item.destination);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time: "+item.destination);
            /*if(stopIt)
                return;
            waitAction=true;

            emit errorOnFolder(item.source,tr("Unable to set time"));
            return;*/
        }
    if(doRightTransfer && item.actionType!=ActionType_RealMove)
    {
        #ifdef Q_OS_UNIX
        struct stat permissions;
        if(stat(item.source.c_str(), &permissions)!=0)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get the right: "+item.destination);
        else
        {
            if(chmod(item.destination.c_str(), permissions.st_mode)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to chmod the right: "+item.destination);
            if(chown(item.destination.c_str(), permissions.st_uid, permissions.st_gid)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to chown the right: "+item.destination);
        }
        #else
        PSECURITY_DESCRIPTOR PSecurityD;
        PACL dacl;

        HANDLE hFile = CreateFileA(item.source.c_str(), READ_CONTROL | ACCESS_SYSTEM_SECURITY ,
                FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,
                                     std::string("CreateFile() failed. Error: INVALID_HANDLE_VALUE ")+item.source.c_str()+", GetLastError(): "+std::to_string(GetLastError())
                                     );
        else
        {
            DWORD lasterror = GetSecurityInfo(hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                        NULL, NULL, &dacl, NULL, &PSecurityD);
            CloseHandle(hFile);
            if (lasterror != ERROR_SUCCESS)
              ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"GetSecurityInfo() failed. Error"+std::to_string(lasterror));
            else
            {
                CloseHandle(hFile);
                hFile = CreateFileA(item.destination.c_str(),READ_CONTROL | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY ,
                                   0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                  ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"CreateFile() failed. Error: INVALID_HANDLE_VALUE");
                else
                {
                    lasterror = SetSecurityInfo(hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION , NULL, NULL, dacl, NULL);
                    if (lasterror != ERROR_SUCCESS)
                      ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"GetSecurityInfo() failed. Error"+std::to_string(lasterror));
                }
                free(dacl);
                free(PSecurityD);
                CloseHandle(hFile);
            }
        }
        #endif
    }
    if(item.actionType==ActionType_MovePath)
    {
        if(!rmpath(item.source))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the source folder: "+item.destination);
            emit errorOnFolder(item.source,tr("Unable to remove").toStdString());
            return;
        }
    }
    pathList.erase(pathList.cbegin());
    emit firstFolderFinish();
    checkIfCanDoTheNext();
}

void MkPath::internalAddPath(const std::string& source, const std::string& destination, const ActionType &actionType)
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(destination.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
    if(source.find("//") != std::string::npos)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source+", destination: "+destination);
    Item tempPath;
    tempPath.source=source;
    tempPath.destination=destination;
    tempPath.actionType=actionType;
    pathList.push_back(tempPath);
    if(!waitAction)
        checkIfCanDoTheNext();
}

void MkPath::checkIfCanDoTheNext()
{
    if(!waitAction && !stopIt && pathList.size()>0)
        emit internalStartDoThisPath();
}

void MkPath::internalSkip()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    waitAction=false;
    pathList.erase(pathList.cbegin());
    emit firstFolderFinish();
    checkIfCanDoTheNext();
}

void MkPath::internalRetry()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    waitAction=false;
    checkIfCanDoTheNext();
}

void MkPath::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
}

void MkPath::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
}

void MkPath::setMkFullPath(const bool mkFullPath)
{
    this->mkFullPath=mkFullPath;
}

bool MkPath::rmpath(const std::string &dir
                    #ifdef ULTRACOPIER_PLUGIN_RSYNC
                    ,const bool &toSync
                    #endif
                    )
{
    if(!TransferThread::is_dir(dir))
        return true;
    bool allHaveWork=true;
    #ifdef Q_OS_UNIX
    std::vector<TransferThread::dirent_uc> list;
    if(!TransferThread::entryInfoList(dir,list))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"folder list error: "+dir+", errno: "+std::to_string(errno));
        return false;
    }
    for (unsigned int i = 0; i < list.size(); ++i)
    {
        TransferThread::dirent_uc fileInfo=list.at(i);
        if(!fileInfo.isFolder)
        {
            #ifdef ULTRACOPIER_PLUGIN_RSYNC
            if(toSync)
            {
                QFile file(fileInfo.absoluteFilePath());
                if(!file.remove())
                {
                    QFile file(fileInfo.absoluteFilePath());
                    if(!file.remove())
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove a file: "+fileInfo+", due to: "+file.errorString().toStdString());
                        allHaveWork=false;
                    }
                }
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName().toStdString());
                allHaveWork=false;
            }
            #else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+std::string(fileInfo.d_name));
            allHaveWork=false;
            #endif
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(FSabsolutePath(dir)+'/'+fileInfo.d_name+'/'))
                allHaveWork=false;
        }
    }
    #else
    WIN32_FIND_DATAA fdFile;
    HANDLE hFind = NULL;
    char finalpath[MAX_PATH];
    strcpy(finalpath,dir.c_str());
    strcat(finalpath,"\\*");

    allHaveWork=true;
    if((hFind = FindFirstFileA(finalpath, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"folder list error: "+dir+", errno: "+std::to_string(errno));
        return false;
    }

    if(allHaveWork)
    do
    {
        if(strcmp(fdFile.cFileName, ".")!=0 && strcmp(fdFile.cFileName, "..")!=0)
        {
            if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //return the fonction for scan the new folder
                if(!rmpath(dir+'\\'+fdFile.cFileName+'/'))
                    allHaveWork=false;
            }
            else
            {
                #ifdef ULTRACOPIER_PLUGIN_RSYNC
                if(toSync)
                {
                    QFile file(fileInfo.absoluteFilePath());
                    if(!file.remove())
                    {
                        QFile file(fileInfo.absoluteFilePath());
                        if(!file.remove())
                        {
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove a file: "+fileInfo+", due to: "+file.errorString().toStdString());
                            allHaveWork=false;
                        }
                    }
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName().toStdString());
                    allHaveWork=false;
                }
                #else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+std::string(fdFile.cFileName));
                allHaveWork=false;
                #endif
            }
        }
    }
    while(FindNextFileA(hFind, &fdFile));
    FindClose(hFind);
    #endif
    if(!allHaveWork)
        return false;
    allHaveWork=rmdir(FSabsolutePath(dir).c_str())==0;
    if(!allHaveWork)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove the folder: "+FSabsolutePath(dir));
    return allHaveWork;
}

//fonction to edit the file date time
bool MkPath::readFileDateTime(const std::string &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"readFileDateTime("+source+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        struct stat info;
        if(stat(source.c_str(),&info)!=0)
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
                Q_UNUSED(ctime);
                return true;
            #else
                wchar_t filePath[65535];
                if(std::regex_match(source,regRead))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                HANDLE hFileSouce = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"open failed to read: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileSouce);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to get the file time");
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

bool MkPath::writeFileDateTime(const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"writeFileDateTime("+destination+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            return utime(destination.c_str(),&butime)==0;
        #else //mainly for mac
            return utime(destination.c_str(),&butime)==0;
        #endif
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
                HANDLE hFileDestination = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if(hFileDestination == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"open failed to write: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the file time");
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
