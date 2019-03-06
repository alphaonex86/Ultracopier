#include "MkPath.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
    #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
    #endif
#endif

std::string MkPath::text_slash="/";

MkPath::MkPath()
{
    stopIt=false;
    waitAction=false;
    doRightTransfer=false;
    tm t;
    t.tm_gmtoff=0;
    t.tm_hour=0;
    t.tm_isdst=0;
    t.tm_mday=1;
    t.tm_min=0;
    t.tm_mon=1;
    t.tm_sec=0;
    t.tm_year=ULTRACOPIER_PLUGIN_MINIMALYEAR;
    minTime=mktime(&t);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+pathList.front().source+", destination: "+
                             pathList.front().destination+", move: "+std::to_string(pathList.front().actionType));
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(pathList.front().actionType==ActionType_RmSync)
    {
        if(pathList.front().destination.isFile())
        {
            QFile removedFile(pathList.front().destination.absoluteFilePath());
            if(!removedFile.remove())
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the inode: "+pathList.front().destination+", error: "+removedFile.errorString().toStdString());
                emit errorOnFolder(pathList.front().destination,removedFile.errorString().toStdString());
                return;
            }
        }
        else if(!rmpath(pathList.front().destination.absoluteFilePath()))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the inode: "+pathList.front().destination);
            emit errorOnFolder(pathList.front().destination,tr("Unable to remove").toStdString());
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
        const int64_t &sourceLastModified=TransferThread::readFileMDateTime(pathList.front().source);
        if(!TransferThread::exists(pathList.front().source))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources not exists: "+pathList.front().source);
            doTheDateTransfer=false;
        }
        else if((int64_t)minTime>=sourceLastModified)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources is older to copy the time: "+pathList.front().source+
                                     ": "+QDateTime::fromSecsSinceEpoch(minTime).toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString()+
                                     ">="+QDateTime::fromSecsSinceEpoch(sourceLastModified).toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString());
            doTheDateTransfer=false;
        }
        else
        {
            doTheDateTransfer=readFileDateTime(pathList.front().source);
            /*if(!doTheDateTransfer)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get source folder time: "+pathList.front().source.absoluteFilePath());
                emit errorOnFolder(pathList.front().source,tr("Unable to get time"));
                return;
            }*/
        }
    }
    if(TransferThread::is_dir(pathList.front().destination) && pathList.front().actionType==ActionType_RealMove)
        pathList.front().actionType=ActionType_MovePath;
    if(pathList.front().actionType!=ActionType_RealMove)
    {
        if(!TransferThread::is_dir(pathList.front().destination))
            if(!TransferThread::mkpath(pathList.front().destination))
            {
                if(!TransferThread::is_dir(pathList.front().destination))
                {
                    if(stopIt)
                        return;
                    waitAction=true;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+pathList.front().destination);
                    emit errorOnFolder(pathList.front().destination,tr("Unable to create the folder").toStdString());
                    return;
                }
            }
    }
    else
    {
        if(!TransferThread::is_dir(pathList.front().source))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The source folder don't exists: "+pathList.front().source);
            emit errorOnFolder(pathList.front().destination,tr("The source folder don't exists").toStdString());
            return;
        }
        if(!TransferThread::is_dir(pathList.front().source))//it's really an error?
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The source is not a folder: "+pathList.front().source);
            /*if(stopIt)
                return;
            waitAction=true;
            emit errorOnFolder(pathList.front().destination,tr("The source is not a folder"));
            return;*/
        }
        if(stringStartWith(pathList.front().destination,(pathList.front().source+text_slash)))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"move into it self: "+pathList.front().destination);
            int random=rand();
            std::string tempFolder=FSabsolutePath(pathList.front().source)+text_slash+std::to_string(random);
            while(TransferThread::is_dir(tempFolder))
            {
                random=rand();
                tempFolder=FSabsolutePath(pathList.front().source)+text_slash+std::to_string(random);
            }
            if(rename(pathList.front().source.c_str(),tempFolder.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to temporary rename the folder: "+pathList.front().destination);
                emit errorOnFolder(pathList.front().destination,tr("Unable to temporary rename the folder").toStdString());
                return;
            }
            /* http://doc.qt.io/qt-5/qdir.html#rename
             * On most file systems, rename() fails only if oldName does not exist, or if a file with the new name already exists.
            if(!dir.mkpath(FSabsolutePath(pathList.front().destination)))
            {
                if(!dir.exists(FSabsolutePath(pathList.front().destination)))
                {
                    if(stopIt)
                        return;
                    waitAction=true;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+pathList.front().destination.absoluteFilePath());
                    emit errorOnFolder(pathList.front().destination,tr("Unable to create the folder"));
                    return;
                }
            }*/
            if(rename(tempFolder.c_str(),pathList.front().destination.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to do the final real move the folder: "+pathList.front().destination);
                emit errorOnFolder(pathList.front().destination,tr("Unable to do the final real move the folder").toStdString());
                return;
            }
        }
        else
        {
            /* http://doc.qt.io/qt-5/qdir.html#rename
             * On most file systems, rename() fails only if oldName does not exist, or if a file with the new name already exists.
            if(!dir.mkpath(FSabsolutePath(pathList.front().destination)))
            {
                if(!dir.exists(FSabsolutePath(pathList.front().destination)))
                {
                    if(stopIt)
                        return;
                    waitAction=true;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+pathList.front().destination.absoluteFilePath());
                    emit errorOnFolder(pathList.front().destination,tr("Unable to create the folder"));
                    return;
                }
            }*/
            if(rename(pathList.front().source.c_str(),pathList.front().destination.c_str())!=0)
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: from: "+pathList.front().source+", soruce exists: "+
                                         std::to_string(TransferThread::is_dir(pathList.front().source))+", to: "+pathList.front().destination
                                         +", destination exist: "+std::to_string(TransferThread::is_dir(pathList.front().destination))
                                         );
                emit errorOnFolder(pathList.front().destination,tr("Unable to move the folder").toStdString());
                return;
            }
        }
    }
    if(doTheDateTransfer)
        if(!writeFileDateTime(pathList.front().destination))
        {
            if(!TransferThread::exists(pathList.front().destination))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time (not exists): "+pathList.front().destination);
            else if(!TransferThread::is_dir(pathList.front().destination))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time (not a dir): "+pathList.front().destination);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time: "+pathList.front().destination);
            /*if(stopIt)
                return;
            waitAction=true;

            emit errorOnFolder(pathList.front().source,tr("Unable to set time"));
            return;*/
        }
    if(doRightTransfer && pathList.front().actionType!=ActionType_RealMove)
    {
        #ifdef Q_OS_UNIX
        struct stat permissions;
        if(stat(pathList.front().source.c_str(), &permissions)!=0)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get the right: "+pathList.front().destination);
        else
        {
            if(chmod(pathList.front().destination.c_str(), permissions.st_mode)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to chmod the right: "+pathList.front().destination);
            if(chown(pathList.front().destination.c_str(), permissions.st_uid, permissions.st_gid)!=0)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to chown the right: "+pathList.front().destination);
        }
        #else
        this->permissions=source.permissions();
        return true;
        return destination.setPermissions(this->permissions);
        #endif
    }
    if(pathList.front().actionType==ActionType_MovePath)
    {
        if(!rmpath(pathList.front().source))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the source folder: "+pathList.front().destination);
            emit errorOnFolder(pathList.front().source,tr("Unable to remove").toStdString());
            return;
        }
    }
    pathList.erase(pathList.cbegin());
    emit firstFolderFinish();
    checkIfCanDoTheNext();
}

