/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QFileDialog>
#include <QMessageBox>

#include "copyEngine.h"

copyEngine::copyEngine() :
	ui(new Ui::options())
{
	tempWidget			= NULL;
	sourceDrive			= "";
	sourceDriveMultiple		= false;
	uiIsInstalled			= false;
	destinationDrive		= "";
	destinationDriveMultiple	= false;
	interface			= NULL;
	transferIsStarted_internalVar	= false;
	bytesToTransfer			= 0;
	bytesTransfered			= 0;
	maxSpeed			= 0;
	stopIt				= false;
	alwaysDoThisActionForFileExists	= FileExists_NotSet;
	alwaysDoThisActionForFileError	= FileError_NotSet;
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	threadOfTheTransfer.setObjectName("Read thread");
	#endif // ULTRACOPIER_DEBUG
	qRegisterMetaType<DebugLevel>("DebugLevel");
	qRegisterMetaType<ItemOfCopyList>("ItemOfCopyList");
	qRegisterMetaType<QFileInfo>("QFileInfo");
	connect(&threadOfTheTransfer,SIGNAL(transferIsStarted(bool)),					this,SLOT(transferIsStarted(bool)));
	connect(&threadOfTheTransfer,SIGNAL(transferIsInPause(bool)),					this,SIGNAL(isInPause(bool)));
	connect(&threadOfTheTransfer,SIGNAL(newTransferStart(ItemOfCopyList)),				this,SIGNAL(newTransferStart(ItemOfCopyList)));
	connect(&threadOfTheTransfer,SIGNAL(newTransferStop(quint64)),					this,SIGNAL(newTransferStop(quint64)));
	connect(&threadOfTheTransfer,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(&threadOfTheTransfer,SIGNAL(newActionOnList()),						this,SIGNAL(newActionOnList()));
	connect(&threadOfTheTransfer,SIGNAL(fileAlreadyExists(QFileInfo,QFileInfo,bool)),		this,SLOT(fileAlreadyExists(QFileInfo,QFileInfo,bool)),Qt::QueuedConnection);
	connect(&threadOfTheTransfer,SIGNAL(errorOnFile(QFileInfo,QString,bool)),			this,SLOT(errorOnFile(QFileInfo,QString,bool)),Qt::QueuedConnection);
	connect(&threadOfTheTransfer,SIGNAL(errorOnFileInWriteThread(QFileInfo,QString,int)),		this,SLOT(errorOnFileInWriteThread(QFileInfo,QString,int)),Qt::QueuedConnection);
	connect(&threadOfTheTransfer,SIGNAL(cancelAll()),						this,SIGNAL(cancelAll()));
	connect(&threadOfTheTransfer,SIGNAL(rmPath(QString)),						this,SIGNAL(rmPath(QString)));
	connect(&threadOfTheTransfer,SIGNAL(mkPath(QString)),						this,SIGNAL(mkPath(QString)));
	#if ! defined (Q_CC_GNU)
	ui->keepDate->setEnabled(false);
	ui->keepDate->setToolTip("Not supported with this compiler");
	#endif
}

copyEngine::~copyEngine()
{
	stopIt=true;
	fileErrorDialogObject.close();
	fileExistsDialogObject.close();
	fileIsSameDialogObject.close();
	folderExistsDialogObject.close();
}

//set the copy info and options before runing
void copyEngine::setRightTransfer(const bool doRightTransfer)
{
	this->doRightTransfer=doRightTransfer;
	if(uiIsInstalled)
		ui->doRightTransfer->setChecked(doRightTransfer);
	threadOfTheTransfer.setRightTransfer(doRightTransfer);
}

//set keep date
void copyEngine::setKeepDate(const bool keepDate)
{
	this->keepDate=keepDate;
	if(uiIsInstalled)
		ui->keepDate->setChecked(keepDate);
	threadOfTheTransfer.setKeepDate(keepDate);
}

//set the current max speed in KB/s
void copyEngine::setMaxSpeed(int maxSpeed)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"maxSpeed: "+QString::number(maxSpeed));
	this->maxSpeed=maxSpeed;
	threadOfTheTransfer.setMaxSpeed(maxSpeed);
}

