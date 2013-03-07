#include "MkPath.h"

#ifndef Q_OS_UNIX
    #ifdef Q_OS_WIN32
        #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            #include <windows.h>
        #endif
    #endif
#endif

MkPath::MkPath()
{
    stopIt=false;
    waitAction=false;
    doRightTransfer=false;
    maxTime=QDateTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
    setObjectName("MkPath");
    moveToThread(this);
    start();
}

MkPath::~MkPath()
{
    stopIt=true;
    quit();
    wait();
}

void MkPath::addPath(const QFileInfo& source, const QFileInfo& destination, const bool &move)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    if(stopIt)
        return;
    emit internalStartAddPath(source,destination,move);
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
    connect(this,&MkPath::internalStartAddPath,	this,&MkPath::internalAddPath,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartDoThisPath,	this,&MkPath::internalDoThisPath,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartSkip,	this,&MkPath::internalSkip,Qt::QueuedConnection);
    connect(this,&MkPath::internalStartRetry,	this,&MkPath::internalRetry,Qt::QueuedConnection);
    exec();
}

void MkPath::internalDoThisPath()
{
    if(waitAction || pathList.isEmpty())
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2, move: %3").arg(pathList.first().source.absoluteFilePath()).arg(pathList.first().destination.absoluteFilePath()).arg(pathList.first().move));
    doTheDateTransfer=false;
    if(keepDate)
    {
        doTheDateTransfer=readFileDateTime(pathList.first().source);
        if(!doTheDateTransfer)
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get source folder time: "+pathList.first().destination.absoluteFilePath());
            emit errorOnFolder(pathList.first().source,tr("Unable to get time"));
            return;
        }
    }
    if(!dir.exists(pathList.first().destination.absoluteFilePath()))
        if(!dir.mkpath(pathList.first().destination.absoluteFilePath()))
        {
            if(!dir.exists(pathList.first().destination.absoluteFilePath()))
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+pathList.first().destination.absoluteFilePath());
                emit errorOnFolder(pathList.first().destination,tr("Unable to create the folder"));
                return;
            }
        }
    if(doTheDateTransfer)
        if(!writeFileDateTime(pathList.first().destination))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set destination folder time: "+pathList.first().destination.absoluteFilePath());
            emit errorOnFolder(pathList.first().source,tr("Unable to set time"));
            return;
        }
    if(doRightTransfer)
    {
        QFile source(pathList.first().source.absoluteFilePath());
        QFile destination(pathList.first().destination.absoluteFilePath());
        if(!destination.setPermissions(source.permissions()))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to set the right: "+pathList.first().destination.absoluteFilePath());
            emit errorOnFolder(pathList.first().source,tr("Unable to set the right"));
            return;
        }
    }
    if(pathList.first().move)
    {
        if(!rmpath(pathList.first().source.absoluteFilePath()))
        {
            if(stopIt)
                return;
            waitAction=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to remove the source folder: "+pathList.first().destination.absoluteFilePath());
            emit errorOnFolder(pathList.first().source,tr("Unable to remove"));
            return;
        }
    }
    pathList.removeFirst();
    emit firstFolderFinish();
    checkIfCanDoTheNext();
}

void MkPath::internalAddPath(const QFileInfo& source, const QFileInfo& destination, const bool &move)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    Item tempPath;
    tempPath.source=source;
    tempPath.destination=destination;
    tempPath.move=move;
    pathList << tempPath;
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
    pathList.removeFirst();
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

bool MkPath::rmpath(const QDir &dir)
{
    if(!dir.exists())
        return true;
    bool allHaveWork=true;
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName());
            allHaveWork=false;
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(dir.absolutePath()+'/'+fileInfo.fileName()+'/'))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return false;
    allHaveWork=dir.rmdir(dir.absolutePath());
    if(!allHaveWork)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove the folder: "+dir.absolutePath());
    return allHaveWork;
}

//fonction to edit the file date time
bool MkPath::readFileDateTime(const QFileInfo &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"readFileDateTime("+source.absoluteFilePath()+")");
    if(maxTime>=source.lastModified())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources is older to copy the time: "+source.absoluteFilePath()+": "+source.lastModified().toString());
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
                filePath[source.absoluteFilePath().toWCharArray(filePath)]=L'\0';
                HANDLE hFileSouce = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"open failed to read: "+QString::fromWCharArray(filePath)+", error: "+QString::number(GetLastError()));
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

bool MkPath::writeFileDateTime(const QFileInfo &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"writeFileDateTime("+destination.absoluteFilePath()+")");
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"open failed to write: "+QString::fromWCharArray(filePath)+", error: "+QString::number(GetLastError()));
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
