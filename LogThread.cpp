/** \file LogThread.cpp
\brief The thread to do the log but not block the main thread
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "LogThread.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"
#include "cpp11addition.h"

#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif
#include <QMessageBox>

std::string LogThread::text_header_copy="[Copy] ";
std::string LogThread::text_header_move="[Move] ";
std::string LogThread::text_header_skip="[Skip] ";
std::string LogThread::text_header_stop="[Stop] ";
std::string LogThread::text_header_error="[Error] ";
std::string LogThread::text_header_MkPath="[MkPath] ";
std::string LogThread::text_header_RmPath="[RmPath] ";

std::string LogThread::text_var_source="%source%";
std::string LogThread::text_var_size="%size%";
std::string LogThread::text_var_destination="%destination%";
std::string LogThread::text_var_path="%path%";
std::string LogThread::text_var_error="%error%";
std::string LogThread::text_var_mtime="%mtime%";
std::string LogThread::text_var_time="%time%";
std::string LogThread::text_var_timestring="%dd.MM.yyyy h:m:s%";
#ifdef Q_OS_WIN32
std::string LogThread::text_var_computer="%computer%";
std::string LogThread::text_var_user="%user%";
#endif
std::string LogThread::text_var_operation="%operation%";
std::string LogThread::text_var_rmPath="%rmPath%";
std::string LogThread::text_var_mkPath="%mkPath%";

LogThread::LogThread()
{
    sync=false;

    connect(OptionEngine::optionEngine,&OptionEngine::newOptionValue,	this,	&LogThread::newOptionValue);

    enabled=false;

    moveToThread(this);
    start(QThread::IdlePriority);

    connect(this,	&LogThread::newData,		this,&LogThread::realDataWrite,Qt::QueuedConnection);

    newOptionValue("Write_log",	"transfer",			OptionEngine::optionEngine->getOptionValue("Write_log","transfer"));
    newOptionValue("Write_log",	"error",			OptionEngine::optionEngine->getOptionValue("Write_log","error"));
    newOptionValue("Write_log",	"folder",			OptionEngine::optionEngine->getOptionValue("Write_log","folder"));
    newOptionValue("Write_log",	"sync",				OptionEngine::optionEngine->getOptionValue("Write_log","sync"));
    newOptionValue("Write_log",	"transfer_format",	OptionEngine::optionEngine->getOptionValue("Write_log","transfer_format"));
    newOptionValue("Write_log",	"error_format",		OptionEngine::optionEngine->getOptionValue("Write_log","error_format"));
    newOptionValue("Write_log",	"folder_format",	OptionEngine::optionEngine->getOptionValue("Write_log","folder_format"));
    newOptionValue("Write_log",	"sync",				OptionEngine::optionEngine->getOptionValue("Write_log","sync"));
    newOptionValue("Write_log",	"enabled",			OptionEngine::optionEngine->getOptionValue("Write_log","enabled"));
    #ifdef Q_OS_WIN32
    DWORD size=0;
    WCHAR * computerNameW=new WCHAR[size];
    if(GetComputerNameW(computerNameW,&size))
        computer=QString::fromWCharArray(computerNameW,size-1).toStdString();
    else
        computer="Unknown computer";
    delete computerNameW;

    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
        user=QString::fromWCharArray(userNameW,size-1).toStdString();
    else
        user="Unknown user";
    delete userNameW;
    #endif

    #ifdef Q_OS_WIN32
    lineReturn="\r\n";
    #else
    lineReturn="\n";
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
    if(stringtobool(OptionEngine::optionEngine->getOptionValue("Write_log","enabled"))==false)
        return;
    if(log.isOpen())
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Log file already open, error: %1").arg(log.errorString()));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"log file already open, error: "+log.errorString().toStdString());
        return;
    }
    log.setFileName(QString::fromStdString(OptionEngine::optionEngine->getOptionValue("Write_log","file")));
    if(sync)
    {
        if(!log.open(QIODevice::WriteOnly|QIODevice::Unbuffered))
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to open the log file, error: "+log.errorString().toStdString());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"opened log: "+OptionEngine::optionEngine->getOptionValue("Write_log","file"));
    }
    else
    {
        if(!log.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to open the log file, error: %1").arg(log.errorString()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to open the log file, error: "+log.errorString().toStdString());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"opened log: "+OptionEngine::optionEngine->getOptionValue("Write_log","file"));
    }
}

void LogThread::closeLogs()
{
    if(log.isOpen() && data.size()>0)
        log.write(data.data(),data.size());
    log.close();
}

void LogThread::newTransferStart(const Ultracopier::ItemOfCopyList &item)
{
    if(!logTransfer())
        return;
    std::string text;
    if(item.mode==Ultracopier::Copy)
        text=LogThread::text_header_copy+transfer_format+lineReturn;
    else
        text=LogThread::text_header_move+transfer_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    stringreplaceAll(text,LogThread::text_var_source,item.sourceFullPath);
    stringreplaceAll(text,LogThread::text_var_size,std::to_string(item.size));
    stringreplaceAll(text,LogThread::text_var_destination,item.destinationFullPath);
    stringreplaceAll(text,LogThread::text_var_time,QDateTime::currentDateTime().toString(QString::fromStdString(LogThread::text_var_timestring)).toStdString());
    emit newData(text);
}

/** method called when new transfer is started */
void LogThread::transferSkip(const Ultracopier::ItemOfCopyList &item)
{
    if(!logTransfer())
        return;
    std::string text=LogThread::text_header_skip+transfer_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    stringreplaceAll(text,LogThread::text_var_source,item.sourceFullPath);
    stringreplaceAll(text,LogThread::text_var_size,std::to_string(item.size));
    stringreplaceAll(text,LogThread::text_var_destination,item.destinationFullPath);
    stringreplaceAll(text,LogThread::text_var_time,QDateTime::currentDateTime().toString(QString::fromStdString(LogThread::text_var_timestring)).toStdString());
    emit newData(text);
}

