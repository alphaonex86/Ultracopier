#include "MkPath.h"

MkPath::MkPath()
{
    stopIt=false;
    waitAction=false;
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

void MkPath::addPath(const QFileInfo& source,const QFileInfo& destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    if(stopIt)
        return;
    emit internalStartAddPath(source,destination);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+pathList.first().second.absoluteFilePath());
    if(!dir.exists(pathList.first().second.absoluteFilePath()))
        if(!dir.mkpath(pathList.first().second.absoluteFilePath()))
        {
            if(!dir.exists(pathList.first().second.absoluteFilePath()))
            {
                if(stopIt)
                    return;
                waitAction=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to make the folder: "+pathList.first().second.absoluteFilePath());
                emit errorOnFolder(pathList.first().second,tr("Unable to create the folder"));
                return;
            }
        }
    pathList.removeFirst();
    emit firstFolderFinish();
    checkIfCanDoTheNext();
}

void MkPath::internalAddPath(const QFileInfo& source,const QFileInfo& destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    QPair<QFileInfo,QFileInfo> tempPath;
    tempPath.first=source;
    tempPath.second=destination;
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

