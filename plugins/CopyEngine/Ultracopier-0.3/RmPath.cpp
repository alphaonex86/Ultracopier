#include "RmPath.h"

RmPath::RmPath()
{
	stopIt=false;
	waitAction=false;
	setObjectName("RmPath");
	moveToThread(this);
	start();
}

RmPath::~RmPath()
{
	stopIt=true;
	quit();
	wait();
}

void RmPath::addPath(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+path);
	if(stopIt)
		return;
	emit internalStartAddPath(path);
}

void RmPath::skip()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit internalStartSkip();
}

void RmPath::retry()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit internalStartRetry();
}

void RmPath::run()
{
	connect(this,SIGNAL(internalStartAddPath(QString)),this,SLOT(internalAddPath(QString)),Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartDoThisPath()),this,SLOT(internalDoThisPath()),Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartSkip()),this,SLOT(internalSkip()),Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartRetry()),this,SLOT(internalRetry()),Qt::QueuedConnection);
	exec();
}

void RmPath::internalDoThisPath()
{
	if(waitAction || pathList.isEmpty())
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+pathList.first());
	if(!rmpath(pathList.first()))
	{
		if(stopIt)
			return;
		waitAction=true;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to remove the folder: "+pathList.first());
		emit errorOnFolder(pathList.first(),tr("Unable to remove the folder"));
		return;
	}
	pathList.removeFirst();
	emit firstFolderFinish();
	checkIfCanDoTheNext();
}

/** remplace QDir::rmpath() because it return false if the folder not exists
  and seam bug with parent folder */
bool RmPath::rmpath(const QDir &dir)
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
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"found a file: "+fileInfo.fileName());
			allHaveWork=false;
		}
		else
		{
			//return the fonction for scan the new folder
			if(!rmpath(dir.absolutePath()+fileInfo.fileName()+'/'))
				allHaveWork=false;
		}
	}
	if(!allHaveWork)
		return allHaveWork;
	allHaveWork=dir.rmdir(dir.absolutePath());
	if(!allHaveWork)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to remove the folder: "+dir.absolutePath());
	return allHaveWork;
}

void RmPath::internalAddPath(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+path);
	pathList << path;
	if(!waitAction)
		checkIfCanDoTheNext();
}

void RmPath::checkIfCanDoTheNext()
{
	if(!waitAction && !stopIt && pathList.size()>0)
		emit internalStartDoThisPath();
}

void RmPath::internalSkip()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	waitAction=false;
	pathList.removeFirst();
	emit firstFolderFinish();
	checkIfCanDoTheNext();
}

void RmPath::internalRetry()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	waitAction=false;
	checkIfCanDoTheNext();
}

