/** \file ListThread.cpp
\brief Define the list thread
\author alpha_one_x86
\version 0.3
\date 2011 */

#include "ListThread.h"

/// \todo do pushed or instant mount point (setDrive, ...)
/// \todo semaphore to prevent dual mkpath
/// \todo repair the mkpath, to use mkpath class before file transfer to have the folder
/// \todo do QThread( parent )
/** \todo when overwrite with large inode operation, it not start specificly the first in the list
  When that's is finish, send start file at real transfer start, not inode operation start **/
/** \todo group setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists) and setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists)
  and check if I can choose case by case if I wish overwrite, skip, ... */

ListThread::ListThread()
{
	moveToThread(this);
	start(HighPriority);
	putInPause			= false;
	sourceDrive			= "";
	sourceDriveMultiple		= false;
	destinationDrive		= "";
	destinationDriveMultiple	= false;
	stopIt				= false;
	bytesToTransfer			= 0;
	bytesTransfered			= 0;
	idIncrementNumber		= 1;
	actualRealByteTransfered	= 0;
	preOperationNumber		= 0;
	numberOfTranferRuning		= 0;
	numberOfTransferIntoToDoList	= 0;
	numberOfInodeOperation		= 0;
	maxSpeed			= 0;
	doRightTransfer			= false;
	keepDate			= false;
	blockSize			= 1024;
	alwaysDoThisActionForFileExists = FileExists_NotSet;
	/// \bug mising call for this part, or wrong sender detected
	qRegisterMetaType<DebugLevel>("DebugLevel");
	qRegisterMetaType<ItemOfCopyList>("ItemOfCopyList");
	qRegisterMetaType<QFileInfo>("QFileInfo");
	#if ! defined (Q_CC_GNU)
	ui->keepDate->setEnabled(false);
	ui->keepDate->setToolTip("Not supported with this compiler");
	#endif
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	connect(&timerUpdateDebugDialog,SIGNAL(timeout()),this,SLOT(timedUpdateDebugDialog()));
	timerUpdateDebugDialog.start(ULTRACOPIER_PLUGIN_DEBUG_WINDOW_TIMER);
	#endif
	connect(this,SIGNAL(tryCancel()),this,SLOT(cancel()));
	connect(this,SIGNAL(askNewTransferThread()),this,SLOT(createTransferThread()));
	emit askNewTransferThread();
}

ListThread::~ListThread()
{
	emit tryCancel();
	quit();
	waitCancel.acquire();
	wait();
}

//transfer is finished
void ListThread::transferInodeIsClosed()
{
	numberOfInodeOperation--;
	temp_transfer_thread=qobject_cast<TransferThread *>(QObject::sender());
	if(temp_transfer_thread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not located!"));
		return;
	}
	isFound=false;
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	int countLocalParse=0;
	#endif
	int_for_loop=0;
	while(int_for_loop<transferThreadList.size())
	{
		if(transferThreadList.at(int_for_loop).thread==temp_transfer_thread)
		{
			if(temp_transfer_thread->getStat()!=TransferThread::Idle)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not idle!"));
				return;
			}
			int_for_internal_loop=0;
			loop_size=actionToDoList.size();
			while(int_for_internal_loop<loop_size)
			{
				if(actionToDoList.at(int_for_internal_loop).id==transferThreadList.at(int_for_loop).id)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("[%1] have finish, put at idle; for id: %2").arg(int_for_loop).arg(transferThreadList.at(int_for_loop).id));
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=transferThreadList.at(int_for_loop).id;
					newAction.userAction.type=RemoveItem;
					actionDone << newAction;
					/// \todo check if item is at the right thread
					quint64 temp_id=transferThreadList.at(int_for_loop).id;
					actionToDoList.removeAt(int_for_internal_loop);
					/// \todo add the oversize to all size here
					bytesTransfered+=transferThreadList[int_for_loop].size;
					transferThreadList[int_for_loop].id=0;
					transferThreadList[int_for_loop].size=0;
					#ifdef ULTRACOPIER_PLUGIN_DEBUG
					countLocalParse++;
					#endif
					isFound=true;
					emit newTransferStop(temp_id);
					break;
				}
				int_for_internal_loop++;
			}
			if(!isFound)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(transferThreadList.at(int_for_loop).id).arg(int_for_loop));
				transferThreadList[int_for_loop].id=0;
				transferThreadList[int_for_loop].size=0;
			}
			break;
		}
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("countLocalParse: %1, actionToDoList.size(): %2").arg(countLocalParse).arg(actionToDoList.size()));
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	if(countLocalParse!=1)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("countLocalParse != 1"));
	#endif
	if(actionToDoList.size()==0)
		updateTheStatus();
	else
		doNewActions_inode_manipulation();
}

