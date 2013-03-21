/** \file LogThread.cpp
\brief The thread to do the log but not block the main thread
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LogThread.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif
#include <QMessageBox>

LogThread::LogThread()
{
    sync=false;

    //load the GUI option
    QString defaultLogFile="";
    if(ResourcesManager::resourcesManager->getWritablePath()!="")
        defaultLogFile=ResourcesManager::resourcesManager->getWritablePath()+"ultracopier-files.log";
    QList<QPair<QString, QVariant> > KeysList;
    KeysList.append(qMakePair(QString("enabled"),QVariant(false)));
    KeysList.append(qMakePair(QString("file"),QVariant(defaultLogFile)));
    KeysList.append(qMakePair(QString("transfer"),QVariant(true)));
    KeysList.append(qMakePair(QString("error"),QVariant(true)));
    KeysList.append(qMakePair(QString("folder"),QVariant(true)));
    KeysList.append(qMakePair(QString("sync"),QVariant(true)));
    KeysList.append(qMakePair(QString("transfer_format"),QVariant("[%time%] %source% (%size%) %destination%")));
    KeysList.append(qMakePair(QString("error_format"),QVariant("[%time%] %path%, %error%")));
    KeysList.append(qMakePair(QString("folder_format"),QVariant("[%time%] %operation% %path%")));
    OptionEngine::optionEngine->addOptionGroup("Write_log",KeysList);

    connect(OptionEngine::optionEngine,&OptionEngine::newOptionValue,	this,	&LogThread::newOptionValue);

    enabled=false;

    moveToThread(this);
    start(QThread::IdlePriority);

    connect(this,	&LogThread::newData,		this,&LogThread::realDataWrite,Qt::QueuedConnection);

    newOptionValue("Write_log",	"transfer",			OptionEngine::optionEngine->getOptionValue("Write_log","transfer"));
    newOptionValue("Write_log",	"error",			OptionEngine::optionEngine->getOptionValue("Write_log","error"));
    newOptionValue("Write_log",	"folder",			OptionEngine::optionEngine->getOptionValue("Write_log","folder"));
    newOptionValue("Write_log",	"sync",				OptionEngine::optionEngine->getOptionValue("Write_log","sync"));
    newOptionValue("Write_log",	"transfer_format",		OptionEngine::optionEngine->getOptionValue("Write_log","transfer_format"));
    newOptionValue("Write_log",	"error_format",			OptionEngine::optionEngine->getOptionValue("Write_log","error_format"));
    newOptionValue("Write_log",	"folder_format",		OptionEngine::optionEngine->getOptionValue("Write_log","folder_format"));
    newOptionValue("Write_log",	"sync",				OptionEngine::optionEngine->getOptionValue("Write_log","sync"));
    newOptionValue("Write_log",	"enabled",			OptionEngine::optionEngine->getOptionValue("Write_log","enabled"));
    #ifdef Q_OS_WIN32
    DWORD size=0;
    WCHAR * computerNameW=new WCHAR[size];
    if(GetComputerNameW(computerNameW,&size))
        computer=QString::fromWCharArray(computerNameW,size);
    else
        computer="Unknown computer";
    delete computerNameW;

    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
        user=QString::fromWCharArray(userNameW,size);
    else
        user="Unknown user";
    delete userNameW;
    #endif
}

LogThread::~LogThread()
{
    closeLogs();
    quit();
    wait();
}

bool LogThread::logTransfer()
{
    return enabled && log_enable_transfer;
}

void LogThread::openLogs()
{
    if(OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool()==false)
        return;
    if(log.isOpen())
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Log file already open, error: %1").arg(log.errorString()));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("log file already open, error: %1").arg(log.errorString()));
        return;
    }
    log.setFileName(OptionEngine::optionEngine->getOptionValue("Write_log","file").toString());
    if(sync)
    {
        if(!log.open(QIODevice::WriteOnly|QIODevice::Unbuffered))
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"opened log: "+OptionEngine::optionEngine->getOptionValue("Write_log","file").toString());
    }
    else
    {
        if(!log.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"opened log: "+OptionEngine::optionEngine->getOptionValue("Write_log","file").toString());
    }
}

void LogThread::closeLogs()
{
    if(log.isOpen() && data.size()>0)
        log.write(data.toUtf8());
    log.close();
}

void LogThread::newTransferStart(const Ultracopier::ItemOfCopyList &item)
{
    if(!log_enable_transfer)
        return;
    QString text;
    if(item.mode==Ultracopier::Copy)
        text="[Copy] "+transfer_format+"\n";
    else
        text="[Move] "+transfer_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace("%source%",item.sourceFullPath);
    text=text.replace("%size%",QString::number(item.size));
    text=text.replace("%destination%",item.destinationFullPath);
    emit newData(text);
}

/** method called when new transfer is started */
void LogThread::transferSkip(const Ultracopier::ItemOfCopyList &item)
{
    if(!log_enable_transfer)
        return;
    QString text="[Skip] "+transfer_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace("%source%",item.sourceFullPath);
    text=text.replace("%size%",QString::number(item.size));
    text=text.replace("%destination%",item.destinationFullPath);
    emit newData(text);
}