//set prealoc file size
void copyEngine::setPreallocateFileSize(const bool prealloc)
{
	this->prealloc=prealloc;
	if(uiIsInstalled)
		ui->prealloc->setChecked(prealloc);
	threadOfTheTransfer.setPreallocateFileSize(prealloc);
}

//set block size in KB
void copyEngine::setBlockSize(const int blockSize)
{
	this->blockSize=blockSize;
	if(uiIsInstalled)
		ui->blockSize->setValue(blockSize);
	threadOfTheTransfer.setBlockSize(blockSize);
}

//set auto start
void copyEngine::setAutoStart(const bool autoStart)
{
	this->autoStart=autoStart;
	if(uiIsInstalled)
		ui->autoStart->setChecked(autoStart);
}

//set check destination folder
void copyEngine::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
	this->checkDestinationFolderExists=checkDestinationFolderExists;
	if(uiIsInstalled)
		ui->checkBoxDestinationFolderExists->setChecked(checkDestinationFolderExists);
	for(int i=0;i<scanFileOrFolderThreadsPool.size();i++)
		scanFileOrFolderThreadsPool.at(i).thread->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void copyEngine::folderTransfer(QString source,QString destination,int numberOfItem)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"source: "+source+", destination: "+destination+", numberOfItem: "+QString::number(numberOfItem));
	QObject * senderThread = sender();
	if(senderThread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer null (plugin copy engine)");
		return;
	}
	int index=0;
	while(index<scanFileOrFolderThreadsPool.size())
	{
		if(senderThread==scanFileOrFolderThreadsPool.at(index).thread)
		{
			if(scanFileOrFolderThreadsPool.at(index).mode==Move)
				threadOfTheTransfer.addToRmPath(source,numberOfItem);
			emit newFolderListing(source);
			threadOfTheTransfer.addToMkPath(destination);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer not found (plugin copy engine)");
}

void copyEngine::fileTransfer(QFileInfo sourceFileInfo,QFileInfo destinationFileInfo)
{
	QObject * senderThread = sender();
	if(senderThread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer null (plugin copy engine)");
		return;
	}
	int index=0;
	while(index<scanFileOrFolderThreadsPool.size())
	{
		if(senderThread==scanFileOrFolderThreadsPool.at(index).thread)
		{
			threadOfTheTransfer.addToTransfer(sourceFileInfo,destinationFileInfo,scanFileOrFolderThreadsPool.at(index).mode);
			emit newActionOnList();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer not found (plugin copy engine)");
}

void copyEngine::transferIsStarted(bool value)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"transferIsStarted: "+QString::number(value));
	transferIsStarted_internalVar=value;
	updateTheStatus();
}

//to get engine custom pannel
bool copyEngine::getOptionsEngine(QWidget *tempWidget)
{
	this->tempWidget=tempWidget;
	connect(tempWidget,		SIGNAL(destroyed()),		this,			SLOT(resetTempWidget()));
	ui->setupUi(tempWidget);
	//conect the ui widget
	connect(ui->doRightTransfer,			SIGNAL(toggled(bool)),		&threadOfTheTransfer,	SLOT(setRightTransfer(bool)));
	connect(ui->keepDate,				SIGNAL(toggled(bool)),		&threadOfTheTransfer,	SLOT(setKeepDate(bool)));
	connect(ui->prealloc,				SIGNAL(toggled(bool)),		&threadOfTheTransfer,	SLOT(setPreallocateFileSize(bool)));
	connect(ui->blockSize,				SIGNAL(valueChanged(int)),	&threadOfTheTransfer,	SLOT(setBlockSize(int)));
	connect(ui->checkBoxDestinationFolderExists,	SIGNAL(toggled(bool)),		this,			SLOT(setCheckDestinationFolderExists(bool)));
	uiIsInstalled=true;
	setRightTransfer(doRightTransfer);
	setKeepDate(keepDate);
	setMaxSpeed(maxSpeed);
	setPreallocateFileSize(prealloc);
	setBlockSize(blockSize);
	setAutoStart(autoStart);
	setCheckDestinationFolderExists(checkDestinationFolderExists);
	return true;
}

bool copyEngine::haveSameSource(const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(sourceDriveMultiple)
		return false;
	if(sourceDrive.isEmpty())
		return true;
	int index=0;
	while(index<sources.size())
	{
		if(threadOfTheTransfer.getDrive(sources.at(index))!=sourceDrive)
			return false;
		index++;
	}
	return true;
}

bool copyEngine::haveSameDestination(const QString &destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(destinationDriveMultiple)
		return false;
	if(destinationDrive.isEmpty())
		return true;
	int index=0;
	while(index<destination.size())
	{
		if(threadOfTheTransfer.getDrive(destination.at(index))!=destinationDrive)
			return false;
		index++;
	}
	return true;
}

void copyEngine::setInterfacePointer(QWidget * interface)
{
	this->interface=interface;
	fileErrorDialogObject.setParent(interface);
	fileExistsDialogObject.setParent(interface);
	fileIsSameDialogObject.setParent(interface);
	folderExistsDialogObject.setParent(interface);
}

//user ask ask to add folder (add it with interface ask source/destination)
bool copyEngine::userAddFolder(const CopyMode &mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString source = QFileDialog::getExistingDirectory(interface,tr("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(source.isEmpty() || source.isNull() || source=="")
		return false;
	if(mode==Copy)
		return newCopy(QStringList() << source);
	else
		return newMove(QStringList() << source);
}

bool copyEngine::userAddFile(const CopyMode &mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QStringList sources = QFileDialog::getOpenFileNames(
		interface,
		tr("Select one or more files to open"),
		"",
		tr("All files")+" (*)");
	if(sources.isEmpty())
		return false;
	if(mode==Copy)
		return newCopy(sources);
	else
		return newMove(sources);
}

scanFileOrFolder * copyEngine::newScanThread(CopyMode mode)
{
	scanFileOrFolderThread newThread;
	newThread.mode=mode;
	newThread.thread=new scanFileOrFolder(this);
	connect(newThread.thread,SIGNAL(finished()),this,SLOT(scanThreadHaveFinish()));
	connect(newThread.thread,SIGNAL(started()),this,SLOT(updateTheStatus()));
	connect(newThread.thread,SIGNAL(finished()),this,SLOT(updateTheStatus()));
	connect(newThread.thread,SIGNAL(folderTransfer(QString,QString,int)),this,SLOT(folderTransfer(QString,QString,int)));
	connect(newThread.thread,SIGNAL(fileTransfer(QFileInfo,QFileInfo)),this,SLOT(fileTransfer(QFileInfo,QFileInfo)));
	connect(newThread.thread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(newThread.thread,SIGNAL(errorOnFolder(QFileInfo,QString)),this,SLOT(errorOnFolder(QFileInfo,QString)));
	connect(newThread.thread,SIGNAL(folderAlreadyExists(QFileInfo,bool,QFileInfo)),this,SLOT(folderAlreadyExists(QFileInfo,bool,QFileInfo)));
	scanFileOrFolderThreadsPool << newThread;
	newThread.thread->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
	updateTheStatus();
	return newThread.thread;
}

void copyEngine::scanThreadHaveFinish(bool skipFirstRemove)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listing thread have finish, skipFirstRemove: "+QString::number(skipFirstRemove));
	if(!skipFirstRemove)
	{
		QObject * senderThread = sender();
		if(senderThread==NULL)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer null (plugin copy engine)");
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
			bool isFound=false;
			int index=0;
			while(index<scanFileOrFolderThreadsPool.size())
			{
				if(senderThread==scanFileOrFolderThreadsPool.at(index).thread)
				{
					if(index!=0)
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"scanFileOrFolderThread is not the first (plugin copy engine)");
					scanFileOrFolderThreadsPool.removeAt(index);
					isFound=true;
					break;
				}
				index++;
			}
			if(!isFound)
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer not found (plugin copy engine)");
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
	if(scanFileOrFolderThreadsPool.size()>0)
	{
		//then start the next listing threads
		if(scanFileOrFolderThreadsPool.first().thread->isFinished())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Start listing thread");
			scanFileOrFolderThreadsPool.first().thread->start();
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"The listing thread is already running");
	}
	else
	{
		if(autoStart)
		{
			if(threadOfTheTransfer.isFinished())
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"start the transfer thread");
				threadOfTheTransfer.start();
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"transfer thread seam be already lunched");
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Put the copy engine in pause");
			transferIsStarted_internalVar=true;
			updateTheStatus();
			emit isInPause(true);
		}
	}
}