//transfer is finished
void ListThread::transferIsFinished()
{
	numberOfTranferRuning--;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start transferIsFinished(), numberOfTranferRuning: "+QString::number(numberOfTranferRuning));
	doNewActions_start_transfer();
}

//put the current file at bottom
void ListThread::transferPutAtBottom()
{
	TransferThread *transfer=qobject_cast<TransferThread *>(QObject::sender());
	if(transfer==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not located!"));
		return;
	}
	transfer->skip();
	bool isFound=false;
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	int countLocalParse=0;
	#endif
	int index=0;
	while(index<transferThreadList.size())
	{
		if(transferThreadList.at(index).thread==transfer)
		{
			int indexAction=0;
			while(indexAction<actionToDoList.size())
			{
				if(actionToDoList.at(indexAction).id==transferThreadList.at(index).id)
				{
					//push for interface at the end
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=transferThreadList.at(index).id;
					newAction.userAction.type=MoveItem;
					newAction.position=actionToDoList.size()-1;
					actionDone << newAction;
					//do the wait stat
					actionToDoList[index].isRunning=false;
					//move at the end
					actionToDoList.move(indexAction,actionToDoList.size()-1);
					//reset the thread list stat
					transferThreadList[index].id=0;
					transferThreadList[index].size=0;
					#ifdef ULTRACOPIER_PLUGIN_DEBUG
					countLocalParse++;
					#endif
					isFound=true;
					break;
				}
				indexAction++;
			}
			if(!isFound)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(transferThreadList.at(index).id).arg(index));
				transferThreadList[index].id=0;
				transferThreadList[index].size=0;
			}
			break;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("countLocalParse: %1").arg(countLocalParse));
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	if(countLocalParse!=1)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("countLocalParse != 1"));
	#endif
}

//set the copy info and options before runing
void ListThread::setRightTransfer(const bool doRightTransfer)
{
	this->doRightTransfer=doRightTransfer;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setRightTransfer(doRightTransfer);
		index++;
	}
}

//set keep date
void ListThread::setKeepDate(const bool keepDate)
{
	this->keepDate=keepDate;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setKeepDate(keepDate);
		index++;
	}
}

//set block size in KB
void ListThread::setBlockSize(const int blockSize)
{
	this->blockSize=blockSize;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setBlockSize(blockSize);
		index++;
	}
}

//set auto start
void ListThread::setAutoStart(const bool autoStart)
{
	this->autoStart=autoStart;
}

//set check destination folder
void ListThread::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
	this->checkDestinationFolderExists=checkDestinationFolderExists;
	for(int i=0;i<scanFileOrFolderThreadsPool.size();i++)
		scanFileOrFolderThreadsPool.at(i).thread->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void ListThread::folderTransfer(const QString &source,const QString &destination,const int &numberOfItem)
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
				addToRmPath(source,numberOfItem);
			emit newFolderListing(source);
			if(numberOfItem==0)
				addToMkPath(destination);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer not found (plugin copy engine)");
}

void ListThread::fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo)
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
			addToTransfer(sourceFileInfo,destinationFileInfo,scanFileOrFolderThreadsPool.at(index).mode);
			emit newActionOnList();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer not found (plugin copy engine)");
}

bool ListThread::haveSameSource(QStringList sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(sourceDriveMultiple)
		return false;
	if(sourceDrive.isEmpty())
		return true;
	int index=0;
	while(index<sources.size())
	{
//		if(threadOfTheTransfer.getDrive(sources.at(index))!=sourceDrive)
//			return false;
		index++;
	}
	return true;
}

bool ListThread::haveSameDestination(QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(destinationDriveMultiple)
		return false;
	if(destinationDrive.isEmpty())
		return true;
	int index=0;
	while(index<destination.size())
	{
//		if(threadOfTheTransfer.getDrive(destination.at(index))!=destinationDrive)
//			return false;
		index++;
	}
	return true;
}

