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
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif
#include <QMessageBox>

LogThread::LogThread()
{
    sync=false;

    connect(OptionEngine::optionEngine,&OptionEngine::newOptionValue,	this,	&LogThread::newOptionValue);

    enabled=false;

    moveToThread(this);
    start(QThread::IdlePriority);

    connect(this,	&LogThread::newData,		this,&LogThread::realDataWrite,Qt::QueuedConnection);

    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("transfer"),			OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("transfer")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("error"),			OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("error")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("folder"),			OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("folder")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("sync"),				OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("sync")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("transfer_format"),	OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("transfer_format")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("error_format"),		OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("error_format")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("folder_format"),	OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("folder_format")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("sync"),				OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("sync")));
    newOptionValue(QStringLiteral("Write_log"),	QStringLiteral("enabled"),			OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("enabled")));
    #ifdef Q_OS_WIN32
    DWORD size=0;
    WCHAR * computerNameW=new WCHAR[size];
    if(GetComputerNameW(computerNameW,&size))
        computer=QString::fromWCharArray(computerNameW,size-1);
    else
        computer=QStringLiteral("Unknown computer");
    delete computerNameW;

    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
        user=QString::fromWCharArray(userNameW,size-1);
    else
        user=QStringLiteral("Unknown user");
    delete userNameW;
    #endif
}

LogThread::~LogThread()
{
    closeLogs();
    quit();
    wait();
}

bool LogThread::logTransfer() const
{
    return enabled && log_enable_transfer;
}

void LogThread::openLogs()
{
    if(OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("enabled")).toBool()==false)
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
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open the log file, error: %1").arg(log.errorString()));
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"opened log: "+OptionEngine::optionEngine->getOptionValue("Write_log","file").toString());
    }
    else
    {
        if(!log.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open the log file, error: %1").arg(log.errorString()));
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("opened log: ")+OptionEngine::optionEngine->getOptionValue(QStringLiteral("Write_log"),QStringLiteral("file")).toString());
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
        text=QStringLiteral("[Copy] ")+transfer_format+QStringLiteral("\n");
    else
        text=QStringLiteral("[Move] ")+transfer_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace(QStringLiteral("%source%"),item.sourceFullPath);
    text=text.replace(QStringLiteral("%size%"),QString::number(item.size));
    text=text.replace(QStringLiteral("%destination%"),item.destinationFullPath);
    emit newData(text);
}

/** method called when new transfer is started */
void LogThread::transferSkip(const Ultracopier::ItemOfCopyList &item)
{
    if(!log_enable_transfer)
        return;
    QString text=QStringLiteral("[Skip] ")+transfer_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace(QStringLiteral("%source%"),item.sourceFullPath);
    text=text.replace(QStringLiteral("%size%"),QString::number(item.size));
    text=text.replace(QStringLiteral("%destination%"),item.destinationFullPath);
    emit newData(text);
}

void LogThread::newTransferStop(const Ultracopier::ItemOfCopyList &item)
{
    if(!log_enable_transfer)
        return;
    QString text=QStringLiteral("[Stop] ")+transfer_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    text=text.replace(QStringLiteral("%source%"),item.sourceFullPath);
    text=text.replace(QStringLiteral("%size%"),QString::number(item.size));
    text=text.replace(QStringLiteral("%destination%"),item.destinationFullPath);
    emit newData(text);
}

void LogThread::error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error)
{
    if(!log_enable_error)
        return;
    QString text=QStringLiteral("[Error] ")+error_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %path%, %size%, %mtime%, %error%
    text=text.replace(QStringLiteral("%path%"),path);
    text=text.replace(QStringLiteral("%size%"),QString::number(size));
    text=text.replace(QStringLiteral("%mtime%"),mtime.toString(Qt::ISODate));
    text=text.replace(QStringLiteral("%error%"),error);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
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
    if(group!=QStringLiteral("Write_log"))
        return;

    if(name==QStringLiteral("transfer_format"))
        transfer_format=value.toString();
    else if(name==QStringLiteral("error_format"))
        error_format=value.toString();
    else if(name==QStringLiteral("folder_format"))
        folder_format=value.toString();
    else if(name==QStringLiteral("sync"))
    {
        sync=value.toBool();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("sync flag is set on: %1").arg(sync));
        if(sync)
        {
            if(log.isOpen())
                log.flush();
        }
    }
    else if(name==QStringLiteral("transfer"))
        log_enable_transfer=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    else if(name==QStringLiteral("error"))
        log_enable_error=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    else if(name==QStringLiteral("folder"))
        log_enable_folder=OptionEngine::optionEngine->getOptionValue("Write_log","enabled").toBool() && value.toBool();
    if(name==QStringLiteral("enabled"))
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
    text=text.replace(QStringLiteral("%time%"),QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy h:m:s")));
    #ifdef Q_OS_WIN32
    text=text.replace(QStringLiteral("%computer%"),computer);
    text=text.replace(QStringLiteral("%user%"),user);
    #endif
    return text;
}

void LogThread::rmPath(const QString &path)
{
    if(!log_enable_folder)
        return;
    QString text=QStringLiteral("[RmPath] ")+folder_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    text=text.replace(QStringLiteral("%path%"),path);
    text=text.replace(QStringLiteral("%operation%"),QStringLiteral("rmPath"));
    emit newData(text);
}

void LogThread::mkPath(const QString &path)
{
    if(!log_enable_folder)
        return;
    QString text=QStringLiteral("[MkPath] ")+folder_format+QStringLiteral("\n");
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    text=text.replace(QStringLiteral("%path%"),path);
    text=text.replace(QStringLiteral("%operation%"),QStringLiteral("mkPath"));
    emit newData(text);
}