//external soft like file browser have send copy/move list to do
bool copyEngine::newCopy(const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString destination = QFileDialog::getExistingDirectory(interface,tr("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(destination.isEmpty() || destination.isNull() || destination=="")
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Canceled by the user");
		return false;
	}
	return newCopy(sources,destination);
}

bool copyEngine::newCopy(const QStringList &sources,const QString &destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	scanFileOrFolder * scanFileOrFolderThread = newScanThread(Copy);
	if(scanFileOrFolderThread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to get new thread");
		return false;
	}
	scanFileOrFolderThread->addToList(sources,destination);
	scanThreadHaveFinish(true);
	int index=0;
	while(index<sources.size() && !sourceDriveMultiple)
	{
		QString tempDrive=threadOfTheTransfer.getDrive(sources.at(index));
		if(sourceDrive=="")
			sourceDrive=tempDrive;
		if(tempDrive!=sourceDrive)
			sourceDriveMultiple=true;
		index++;
	}
	QString tempDrive=threadOfTheTransfer.getDrive(destination);
	if(!destinationDriveMultiple)
	{
		if(tempDrive=="")
			destinationDrive=tempDrive;
		if(tempDrive!=destinationDrive)
			destinationDriveMultiple=true;
	}
	return true;
}

bool copyEngine::newMove(const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString destination = QFileDialog::getExistingDirectory(interface,tr("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(destination.isEmpty() || destination.isNull() || destination=="")
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Canceled by the user");
		return false;
	}
	return newMove(sources,destination);
}

bool copyEngine::newMove(const QStringList &sources,const QString &destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	scanFileOrFolder * scanFileOrFolderThread = newScanThread(Move);
	if(scanFileOrFolderThread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to get new thread");
		return false;
	}
	scanFileOrFolderThread->addToList(sources,destination);
	scanThreadHaveFinish(true);
	int index=0;
	while(index<sources.size() && !sourceDriveMultiple)
	{
		QString tempDrive=threadOfTheTransfer.getDrive(sources.at(index));
		if(sourceDrive=="")
			sourceDrive=tempDrive;
		if(tempDrive!=sourceDrive)
			sourceDriveMultiple=true;
		index++;
	}
	QString tempDrive=threadOfTheTransfer.getDrive(destination);
	if(!destinationDriveMultiple)
	{
		if(tempDrive=="")
			destinationDrive=tempDrive;
		if(tempDrive!=destinationDrive)
			destinationDriveMultiple=true;
	}
	return true;
}

void copyEngine::setDrive(const QStringList &drives)
{
	threadOfTheTransfer.setDrive(drives);
}

//action on the copy
/*void copyEngine::start()
{
	if(threadOfTheTransfer.isFinished())
		threadOfTheTransfer.start();
}*/

void copyEngine::pause()
{
	threadOfTheTransfer.pauseTransfer();
}

void copyEngine::resume()
{
	if(threadOfTheTransfer.isFinished())
		threadOfTheTransfer.start();
	else
		threadOfTheTransfer.resumeTransfer();
}

void copyEngine::skip(const quint64 &id)
{
	threadOfTheTransfer.skipCurrentTransfer(id);
}

void copyEngine::cancel()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopIt=true;
	int index=0;
	while(index<scanFileOrFolderThreadsPool.size())
	{
		scanFileOrFolderThreadsPool.at(index).thread->stop();
		index++;
	}
	threadOfTheTransfer.stopTheTransfer();
}

//first = current transfered byte, second = byte to transfer
QPair<quint64,quint64> copyEngine::getGeneralProgression()
{
	return threadOfTheTransfer.getGeneralProgression();
}

//first = current transfered byte, second = byte to transfer
returnSpecificFileProgression copyEngine::getFileProgression(const quint64 &id)
{
	return threadOfTheTransfer.getFileProgression(id);
}

//edit the transfer list
/*bool copyEngine::setActionOnList(ActionOnCopyList action)
{
	threadOfTheTransfer.setActionOnList(action);
	return true;
}*/

void copyEngine::removeItems(const QList<int> &ids)
{
	threadOfTheTransfer.removeItems(ids);
}

void copyEngine::moveItemsOnTop(const QList<int> &ids)
{
	threadOfTheTransfer.moveItemsOnTop(ids);
}

void copyEngine::moveItemsUp(const QList<int> &ids)
{
	threadOfTheTransfer.moveItemsUp(ids);
}

void copyEngine::moveItemsDown(const QList<int> &ids)
{
	threadOfTheTransfer.moveItemsDown(ids);
}

void copyEngine::moveItemsOnBottom(const QList<int> &ids)
{
	threadOfTheTransfer.moveItemsOnBottom(ids);
}

/// \brief export the transfer list into a file
void copyEngine::exportTransferList()
{
}

/// \brief import the transfer list into a file
void copyEngine::importTransferList()
{
}

QList<returnActionOnCopyList> copyEngine::getActionOnList()
{
	return threadOfTheTransfer.getActionOnList();
}

//speed limitation
qint64 copyEngine::getSpeedLimitation()
{
	return maxSpeed;
}

bool copyEngine::setSpeedLimitation(const qint64 &speedLimitation)
{
	maxSpeed=speedLimitation;
	threadOfTheTransfer.setMaxSpeed(speedLimitation);
	return true;
}

QList<QPair<QString,QString> > copyEngine::getCollisionAction()
{
	QPair<QString,QString> tempItem;
	QList<QPair<QString,QString> > list;
	tempItem.first=tr("Ask");tempItem.second="ask";list << tempItem;
	tempItem.first=tr("Skip");tempItem.second="skip";list << tempItem;
	tempItem.first=tr("Overwrite");tempItem.second="overwrite";list << tempItem;
	tempItem.first=tr("Overwrite if newer");tempItem.second="overwriteIfNewer";list << tempItem;
	tempItem.first=tr("Overwrite if not same modification date");tempItem.second="overwriteIfNotSameModificationDate";list << tempItem;
	tempItem.first=tr("Rename");tempItem.second="rename";list << tempItem;
	return list;
}

QList<QPair<QString,QString> > copyEngine::getErrorAction()
{
	QPair<QString,QString> tempItem;
	QList<QPair<QString,QString> > list;
	tempItem.first=tr("Ask");tempItem.second="ask";list << tempItem;
	tempItem.first=tr("Skip");tempItem.second="skip";list << tempItem;
	tempItem.first=tr("Put to end of the list");tempItem.second="putToEndOfTheList";list << tempItem;
	return list;
}

void copyEngine::setCollisionAction(const QString &action)
{
	if(action=="skip")
		alwaysDoThisActionForFileExists=FileExists_Skip;
	else if(action=="overwrite")
		alwaysDoThisActionForFileExists=FileExists_Overwrite;
	else if(action=="overwriteIfNewer")
		alwaysDoThisActionForFileExists=FileExists_OverwriteIfNewer;
	else if(action=="overwriteIfNotSameModificationDate")
		alwaysDoThisActionForFileExists=FileExists_OverwriteIfNotSameModificationDate;
	else if(action=="rename")
		alwaysDoThisActionForFileExists=FileExists_Rename;
	else
		alwaysDoThisActionForFileExists=FileExists_NotSet;
}

void copyEngine::setErrorAction(const QString &action)
{
	if(action=="skip")
		alwaysDoThisActionForFileError=FileError_Skip;
	else if(action=="putToEndOfTheList")
		alwaysDoThisActionForFileError=FileError_PutToEndOfTheList;
	else
		alwaysDoThisActionForFileError=FileError_NotSet;
}

QList<ItemOfCopyList> copyEngine::getTransferList()
{
	return threadOfTheTransfer.getTransferList();
}

ItemOfCopyList copyEngine::getTransferListEntry(const quint64 &id)
{
	return threadOfTheTransfer.getTransferListEntry(id);
}

void copyEngine::updateTheStatus()
{
	bool copy=transferIsStarted_internalVar;
	/*if(threadOfTheTransfer.haveContent())
		copy=true;*/
	bool listing=scanFileOrFolderThreadsPool.size()>0;
	EngineActionInProgress tempVal;
	if(copy && listing)
		tempVal=CopyingAndListing;
	else if(listing)
		tempVal=Listing;
	else if(copy)
		tempVal=Copying;
	else
		tempVal=Idle;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"emit actionInProgess("+QString::number(tempVal)+")");
	emit actionInProgess(tempVal);
}