scanFileOrFolder * ListThread::newScanThread(CopyMode mode)
{
	//create new thread because is auto-detroyed
	scanFileOrFolderThread newThread;
	newThread.mode=mode;
	newThread.thread=new scanFileOrFolder();
	connect(newThread.thread,SIGNAL(finished()),							this,SLOT(scanThreadHaveFinish()));
	connect(newThread.thread,SIGNAL(started()),							this,SLOT(updateTheStatus()));
	connect(newThread.thread,SIGNAL(finished()),							this,SLOT(updateTheStatus()));
	connect(newThread.thread,SIGNAL(folderTransfer(QString,QString,int)),				this,SLOT(folderTransfer(QString,QString,int)));
	connect(newThread.thread,SIGNAL(fileTransfer(QFileInfo,QFileInfo)),				this,SLOT(fileTransfer(QFileInfo,QFileInfo)));
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(newThread.thread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	#endif
	connect(newThread.thread,SIGNAL(errorOnFolder(QFileInfo,QString)),				this,SLOT(errorOnFolder(QFileInfo,QString)));
	connect(newThread.thread,SIGNAL(folderAlreadyExists(QFileInfo,QFileInfo,bool)),			this,SLOT(folderAlreadyExists(QFileInfo,QFileInfo,bool)));
	scanFileOrFolderThreadsPool << newThread;
	newThread.thread->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
	updateTheStatus();
	return newThread.thread;
}

void ListThread::scanThreadHaveFinish(bool skipFirstRemove)
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
                                        delete scanFileOrFolderThreadsPool.at(index).thread;
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
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Auto start the copy");
			updateTheStatus();
			doNewActions_inode_manipulation();
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Put the copy engine in pause");
			updateTheStatus();
			emit isInPause(true);
		}
	}
}

bool ListThread::newCopy(QStringList sources,QString destination)
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
	return true;
}

bool ListThread::newMove(QStringList sources,QString destination)
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
/*	int index=0;
	while(index<sources.size() && !sourceDriveMultiple)
	{
		QString tempDrive=threadOfTheTransfer.getDrive(sources.at(index));
		if(sourceDrive=="")
			sourceDrive=tempDrive;
		if(tempDrive!=sourceDrive)
			sourceDriveMultiple=true;
		index++;
	}*/
/*	QString tempDrive=threadOfTheTransfer.getDrive(destination);
	if(!destinationDriveMultiple)
	{
		if(tempDrive=="")
			destinationDrive=tempDrive;
		if(tempDrive!=destinationDrive)
			destinationDriveMultiple=true;
	}*/
	return true;
}

void ListThread::setDrive(QStringList drives)
{
	this->drives=drives;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setDrive(drives);
		index++;
	}
}

void ListThread::setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists)
{
	this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
		index++;
	}
}

//set the folder local colision
void ListThread::setFolderColision(FolderExistsAction alwaysDoThisActionForFolderExists)
{
	this->alwaysDoThisActionForFolderExists=alwaysDoThisActionForFolderExists;
}

void ListThread::pause()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(putInPause)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Seam already in pause!");
		return;
	}
	putInPause=true;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->pause();
		index++;
	}
	emit isInPause(true);
}

void ListThread::resume()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(!putInPause)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Seam already resumed!");
		return;
	}
	putInPause=false;
	doNewActions_inode_manipulation();
	doNewActions_start_transfer();
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->resume();
		index++;
	}
	emit isInPause(false);
}

void ListThread::skip(quint64 id)
{
	skipInternal(id);
}

bool ListThread::skipInternal(quint64 id)
{
	int index=0;
	while(index<transferThreadList.size())
	{
		if(transferThreadList.at(index).id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("skip one transfer: %1").arg(id));
			transferThreadList.at(index).thread->skip();
			return true;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"skip transfer not found");
	return false;
}

void ListThread::cancel()
{
	quit();
	if(stopIt)
		return;
	stopIt=true;
	disconnect(this);
        int index=0;
	while(index<transferThreadList.size())
        {
                transferThreadList.at(index).thread->stop();
		delete transferThreadList.at(index).thread;//->deleteLayer();
		transferThreadList[index].thread=NULL;
                index++;
	}
        index=0;
        while(index<scanFileOrFolderThreadsPool.size())
        {
                scanFileOrFolderThreadsPool.at(index).thread->stop();
		delete scanFileOrFolderThreadsPool.at(index).thread;//->deleteLayer();
		scanFileOrFolderThreadsPool[index].thread=NULL;
                index++;
	}
	waitCancel.release();
}

//first = current transfered byte, second = byte to transfer
QPair<quint64,quint64> ListThread::getGeneralProgression()
{
	qint64 oversize=0;
	qint64 currentProgression=0;
	int index=0;
	while(index<transferThreadList.size())
	{
		if(transferThreadList.at(index).thread->getStat()==TransferThread::Transfer)
		{
			qint64 copiedSize=transferThreadList.at(index).thread->copiedSize();
			currentProgression+=copiedSize;
			if(copiedSize>transferThreadList.at(index).size)
				oversize+=copiedSize-transferThreadList.at(index).size;
		}
		index++;
	}
	QPair<quint64,quint64> returnedValue;
	returnedValue.first=bytesTransfered+currentProgression;
	returnedValue.second=bytesToTransfer+oversize;
	return returnedValue;
}

