/** \file LogThread.cpp
\brief The thread to do the log but not block the main thread
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LogThread.h"

#include <QMessageBox>

LogThread::LogThread()
{
	sync=false;
	/** No here because code need direct access

	//load the GUI option
	QString defaultLogFile="";
	if(resources->getWritablePath()!="")
		defaultLogFile=resources->getWritablePath()+"ultracopier-files.log";
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("enabled"),QVariant(false)));
	KeysList.append(qMakePair(QString("file"),QVariant(defaultLogFile)));
	KeysList.append(qMakePair(QString("transfer"),QVariant(true)));
	KeysList.append(qMakePair(QString("error"),QVariant(true)));
	KeysList.append(qMakePair(QString("folder"),QVariant(true)));
	KeysList.append(qMakePair(QString("transfer_format"),QVariant("[%time%] %source% (%size%) %destination%")));
	KeysList.append(qMakePair(QString("error_format"),QVariant("[%time%] %path%, %error%")));
	KeysList.append(qMakePair(QString("folder_format"),QVariant("[%time%] %operation% %path%")));
	options->addOptionGroup("Write_log",KeysList);
	newOptionValue("Write_log",	"transfer",			options->getOptionValue("Write_log","transfer"));
	newOptionValue("Write_log",	"error",			options->getOptionValue("Write_log","error"));
	newOptionValue("Write_log",	"folder",			options->getOptionValue("Write_log","folder"));
	newOptionValue("Write_log",	"sync",				options->getOptionValue("Write_log","sync"));
	*/

	connect(options,SIGNAL(newOptionValue(QString,QString,QVariant)),	this,	SLOT(newOptionValue(QString,QString,QVariant)));

	moveToThread(this);
	start();
}

LogThread::~LogThread()
{
	closeLogs();
	quit();
	wait();
}

void LogThread::openLogs()
{
	if(options->getOptionValue("Write_log","enabled").toBool()==false)
		return;
	log.setFileName(options->getOptionValue("Write_log","file").toString());
	if(sync)
	{
		if(!log.open(QIODevice::WriteOnly|QIODevice::Unbuffered))
			QMessageBox::critical(NULL,tr("Error"),tr("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
	}
	else
	{
		if(!log.open(QIODevice::WriteOnly))
			QMessageBox::critical(NULL,tr("Error"),tr("Unable to open file to keep the log file, error: %1").arg(log.errorString()));
	}
	newOptionValue("Write_log",	"transfer_format",		options->getOptionValue("Write_log","transfer_format"));
	newOptionValue("Write_log",	"error_format",			options->getOptionValue("Write_log","error_format"));
	newOptionValue("Write_log",	"folder_format",		options->getOptionValue("Write_log","folder_format"));
	newOptionValue("Write_log",	"sync",				options->getOptionValue("Write_log","sync"));
	start(QThread::IdlePriority);
}

void LogThread::closeLogs()
{
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
	}
	emit newData();
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
	}
	emit newData();
}

void LogThread::run()
{
	connect(this,SIGNAL(newData()),this,SLOT(realDataWrite()),Qt::QueuedConnection);
	exec();
}

void LogThread::realDataWrite()
{
	#ifdef ULTRACOPIER_DEBUG
	if(!log.isOpen())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"transfer log not open");
		return;
	}
	#endif // ULTRACOPIER_DEBUG
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString currentData;
	{
		QMutexLocker lock_mutex(&mutex);
		currentData=data;
		data.clear();
	}
	if(log.write(currentData.toUtf8())==-1)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to write into transfer log: %1").arg(log.errorString()));
		return;
	}
	if(sync)
	{
		if(log.isOpen())
			log.flush();
	}
}

void LogThread::newOptionValue(QString group,QString name,QVariant value)
{
	if(group!="Write_log")
		return;
	if(name=="transfer_format")
		transfer_format=value.toString();
	if(name=="error_format")
		error_format=value.toString();
	if(name=="folder_format")
		folder_format=value.toString();
	if(name=="sync")
	{
		sync=value.toBool();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("sync flag is set on: %1").arg(sync));
		if(sync)
		{
			if(log.isOpen())
				log.flush();
		}
	}
	if(name=="enabled")
	{
		if(value.toBool())
			openLogs();
		else
			closeLogs();
	}
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
	}
	emit newData();
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
	}
	emit newData();
}