void LogThread::newTransferStop(const Ultracopier::ItemOfCopyList &item)
{
    if(!logTransfer())
        return;
    std::string text=LogThread::text_header_stop+transfer_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %source%, %size%, %destination%
    stringreplaceAll(text,LogThread::text_var_source,item.sourceFullPath);
    stringreplaceAll(text,LogThread::text_var_size,std::to_string(item.size));
    stringreplaceAll(text,LogThread::text_var_destination,item.destinationFullPath);
    stringreplaceAll(text,LogThread::text_var_time,QDateTime::currentDateTime().toString(QString::fromStdString(LogThread::text_var_timestring)).toStdString());
    emit newData(text);
}

void LogThread::error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error)
{
    if(!log_enable_error)
        return;
    std::string text=LogThread::text_header_error+error_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %path%, %size%, %mtime%, %error%
    stringreplaceAll(text,LogThread::text_var_path,path);
    stringreplaceAll(text,LogThread::text_var_size,std::to_string(size));
    QDateTime t;
    t.setSecsSinceEpoch(static_cast<unsigned int>(mtime));
    stringreplaceAll(text,LogThread::text_var_mtime,t.toString(Qt::ISODate).toStdString());
    stringreplaceAll(text,LogThread::text_var_error,error);
    stringreplaceAll(text,LogThread::text_var_time,t.toString(QString::fromStdString(LogThread::text_var_timestring)).toStdString());
    emit newData(text);
}

void LogThread::run()
{
    exec();
}

void LogThread::realDataWrite(const std::string &text)
{
    #ifdef ULTRACOPIER_DEBUG
    if(!log.isOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer log not open");
        return;
    }
    #endif // ULTRACOPIER_DEBUG
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(log.write(text.data(),text.size())==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to write into transfer log: "+log.errorString().toStdString());
        return;
    }
    if(sync)
        log.flush();
}

void LogThread::newOptionValue(const std::string &group,const std::string &name,const std::string &value)
{
    if(group!="Write_log")
        return;

    if(name=="transfer_format")
        transfer_format=value;
    else if(name=="error_format")
        error_format=value;
    else if(name=="folder_format")
        folder_format=value;
    else if(name=="sync")
    {
        sync=stringtobool(value);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sync flag is set on: "+value);
        if(sync)
        {
            if(log.isOpen())
                log.flush();
        }
    }
    else if(name=="transfer")
        log_enable_transfer=stringtobool(OptionEngine::optionEngine->getOptionValue("Write_log","enabled")) && stringtobool(value);
    else if(name=="error")
        log_enable_error=stringtobool(OptionEngine::optionEngine->getOptionValue("Write_log","enabled")) && stringtobool(value);
    else if(name=="folder")
        log_enable_folder=stringtobool(OptionEngine::optionEngine->getOptionValue("Write_log","enabled")) && stringtobool(value);
    if(name=="enabled")
    {
        enabled=stringtobool(value);
        if(enabled)
            openLogs();
        else
            closeLogs();
    }
}

std::string LogThread::replaceBaseVar(std::string text)
{
    stringreplaceAll(text,LogThread::text_var_time,QDateTime::currentDateTime().toString(QString::fromStdString(LogThread::text_var_timestring)).toStdString());
    #ifdef Q_OS_WIN32
    stringreplaceAll(text,LogThread::text_var_computer,computer);
    stringreplaceAll(text,LogThread::text_var_user,user);
    #endif
    return text;
}

void LogThread::rmPath(const std::string &path)
{
    if(!logTransfer())
        return;
    std::string text=LogThread::text_header_RmPath+folder_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    stringreplaceAll(text,LogThread::text_var_path,path);
    stringreplaceAll(text,LogThread::text_var_operation,LogThread::text_var_rmPath);
    emit newData(text);
}

void LogThread::mkPath(const std::string &path)
{
    if(!logTransfer())
        return;
    std::string text=LogThread::text_header_MkPath+folder_format+lineReturn;
    text=replaceBaseVar(text);
    //Variable is %operation% %path%
    stringreplaceAll(text,LogThread::text_var_path,path);
    stringreplaceAll(text,LogThread::text_var_operation,LogThread::text_var_mkPath);
    emit newData(text);
}