//first = current transfered byte, second = byte to transfer
returnSpecificFileProgression ListThread::getFileProgression(quint64 id)
{
	returnSpecificFileProgression returnedValue;
	int index=0;
	while(index<transferThreadList.size())
	{
		if(transferThreadList.at(index).id==id)
		{
			returnedValue.copiedSize=transferThreadList.at(index).thread->copiedSize();
			qint64 oversize=0;
			if(returnedValue.copiedSize>(quint64)transferThreadList.at(index).size)
				oversize=returnedValue.copiedSize-transferThreadList.at(index).size;
			returnedValue.totalSize=transferThreadList.at(index).size+oversize;
			returnedValue.haveBeenLocated=true;
			return returnedValue;
		}
		index++;
	}
	returnedValue.copiedSize=0;
	returnedValue.totalSize=0;
	returnedValue.haveBeenLocated=false;
	return returnedValue;
}

//edit the transfer list
QList<returnActionOnCopyList> ListThread::getActionOnList()
{
	QList<returnActionOnCopyList> actionDone;
	actionDone=this->actionDone;
	this->actionDone.clear();
	return actionDone;
}

//speed limitation
qint64 ListThread::getSpeedLimitation()
{
	return maxSpeed;
}

bool ListThread::setSpeedLimitation(qint64 speedLimitation)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"maxSpeed: "+QString::number(maxSpeed));
	maxSpeed=speedLimitation;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setMaxSpeed(speedLimitation/ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER);
		index++;
	}
	return true;
}

QList<ItemOfCopyList> ListThread::getTransferList()
{
	QList<ItemOfCopyList> listToSend;
	int size=actionToDoList.size();
	for (int index=0;index<size;++index) {
		if(actionToDoList.at(index).type==ActionType_Transfer)
		{
			ItemOfCopyList newItemToSend;
			newItemToSend.sourceFullPath=actionToDoList.at(index).source.absoluteFilePath();
			newItemToSend.sourceFileName=actionToDoList.at(index).source.fileName();
			newItemToSend.destinationFullPath=actionToDoList.at(index).destination.absoluteFilePath();
			newItemToSend.destinationFileName=actionToDoList.at(index).destination.fileName();
			newItemToSend.size=actionToDoList.at(index).size;
			newItemToSend.mode=actionToDoList.at(index).mode;
			newItemToSend.id=actionToDoList.at(index).id;
			listToSend << newItemToSend;
		}
	}
	return listToSend;
}

ItemOfCopyList ListThread::getTransferListEntry(quint64 id)
{
	ItemOfCopyList newItemToSend;
	int size=actionToDoList.size();
	for (int index=0;index<size;++index) {
		if(id==actionToDoList.at(index).id)
		{
			if(actionToDoList.at(index).type!=ActionType_Transfer)
				return newItemToSend;
			newItemToSend.sourceFullPath=actionToDoList.at(index).source.absoluteFilePath();
			newItemToSend.sourceFileName=actionToDoList.at(index).source.fileName();
			newItemToSend.destinationFullPath=actionToDoList.at(index).destination.absoluteFilePath();
			newItemToSend.destinationFileName=actionToDoList.at(index).destination.fileName();
			newItemToSend.size=actionToDoList.at(index).size;
			newItemToSend.mode=actionToDoList.at(index).mode;
			newItemToSend.id=actionToDoList.at(index).id;
			return newItemToSend;
		}
	}
	return newItemToSend;
}

void ListThread::updateTheStatus()
{
	/*if(threadOfTheTransfer.haveContent())
		copy=true;*/
	updateTheStatus_listing=scanFileOrFolderThreadsPool.size()>0;
	updateTheStatus_copying=actionToDoList.size()>0;
	if(updateTheStatus_copying && updateTheStatus_listing)
		updateTheStatus_action_in_progress=CopyingAndListing;
	else if(updateTheStatus_listing)
		updateTheStatus_action_in_progress=Listing;
	else if(updateTheStatus_copying)
		updateTheStatus_action_in_progress=Copying;
	else
		updateTheStatus_action_in_progress=Idle;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"emit actionInProgess("+QString::number(updateTheStatus_action_in_progress)+")");
	emit actionInProgess(updateTheStatus_action_in_progress);
}