void copyEngine::fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame)
{
	if(isSame)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file is same: "+source.absoluteFilePath());
		FileExistsAction tempAction=alwaysDoThisActionForFileExists;
		if(tempAction==FileExists_Overwrite || tempAction==FileExists_OverwriteIfNewer || tempAction==FileExists_OverwriteIfNotSameModificationDate)
			tempAction=FileExists_NotSet;
		switch(tempAction)
		{
			case FileExists_Skip:
			case FileExists_Rename:
				threadOfTheTransfer.setFileExistsAction(tempAction);
			break;
			default:
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
				fileIsSameDialogObject.setInfo(source);
				emit isInPause(true);
				fileIsSameDialogObject.exec();
				if(stopIt)
					return;
				FileExistsAction newAction=fileIsSameDialogObject.getAction();
				emit isInPause(false);
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
				if(newAction==FileExists_Cancel)
				{
					emit cancelAll();
					return;
				}
				if(fileIsSameDialogObject.getAlways() && newAction!=alwaysDoThisActionForFileExists)
				{
					alwaysDoThisActionForFileExists=newAction;
					switch(newAction)
					{
						default:
						case FileExists_Skip:
							emit newCollisionAction("skip");
						break;
						case FileExists_Rename:
							emit newCollisionAction("rename");
						break;
					}
				}
				if(fileIsSameDialogObject.getAlways() || newAction!=FileExists_Rename)
					threadOfTheTransfer.setFileExistsAction(newAction);
				else
					threadOfTheTransfer.setFileExistsAction(newAction,fileIsSameDialogObject.getNewName());
			break;
		}
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
		FileExistsAction tempAction=alwaysDoThisActionForFileExists;
		switch(tempAction)
		{
			case FileExists_Skip:
			case FileExists_Rename:
			case FileExists_Overwrite:
			case FileExists_OverwriteIfNewer:
			case FileExists_OverwriteIfNotSameModificationDate:
				threadOfTheTransfer.setFileExistsAction(tempAction);
			break;
			default:
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
				fileExistsDialogObject.setInfo(source,destination);
				emit isInPause(true);
				fileExistsDialogObject.exec();
				if(stopIt)
					return;
				FileExistsAction newAction=fileExistsDialogObject.getAction();
				emit isInPause(false);
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
				if(newAction==FileExists_Cancel)
				{
					emit cancelAll();
					return;
				}
				if(fileExistsDialogObject.getAlways() && newAction!=alwaysDoThisActionForFileExists)
				{
					alwaysDoThisActionForFileExists=newAction;
					switch(newAction)
					{
						default:
						case FileExists_Skip:
							emit newCollisionAction("skip");
						break;
						case FileExists_Rename:
							emit newCollisionAction("rename");
						break;
						case FileExists_Overwrite:
							emit newCollisionAction("overwrite");
						break;
						case FileExists_OverwriteIfNewer:
							emit newCollisionAction("overwriteIfNewer");
						break;
						case FileExists_OverwriteIfNotSameModificationDate:
							emit newCollisionAction("overwriteIfNotSameModificationDate");
						break;
					}
				}
				threadOfTheTransfer.setFileExistsAction(newAction);
			break;
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

void copyEngine::errorOnFile(QFileInfo fileInfo,QString errorString,bool canPutAtTheEnd)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
	if(!canPutAtTheEnd && alwaysDoThisActionForFileError==FileError_PutToEndOfTheList)
		alwaysDoThisActionForFileError=FileError_NotSet;
	FileErrorAction tempAction=alwaysDoThisActionForFileError;
	switch(tempAction)
	{
		case FileError_Skip:
		case FileError_Retry:
		case FileError_PutToEndOfTheList:
			threadOfTheTransfer.setFileErrorAction(tempAction);
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
			emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
			fileErrorDialogObject.setInfo(fileInfo,errorString,canPutAtTheEnd);
			emit isInPause(true);
			fileErrorDialogObject.exec();
			if(stopIt)
				return;
			FileErrorAction newAction=fileErrorDialogObject.getAction();
			emit isInPause(false);
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
			if(newAction==FileError_Cancel)
			{
				emit cancelAll();
				return;
			}
			if(fileErrorDialogObject.getAlways() && newAction!=alwaysDoThisActionForFileError)
			{
				alwaysDoThisActionForFileError=newAction;
				switch(newAction)
				{
					default:
					case FileError_Skip:
						emit newErrorAction("skip");
					break;
					case FileError_PutToEndOfTheList:
						emit newErrorAction("putToEndOfTheList");
					break;
				}
			}
			threadOfTheTransfer.setFileErrorAction(newAction);
		break;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

void copyEngine::errorOnFileInWriteThread(QFileInfo fileInfo,QString errorString,int indexOfWriteThread)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file have error"+fileInfo.absoluteFilePath()+", error: "+errorString);
	FileErrorAction tempAction=alwaysDoThisActionForFileError;
	switch(tempAction)
	{
		case FileError_Skip:
		case FileError_Retry:
		case FileError_PutToEndOfTheList:
			threadOfTheTransfer.setFileErrorActionInWriteThread(tempAction,indexOfWriteThread);
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
			emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
			fileErrorDialogObject.setInfo(fileInfo,errorString,true);
			emit isInPause(true);
			fileErrorDialogObject.exec();
			if(stopIt)
				return;
			FileErrorAction newAction=fileErrorDialogObject.getAction();
			emit isInPause(false);
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
			if(newAction==FileError_Cancel)
			{
				emit cancelAll();
				return;
			}
			if(fileErrorDialogObject.getAlways() && newAction!=alwaysDoThisActionForFileError)
			{
				alwaysDoThisActionForFileError=newAction;
				switch(newAction)
				{
					default:
					case FileError_Skip:
						emit newErrorAction("skip");
					break;
					case FileError_PutToEndOfTheList:
						emit newErrorAction("putToEndOfTheList");
					break;
				}
			}
			threadOfTheTransfer.setFileErrorActionInWriteThread(newAction,indexOfWriteThread);
		break;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

quint64 copyEngine::realByteTransfered()
{
	return threadOfTheTransfer.realByteTransfered();
}

//set the translate
void copyEngine::newLanguageLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, retranslate the widget options");
	if(tempWidget!=NULL)
		ui->retranslateUi(tempWidget);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"ui not loaded!");
}

//reset widget
void copyEngine::resetTempWidget()
{
	tempWidget=NULL;
}

void copyEngine::folderAlreadyExists(QFileInfo source,bool isSame,QFileInfo destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
	//firstly, locate the right thread
	scanFileOrFolder * thread = qobject_cast<scanFileOrFolder *>(sender());
	if(thread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to locate the thread");
		return;
	}
	//load the always action
	FolderExistsAction tempAction=alwaysDoThisActionForFolderExists;
	switch(tempAction)
	{
		case FolderExists_Skip:
		case FolderExists_Rename:
		case FolderExists_Merge:
			thread->setFolderExistsAction(tempAction);
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
			folderExistsDialogObject.setInfo(source,isSame,destination);
			folderExistsDialogObject.exec();
			if(stopIt)
				return;
			FolderExistsAction newAction=folderExistsDialogObject.getAction();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
			if(newAction==FolderExists_Cancel)
			{
				emit cancelAll();
				return;
			}
			if(folderExistsDialogObject.getAlways() && newAction!=alwaysDoThisActionForFolderExists)
			{
				setComboBoxFolderColision(newAction);
			}
			thread->setFolderExistsAction(newAction);
		break;
	}
}

void copyEngine::errorOnFolder(QFileInfo fileInfo,QString errorString)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
	//firstly, locate the right thread
	scanFileOrFolder * thread = qobject_cast<scanFileOrFolder *>(sender());
	if(thread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to locate the thread");
		return;
	}
	//load the always action
	FileErrorAction tempAction=alwaysDoThisActionForFolderError;
	switch(tempAction)
	{
		case FileError_Skip:
		case FileError_Retry:
		case FileError_PutToEndOfTheList:
			thread->setFolderErrorAction(tempAction);
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog");
			emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
			fileErrorDialogObject.setInfo(fileInfo,errorString,false);
			fileErrorDialogObject.exec();
			if(stopIt)
				return;
			FileErrorAction newAction=fileErrorDialogObject.getAction();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"close dialog: "+QString::number(newAction));
			if(newAction==FileError_Cancel)
			{
				emit cancelAll();
				return;
			}
			if(fileErrorDialogObject.getAlways() && newAction!=alwaysDoThisActionForFileError)
			{
				setComboBoxFolderError(newAction);
			}
			thread->setFolderErrorAction(newAction);
		break;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

void copyEngine::on_comboBoxFolderColision_currentIndexChanged(int index)
{
	switch(index)
	{
		case 0:
			setComboBoxFolderColision(FolderExists_NotSet,false);
		break;
		case 1:
			setComboBoxFolderColision(FolderExists_Merge,false);
		break;
		case 2:
			setComboBoxFolderColision(FolderExists_Skip,false);
		break;
		case 3:
			setComboBoxFolderColision(FolderExists_Rename,false);
		break;
	}
}

void copyEngine::on_comboBoxFolderError_currentIndexChanged(int index)
{
	switch(index)
	{
		case 0:
			setComboBoxFolderError(FileError_NotSet,false);
		break;
		case 1:
			setComboBoxFolderError(FileError_Skip,false);
		break;
	}
}

void copyEngine::setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox)
{
	alwaysDoThisActionForFolderExists=action;
	if(!changeComboBox || !uiIsInstalled)
		return;
	switch(action)
	{
		case FolderExists_Merge:
			ui->comboBoxFolderColision->setCurrentIndex(1);
		break;
		case FolderExists_Skip:
			ui->comboBoxFolderColision->setCurrentIndex(2);
		break;
		case FolderExists_Rename:
			ui->comboBoxFolderColision->setCurrentIndex(3);
		break;
		default:
			ui->comboBoxFolderColision->setCurrentIndex(0);
		break;
	}

}

void copyEngine::setComboBoxFolderError(FileErrorAction action,bool changeComboBox)
{
	alwaysDoThisActionForFileError=action;
	if(!changeComboBox || !uiIsInstalled)
		return;
	switch(action)
	{
		case FileError_Skip:
			ui->comboBoxFolderError->setCurrentIndex(1);
		break;
		default:
			ui->comboBoxFolderError->setCurrentIndex(0);
		break;
	}

}
