#include "LogThread.h"

#include <QMessageBox>

LogThread::LogThread()
{
}

LogThread::~LogThread()
{
	closeLogs();
}

void LogThread::openLogs()
{
	if(options->getOptionValue("Write_log","enabled").toBool()==false)
		return;
	log.setFileName(options->getOptionValue("Write_log","file").toString());
	if(!log.open(QIODevice::WriteOnly))
		QMessageBox::critical(NULL,tr("Error"),tr("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
	newOptionValue("Write_log",	"transfer_format",		options->getOptionValue("Write_log","transfer_format"));
	newOptionValue("Write_log",	"error_format",			options->getOptionValue("Write_log","error_format"));
	newOptionValue("Write_log",	"folder_format",		options->getOptionValue("Write_log","folder_format"));
	start(QThread::IdlePriority);
}

void LogThread::closeLogs()
{
	quit();
	wait(0);
	if(log.isOpen() && data.size()>0)
		log.write(data.toUtf8());
	log.close();
}

void LogThread::newTransferStart(ItemOfCopyList item)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+item.sourceFullPath);
	QString text=transfer_format+"\n";
	text=replaceBaseVar(text);
	//Variable is %source%, %size%, %destination%
	text=text.replace("%source%",item.sourceFullPath);
	text=text.replace("%size%",QString::number(item.size));
	text=text.replace("%destination%",item.destinationFullPath);
	{
		QMutexLocker lock_mutex(&mutex);
		data+=text;
		if(data.size()>4*1024*1024)
			emit newData();
	}
}

void LogThread::newTransferStop(quint64 id)
{
	Q_UNUSED(id)
}

void LogThread::error(QString path,quint64 size,QDateTime mtime,QString error)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString text=error_format+"\n";
	text=replaceBaseVar(text);
	//Variable is %path%, %size%, %mtime%, %error%
	text=text.replace("%path%",path);
	text=text.replace("%size%",QString::number(size));
	text=text.replace("%mtime%",mtime.toString(Qt::ISODate));
	text=text.replace("%error%",error);
	{
		QMutexLocker lock_mutex(&mutex);
		data+=text;
		if(data.size()>4*1024*1024)
			emit newData();
	}
}

void LogThread::run()
{
	connect(this,SIGNAL(newData()),this,SLOT(realDataWrite()));
	exec();
}

void LogThread::realDataWrite()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString currentData;
	{
		QMutexLocker lock_mutex(&mutex);
		currentData=data;
		data.clear();
	}
	log.write(currentData.toUtf8());
}

void LogThread::newOptionValue(QString group,QString name,QVariant value)
{
	if(group=="Write_log" && name=="transfer_format")
		transfer_format=value.toString();
	if(group=="Write_log" && name=="error_format")
		error_format=value.toString();
	if(group=="Write_log" && name=="folder_format")
		folder_format=value.toString();
}

QString LogThread::replaceBaseVar(QString text)
{
	text=text.replace("%time%",QDateTime::currentDateTime().toString("dd.MM.yyyy h:m:s"));
	return text;
}

void LogThread::rmPath(QString path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString text=folder_format+"\n";
	text=replaceBaseVar(text);
	//Variable is %operation% %path%
	text=text.replace("%path%",path);
	text=text.replace("%operation%","rmPath");
	{
		QMutexLocker lock_mutex(&mutex);
		data+=text;
		if(data.size()>4*1024*1024)
			emit newData();
	}
}

void LogThread::mkPath(QString path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString text=folder_format+"\n";
	text=replaceBaseVar(text);
	//Variable is %operation% %path%
	text=text.replace("%path%",path);
	text=text.replace("%operation%","mkPath");
	{
		QMutexLocker lock_mutex(&mutex);
		data+=text;
		if(data.size()>4*1024*1024)
			emit newData();
	}
}