quint64 ListThread::realByteTransfered()
{
	QPair<quint64,quint64> item=ListThread::getGeneralProgression();
	return item.first;
}

//set data local to the thread
void ListThread::setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists)
{
	this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
	int index=0;
	while(index<transferThreadList.size())
	{
		transferThreadList.at(index).thread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
		index++;
	}
}

//mk path to do
quint64 ListThread::addToMkPath(const QDir& folder)
{
	actionToDo temp;
	temp.type	= ActionType_MkPath;
	temp.id		= generateIdNumber();
	temp.source.setFile(folder.absolutePath());
	temp.isRunning	= false;
	actionToDoList << temp;
	return temp.id;
}

//add file transfer to do
quint64 ListThread::addToTransfer(const QFileInfo& source,const QFileInfo& destination,const CopyMode& mode)
{
	//add to transfer list
	numberOfTransferIntoToDoList++;
	bytesToTransfer+= source.size();
	actionToDo temp;
	temp.type	= ActionType_Transfer;
	temp.id		= generateIdNumber();
	temp.size	= source.size();
	temp.source	= source;
	temp.destination= destination;
	temp.mode	= mode;
	temp.isRunning	= false;
	actionToDoList << temp;
	//push the new transfer to interface
	returnActionOnCopyList newAction;
	newAction.type				= AddingItem;
	newAction.addAction.id			= temp.id;
	newAction.addAction.sourceFullPath	= source.absoluteFilePath();
	newAction.addAction.sourceFileName	= source.fileName();
	newAction.addAction.destinationFullPath	= destination.absoluteFilePath();
	newAction.addAction.destinationFileName	= destination.fileName();
	newAction.addAction.size		= temp.size;
	newAction.addAction.mode		= mode;
	actionDone << newAction;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"source: "+source.absoluteFilePath()+",destination: "+destination.absoluteFilePath()+", add entry: "+QString::number(temp.id));
	return temp.id;
}

//add rm path to do
void ListThread::addToRmPath(const QDir& folder,const int& inodeToRemove)
{
	actionToDo temp;
	temp.type	= ActionType_RmPath;
	temp.id		= generateIdNumber();
	temp.size	= inodeToRemove;
	temp.source.setFile(folder.absolutePath());
	temp.isRunning	= false;
	actionToDoList << temp;
}

//generate id number
quint64 ListThread::generateIdNumber()
{
	idIncrementNumber++;
	if(idIncrementNumber>(((quint64)1024*1024)*1024*1024*2))
		idIncrementNumber=0;
	return idIncrementNumber;
}

//warning the first entry is accessible will copy
void ListThread::removeItems(QList<int> ids)
{
	for(int i=0;i<ids.size();i++)
		skipInternal(ids.at(i));
}

//put on top
void ListThread::moveItemsOnTop(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int indexToMove=0;
	for (int i=0; i<actionToDoList.size(); ++i) {
		if(ids.contains(actionToDoList.at(i).id))
		{
			ids.removeOne(actionToDoList.at(i).id);
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(indexToMove));
			returnActionOnCopyList newAction;
			newAction.type=OtherAction;
			newAction.userAction.id=actionToDoList.at(i).id;
			newAction.userAction.type=MoveItem;
			newAction.position=indexToMove;
			actionDone << newAction;
			actionToDoList.move(i,indexToMove);
			indexToMove++;
			if(ids.size()==0)
			{
				emit newActionOnList();
				return;
			}
		}
	}
	emit newActionOnList();
}

//move up
void ListThread::moveItemsUp(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int lastGoodPositionExtern=0;
	int lastGoodPositionReal=0;
	bool haveGoodPosition=false;
	bool haveChanged=false;
	for (int i=0; i<actionToDoList.size(); ++i) {
		if(actionToDoList.at(i).type==ActionType_Transfer)
		{
			if(ids.contains(actionToDoList.at(i).id))
			{
				if(haveGoodPosition)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i-1));
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=actionToDoList.at(i).id;
					newAction.userAction.type=MoveItem;
					newAction.position=lastGoodPositionExtern;
					actionDone << newAction;
					actionToDoList.swap(i,lastGoodPositionReal);
					haveChanged=true;
				}
				else
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
				ids.removeOne(actionToDoList.at(i).id);
				if(ids.size()==0)
				{
					if(haveChanged)
						emit newActionOnList();
					return;
				}
			}
			else
			{
				lastGoodPositionExtern++;
				lastGoodPositionReal=i;
				haveGoodPosition=true;
			}
		}
	}
	emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//move down