void LogThread::newTransferStop(const Ultracopier::ItemOfCopyList &item)
{
    if(!log_enable_transfer)
        return;
    QString text="[Stop] "+transfer_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace("%source%",item.sourceFullPath);
    text=text.replace("%size%",QString::number(item.size));
    text=text.replace("%destination%",item.destinationFullPath);
    emit newData(text);
}

void LogThread::error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error)
{
    if(!log_enable_error)
        return;
    QString text="[Error] "+error_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %path%, %size%, %mtime%, %error%
    text=text.replace("%path%",path);
    text=text.replace("%size%",QString::number(size));
    text=text.replace("%mtime%",mtime.toString(Qt::ISODate));
    text=text.replace("%error%",error);
    emit newData(text);
}

void LogThread::run()
{
    exec();
}

void LogThread::realDataWrite(const QString &text)
{
    #ifdef ULTRACOPIER_DEBUG
    if(!log.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer log not open");
        return;
    }
    #endif // ULTRACOPIER_DEBUG
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(log.write(text.toUtf8())==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unable to write into transfer log: %1").arg(log.errorString()));
        return;
    }
    if(sync)
        log.flush();
}

void LogThread::newOptionValue(const QString &group,const QString &name,const QVariant &value)
{
    if(group!="Write_log")
        return;

    if(name=="transfer_format")
        transfer_format=value.toString();
    else if(name=="error_format")
        error_format=value.toString();
    else if(name=="folder_format")
        folder_format=value.toString();
    else if(name=="sync")
    {
        sync=value.toBool();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("sync flag is set on: %1").arg(sync));
        if(sync)
        {
            if(log.isOpen())
                log.flush();
        }
    }
    else if(name=="transfer")
        log_enable_transfer=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    else if(name=="error")
        log_enable_error=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    else if(name=="folder")
        log_enable_folder=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    if(name=="enabled")
    {
        enabled=value.toBool();
        if(enabled)
            openLogs();
        else
            closeLogs();
    }
}

QString LogThread::replaceBaseVar(QString text)
{
    text=text.replace("%time%",QDateTime::currentDateTime().toString("dd.MM.yyyy h:m:s"));
    #ifdef Q_OS_WIN32
    text=text.replace("%computer%",computer);
    text=text.replace("%user%",user);
    #endif
    return text;
}

void LogThread::rmPath(const QString &path)
{
    if(!log_enable_folder)
        return;
    QString text="[RmPath] "+folder_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    text=text.replace("%path%",path);
    text=text.replace("%operation%","rmPath");
    emit newData(text);
}

void LogThread::mkPath(const QString &path)
{
    if(!log_enable_folder)
        return;
    QString text="[MkPath] "+folder_format+"\n";
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    text=text.replace("%path%",path);
    text=text.replace("%operation%","mkPath");
    emit newData(text);
}