void MkPath::internalAddPath(const std::string& source, const std::string& destination, const ActionType &actionType)
{
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

bool MkPath::rmpath(const std::string &dir
                    #ifdef ULTRACOPIER_PLUGIN_RSYNC
                    ,const bool &toSync
                    #endif
                    )
{
    if(!TransferThread::is_dir(dir))
        return true;
    bool allHaveWork=true;
    std::vector<dirent> list;
    if(!TransferThread::entryInfoList(dir,list))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"folder list error: "+dir+", errno: "+std::to_string(errno));
        return false;
    }
    for (unsigned int i = 0; i < list.size(); ++i)
    {
        std::string fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            #ifdef ULTRACOPIER_PLUGIN_RSYNC
            if(toSync)
            {
                QFile file(fileInfo.absoluteFilePath());
                if(!file.remove())
                {
                    if(toSync)
                    {
                        QFile file(fileInfo.absoluteFilePath());
                        if(!file.remove())
                        {
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove a file: "+fileInfo+", due to: "+file.errorString().toStdString());
                            allHaveWork=false;
                        }
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName().toStdString());
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName().toStdString());
            allHaveWork=false;
            #endif
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(FSabsolutePath(dir)+'/'+fileInfo.fileName()+'/'))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return false;
    allHaveWork=dir.rmdir(FSabsolutePath(dir));
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
            std::string fileInfo(source);
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