void ListThread::moveItemsDown(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int lastGoodPositionExtern=numberOfTransferIntoToDoList;
	int lastGoodPositionReal=0;
	bool haveGoodPosition=false;
	bool haveChanged=false;
	for (int i=actionToDoList.size()-1; i>=0; --i) {
		if(actionToDoList.at(i).type==ActionType_Transfer)
		{
			if(ids.contains(actionToDoList.at(i).id))
			{
				if(haveGoodPosition)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i+1));
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=actionToDoList.at(i).id;
					newAction.userAction.type=MoveItem;
					newAction.position=lastGoodPositionReal;
					actionDone << newAction;
					actionToDoList.swap(i,lastGoodPositionReal);
					haveChanged=true;
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
				}
				ids.removeOne(actionToDoList.at(i).id);
				if(ids.size()==0)
				{
					if(haveChanged)
						emit newActionOnList();
					return;
				}
			}
			else
			{
				lastGoodPositionExtern--;
				lastGoodPositionReal=i;
				haveGoodPosition=true;
			}
		}
	}
	emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//put on bottom
void ListThread::moveItemsOnBottom(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int lastGoodPositionExtern=numberOfTransferIntoToDoList;
	int lastGoodPositionReal=actionToDoList.size()-1;
	for (int i=lastGoodPositionReal; i>=0; --i) {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Check action on item ")+QString::number(i));
		if(ids.contains(actionToDoList.at(i).id))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(lastGoodPositionReal));
			ids.removeOne(actionToDoList.at(i).id);
			returnActionOnCopyList newAction;
			newAction.type=OtherAction;
			newAction.userAction.id=actionToDoList.at(i).id;
			newAction.userAction.type=MoveItem;
			newAction.position=lastGoodPositionExtern;
			actionDone << newAction;
			actionToDoList.move(i,lastGoodPositionReal);
			lastGoodPositionReal--;
			lastGoodPositionExtern--;
			if(ids.size()==0)
			{
				emit newActionOnList();
				return;
			}
		}
	}
	emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//do new actions
void ListThread::doNewActions_start_transfer()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoList.size(): %1").arg(actionToDoList.size()));
	if(stopIt || putInPause)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//lunch the transfer in WaitForTheTransfer
	int_for_loop=0;
	while(int_for_loop<transferThreadList.size() && numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
	{
		if(transferThreadList.at(int_for_loop).thread->getStat()==TransferThread::WaitForTheTransfer)
		{
			transferThreadList.at(int_for_loop).thread->startTheTransfer();
			numberOfTranferRuning++;
		}
		int_for_loop++;
	}
	int_for_loop=0;
	while(int_for_loop<transferThreadList.size() && numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
	{
		if(transferThreadList.at(int_for_loop).thread->getStat()==TransferThread::PreOperation)
		{
			transferThreadList.at(int_for_loop).thread->startTheTransfer();
			numberOfTranferRuning++;
		}
		int_for_loop++;
	}
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	QStringList stringList;
	int_for_loop=0;
	loop_size=actionToDoList.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoList.at(int_for_loop).type!=ActionType_Transfer)
			stringList << QString("type: %1, source: %2").arg(actionToDoList.at(int_for_loop).type).arg(actionToDoList.at(int_for_loop).source.absoluteFilePath());
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoList type: %1").arg(stringList.join(",")));
	#endif
}

void ListThread::doNewActions_inode_manipulation()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoList.size(): %1").arg(actionToDoList.size()));
	if(stopIt || putInPause)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//lunch the pre-op or inode op
	int_for_loop=0;
	number_rm_path_moved=0;
	loop_size=actionToDoList.size();
	while(int_for_loop<loop_size && numberOfInodeOperation<transferThreadList.size())
	{
		if(!actionToDoList.at(int_for_loop).isRunning)
		{
			int_for_internal_loop=0;
			switch(actionToDoList.at(int_for_loop).type)
			{
				case ActionType_Transfer:
				while(int_for_internal_loop<transferThreadList.size())
				{
					/**
					  transferThreadList.at(int_for_internal_loop).id==0) /!\ important!
					  Because the other thread can have call doNewAction before than this thread have the finish event parsed!
					  I this case it lose all data
					  */
					if(transferThreadList.at(int_for_internal_loop).thread->getStat()==TransferThread::Idle
							&& transferThreadList.at(int_for_internal_loop).id==0) // /!\ important!
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("[%1] is idle, use it for %2").arg(transferThreadList.at(int_for_loop).id).arg(actionToDoList.at(int_for_loop).destination.fileName()));
						transferThreadList[int_for_internal_loop].id=actionToDoList.at(int_for_loop).id;
						transferThreadList[int_for_internal_loop].size=actionToDoList.at(int_for_loop).size;
						transferThreadList.at(int_for_internal_loop).thread->setFiles(
							actionToDoList.at(int_for_loop).source.absoluteFilePath(),
							actionToDoList.at(int_for_loop).size,
							actionToDoList.at(int_for_loop).destination.absoluteFilePath(),
							actionToDoList.at(int_for_loop).mode
							);
						actionToDoList[int_for_loop].isRunning=true;
						numberOfInodeOperation++;
						ItemOfCopyList temp;
						temp.destinationFileName=actionToDoList.at(int_for_loop).destination.fileName();
						temp.destinationFullPath=actionToDoList.at(int_for_loop).destination.absoluteFilePath();
						temp.id=actionToDoList[int_for_loop].id;
						temp.mode=actionToDoList[int_for_loop].mode;
						temp.size=actionToDoList[int_for_loop].size;
						temp.sourceFileName=actionToDoList.at(int_for_loop).source.fileName();
						temp.sourceFullPath=actionToDoList.at(int_for_loop).source.absoluteFilePath();
						emit newTransferStart(temp);		//should update interface information on this event
						break;
					}
					int_for_internal_loop++;
				}
				if(int_for_internal_loop==transferThreadList.size())
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to found free thread to do the transfer");
					return;
				}
				break;
				case ActionType_MkPath:
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("launch mkpath: %1").arg(actionToDoList.at(int_for_loop).source.absoluteFilePath()));
					mkPathQueue.addPath(actionToDoList.at(int_for_loop).source.absoluteFilePath());
					actionToDoList[int_for_loop].isRunning=true;
					numberOfInodeOperation++;
				break;
				case ActionType_RmPath:
					if((int_for_loop+number_rm_path_moved)>=(loop_size-1))
					{
						if(numberOfTranferRuning)
							break;
						else
							actionToDoList[int_for_loop].size=0;
					}
					if(actionToDoList.at(int_for_loop).size==0)
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("launch rmpath: %1").arg(actionToDoList.at(int_for_loop).source.absoluteFilePath()));
						rmPathQueue.addPath(actionToDoList.at(int_for_loop).source.absoluteFilePath());
						actionToDoList[int_for_loop].isRunning=true;
						numberOfInodeOperation++;
					}
					else
					{
						actionToDoList.move(int_for_loop,loop_size-1);
						number_rm_path_moved++;
						if(!numberOfTranferRuning)
							actionToDoList[int_for_loop].size=0;
						int_for_loop--;
					}
				break;
			}
		}
		int_for_loop++;
	}
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	QStringList stringList;
	int_for_loop=0;
	loop_size=actionToDoList.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoList.at(int_for_loop).type!=ActionType_Transfer)
			stringList << QString("type: %1, source: %2").arg(actionToDoList.at(int_for_loop).type).arg(actionToDoList.at(int_for_loop).source.absoluteFilePath());
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoList type: %1").arg(stringList.join(",")));
	#endif
}


//restart transfer if it can
void ListThread::restartTransferIfItCan()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	TransferThread *transfer=qobject_cast<TransferThread *>(QObject::sender());
	if(transfer==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not located!"));
		return;
	}
	if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER && transfer->getStat()==TransferThread::WaitForTheTransfer)
	{
		transfer->startTheTransfer();
		numberOfTranferRuning++;
	}
	doNewActions_start_transfer();
}

void ListThread::mkPathFirstFolderFinish()
{
	int_for_loop=0;
	loop_size=actionToDoList.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoList.at(int_for_loop).type==ActionType_MkPath)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("stop mkpath: %1").arg(actionToDoList.at(int_for_loop).source.absoluteFilePath()));
			actionToDoList.removeAt(int_for_loop);
			numberOfInodeOperation--;
			if(actionToDoList.size()==0)
				updateTheStatus();
			else
				doNewActions_inode_manipulation();
			return;
		}
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to found item into the todo list");
}

void ListThread::rmPathFirstFolderFinish()
{
	int_for_loop=0;
	loop_size=actionToDoList.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoList.at(int_for_loop).type==ActionType_RmPath)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("stop rmpath: %1").arg(actionToDoList.at(int_for_loop).source.absoluteFilePath()));
			actionToDoList.removeAt(int_for_loop);
			numberOfInodeOperation--;
			if(actionToDoList.size()==0)
				updateTheStatus();
			else
				doNewActions_inode_manipulation();
			return;
		}
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to found item into the todo list");
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW

void ListThread::timedUpdateDebugDialog()
{
	QStringList newList;
	int index=0;
	while(index<transferThreadList.size())
	{
		QString stat;
		switch(transferThreadList.at(index).thread->getStat())
		{
		case TransferThread::Idle:
			stat="Idle";
			break;
		case TransferThread::PreOperation:
			stat="PreOperation";
			break;
		case TransferThread::WaitForTheTransfer:
			stat="WaitForTheTransfer";
			break;
		case TransferThread::Transfer:
			stat="Transfer";
			break;
		case TransferThread::PostOperation:
			stat="PostOperation";
			break;
		case TransferThread::PostTransfer:
			stat="PostTransfer";
			break;
		default:
			stat=QString("??? (%1)").arg(transferThreadList.at(index).thread->getStat());
			break;
		}
		newList << QString("%1) (%3,%4) %2")
			.arg(index)
			.arg(stat)
			.arg(transferThreadList.at(index).thread->readingLetter())
			.arg(transferThreadList.at(index).thread->writingLetter());
		index++;
	}
	QStringList newList2;
	index=0;
	while(index<actionToDoList.size())
	{
		if(actionToDoList.at(index).type==ActionType_Transfer)
			newList2 << QString("%1 %2 %3")
				   .arg(actionToDoList.at(index).source.absoluteFilePath())
				   .arg(actionToDoList.at(index).size)
				   .arg(actionToDoList.at(index).destination.absoluteFilePath());
		if(index>(transferThreadList.size()+2))
		{
			newList2 << QString("...");
			break;
		}
		index++;
	}
	emit updateTheDebugInfo(newList,newList2,numberOfInodeOperation);
}

#endif

/// \note Can be call without queue because all call will be serialized
void ListThread::fileAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame)
{
	emit send_fileAlreadyExists(source,destination,isSame,qobject_cast<TransferThread *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
void ListThread::errorOnFile(const QFileInfo &fileInfo,const QString &errorString)
{
	emit send_errorOnFile(fileInfo,errorString,qobject_cast<TransferThread *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
void ListThread::folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame)
{
	emit send_folderAlreadyExists(source,destination,isSame,qobject_cast<scanFileOrFolder *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void ListThread::errorOnFolder(const QFileInfo &fileInfo,const QString &errorString)
{
	emit send_errorOnFolder(fileInfo,errorString,qobject_cast<scanFileOrFolder *>(sender()));
}

//to run the thread
void ListThread::run()
{
	connect(&mkPathQueue,SIGNAL(firstFolderFinish()),this,SLOT(mkPathFirstFolderFinish()),Qt::QueuedConnection);
	connect(&rmPathQueue,SIGNAL(firstFolderFinish()),this,SLOT(rmPathFirstFolderFinish()),Qt::QueuedConnection);
	exec();
}

/// \to create transfer thread
void ListThread::createTransferThread()
{
	if(stopIt)
		return;
	transfer newItem;
	newItem.thread=new TransferThread();
	newItem.id=0;
	newItem.size=0;
	newItem.thread->setRightTransfer(doRightTransfer);
	newItem.thread->setKeepDate(keepDate);
	newItem.thread->setBlockSize(blockSize);
	newItem.thread->setDrive(drives);
	newItem.thread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
	newItem.thread->setMaxSpeed(maxSpeed/ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER);
	transferThreadList << newItem;
	connect(newItem.thread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(errorOnFile(QFileInfo,QString)),				this,SLOT(errorOnFile(QFileInfo,QString)),				Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(fileAlreadyExists(QFileInfo,QFileInfo,bool)),		this,SLOT(fileAlreadyExists(QFileInfo,QFileInfo,bool)),			Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(tryPutAtBottom()),					this,SLOT(transferPutAtBottom()),					Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(readStopped()),						this,SLOT(transferIsFinished()),					Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(readStopped()),						this,SLOT(doNewActions_start_transfer()),				Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(preOperationStopped()),					this,SLOT(doNewActions_start_transfer()),				Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(postOperationStopped()),					this,SLOT(transferInodeIsClosed()),					Qt::QueuedConnection);
	connect(newItem.thread,SIGNAL(checkIfItCanBeResumed()),					this,SLOT(restartTransferIfItCan()),					Qt::QueuedConnection);
	newItem.thread->start();
	newItem.thread->setObjectName(QString("transfer %1").arg(transferThreadList.size()-1));
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	newItem.thread->setId(transferThreadList.size()-1);
	#endif
	if(transferThreadList.size()>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
		return;
	if(stopIt)
		return;
	emit askNewTransferThread();
}
