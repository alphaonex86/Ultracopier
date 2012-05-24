#include "ListThread.h"

/// \todo do pushed or instant mount point (setDrive, ...)
/// \todo semaphore to prevent dual mkpath
/// \todo repair the mkpath, to use mkpath class before file transfer to have the folder
/// \todo do QThread( parent )
/** \todo when overwrite with large inode operation, it not start specificly the first in the list
  When that's is finish, send start file at real transfer start, not inode operation start **/
/** \todo group setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists) and setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists)
  and check if I can choose case by case if I wish overwrite, skip, ... */
/// \todo move directly the folder when it's needed
/// \todo connect folder error of mkpath and rmpath class

ListThread::ListThread(FacilityInterface * facilityInterface)
{
	qRegisterMetaType<DebugLevel>("DebugLevel");
	qRegisterMetaType<ItemOfCopyList>("ItemOfCopyList");
	qRegisterMetaType<QFileInfo>("QFileInfo");
	qRegisterMetaType<CopyMode>("CopyMode");
	qRegisterMetaType<QList<Filters_rules> >("QList<Filters_rules>");

	timerActionDone=NULL;
	timerProgression=NULL;
	moveToThread(this);
	start(HighPriority);
	this->facilityInterface		= facilityInterface;
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
	osBufferLimit			= 512;
	alwaysDoThisActionForFileExists = FileExists_NotSet;
	doChecksum			= false;
	checksumIgnoreIfImpossible	= true;
	checksumOnlyOnError		= true;
	osBuffer			= false;
	osBufferLimited			= false;
	forcedMode			= false;

	#if ! defined (Q_CC_GNU)
	ui->keepDate->setEnabled(false);
	ui->keepDate->setToolTip("Not supported with this compiler");
	#endif
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	connect(&timerUpdateDebugDialog,SIGNAL(timeout()),this,SLOT(timedUpdateDebugDialog()));
	timerUpdateDebugDialog.start(ULTRACOPIER_PLUGIN_DEBUG_WINDOW_TIMER);
	#endif
	connect(this,		SIGNAL(tryCancel()),							this,SLOT(cancel()),							Qt::QueuedConnection);
	connect(this,		SIGNAL(askNewTransferThread()),						this,SLOT(createTransferThread()),					Qt::QueuedConnection);
	connect(&mkPathQueue,	SIGNAL(firstFolderFinish()),						this,SLOT(mkPathFirstFolderFinish()),					Qt::QueuedConnection);
	connect(&rmPathQueue,	SIGNAL(firstFolderFinish()),						this,SLOT(rmPathFirstFolderFinish()),					Qt::QueuedConnection);
	connect(&mkPathQueue,	SIGNAL(errorOnFolder(QFileInfo,QString)),				this,SIGNAL(mkPathErrorOnFolder(QFileInfo,QString)),			Qt::QueuedConnection);
	connect(&rmPathQueue,	SIGNAL(errorOnFolder(QFileInfo,QString)),				this,SIGNAL(rmPathErrorOnFolder(QFileInfo,QString)),			Qt::QueuedConnection);
	connect(this,		SIGNAL(send_syncTransferList()),					this,SLOT(syncTransferList_internal()),					Qt::QueuedConnection);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(&mkPathQueue,	SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	Qt::QueuedConnection);
	connect(&rmPathQueue,	SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	Qt::QueuedConnection);
	#endif // ULTRACOPIER_PLUGIN_DEBUG

	emit askNewTransferThread();
	mkpathTransfer.release();
}

ListThread::~ListThread()
{
	emit tryCancel();
	waitCancel.acquire();
	quit();
	wait();
}

//transfer is finished
void ListThread::transferInodeIsClosed()
{
	qint64 copiedSize;
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
	if(temp_transfer_thread->getStat()!=TransferThread::Idle)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not idle!"));
		return;
	}
	int_for_internal_loop=0;
	loop_size=actionToDoListTransfer.size();
	while(int_for_internal_loop<loop_size)
	{
		if(actionToDoListTransfer.at(int_for_internal_loop).id==temp_transfer_thread->transferId)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("[%1] have finish, put at idle; for id: %2").arg(int_for_internal_loop).arg(temp_transfer_thread->transferId));
			returnActionOnCopyList newAction;
			newAction.type=RemoveItem;
			newAction.addAction.id=temp_transfer_thread->transferId;
			newAction.userAction.position=int_for_internal_loop;
			actionDone << newAction;
			/// \todo check if item is at the right thread
			actionToDoListTransfer.removeAt(int_for_internal_loop);
			if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
				updateTheStatus();

			copiedSize=temp_transfer_thread->copiedSize();
			if(copiedSize>(qint64)temp_transfer_thread->transferSize)
			{
				oversize=copiedSize-temp_transfer_thread->transferSize;
				bytesToTransfer+=oversize;
				bytesTransfered+=oversize;
			}

			bytesTransfered+=temp_transfer_thread->transferSize;
			temp_transfer_thread->transferId=0;
			temp_transfer_thread->transferSize=0;
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			countLocalParse++;
			#endif
			isFound=true;
			//emit newActionOnList();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"numberOfTranferRuning: "+QString::number(numberOfTranferRuning));
			if(actionToDoListTransfer.size()==0)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"numberOfTranferRuning==0");
				actionToDoListInode << actionToDoListInode_afterTheTransfer;
				actionToDoListInode_afterTheTransfer.clear();
				doNewActions_inode_manipulation();
			}
			break;
		}
		int_for_internal_loop++;
	}
	if(!isFound)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(temp_transfer_thread->transferId).arg(int_for_loop));
		temp_transfer_thread->transferId=0;
		temp_transfer_thread->transferSize=0;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("countLocalParse: %1, actionToDoList.size(): %2").arg(countLocalParse).arg(actionToDoListTransfer.size()));
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	if(countLocalParse!=1)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("countLocalParse != 1"));
	#endif
	doNewActions_inode_manipulation();
}

//transfer is finished
void ListThread::transferIsFinished()
{
	temp_transfer_thread=qobject_cast<TransferThread *>(QObject::sender());
	if(temp_transfer_thread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("transfer thread not located!"));
		return;
	}
//	emit newTransferStop(temp_transfer_thread->transferId);
	numberOfTranferRuning--;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start transferIsFinished(), numberOfTranferRuning: "+QString::number(numberOfTranferRuning));
	doNewActions_start_transfer();
}

/** \brief put the current file at bottom in case of error
\note ONLY IN CASE OF ERROR */
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
	int indexAction=0;
	while(indexAction<actionToDoListTransfer.size())
	{
		if(actionToDoListTransfer.at(indexAction).id==transfer->transferId)
		{
			//push for interface at the end
			returnActionOnCopyList newAction;
			newAction.type=MoveItem;
			newAction.addAction.id=transfer->transferId;
			newAction.userAction.position=actionToDoListTransfer.size()-1;
			actionDone << newAction;
			//do the wait stat
			actionToDoListTransfer[indexAction].isRunning=false;
			//move at the end
			actionToDoListTransfer.move(indexAction,actionToDoListTransfer.size()-1);
			//reset the thread list stat
			transfer->transferId=0;
			transfer->transferSize=0;
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
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(transfer->transferId));
		transfer->transferId=0;
		transfer->transferSize=0;
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
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->setRightTransfer(doRightTransfer);
		index++;
	}
}

//set keep date
void ListThread::setKeepDate(const bool keepDate)
{
	this->keepDate=keepDate;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->setKeepDate(keepDate);
		index++;
	}
}

//set block size in KB
void ListThread::setBlockSize(const int blockSize)
{
	this->blockSize=blockSize;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->setBlockSize(blockSize);
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
		scanFileOrFolderThreadsPool.at(i)->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void ListThread::fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo,const CopyMode &mode)
{
	addToTransfer(sourceFileInfo,destinationFileInfo,mode);
}

// -> add thread safe, by Qt::BlockingQueuedConnection
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

// -> add thread safe, by Qt::BlockingQueuedConnection
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start with: "+QString::number(mode));

	//create new thread because is auto-detroyed
	scanFileOrFolderThreadsPool << new scanFileOrFolder(mode);
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(finishedTheListing()),					this,SLOT(scanThreadHaveFinish()),				Qt::QueuedConnection);
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(fileTransfer(QFileInfo,QFileInfo,CopyMode)),			this,SLOT(fileTransfer(QFileInfo,QFileInfo,CopyMode)),		Qt::QueuedConnection);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	#endif
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(newFolderListing(QString)),					this,SIGNAL(newFolderListing(QString)));
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(addToRmPath(QString,int)),					this,SLOT(addToRmPath(QString,int)),				Qt::QueuedConnection);
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(addToMkPath(QString)),					this,SLOT(addToMkPath(QString)),				Qt::QueuedConnection);

	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(errorOnFolder(QFileInfo,QString)),				this,SLOT(errorOnFolder(QFileInfo,QString)),			Qt::QueuedConnection);
	connect(scanFileOrFolderThreadsPool.last(),SIGNAL(folderAlreadyExists(QFileInfo,QFileInfo,bool)),		this,SLOT(folderAlreadyExists(QFileInfo,QFileInfo,bool)),	Qt::QueuedConnection);

	scanFileOrFolderThreadsPool.last()->setFilters(include,exclude);
	scanFileOrFolderThreadsPool.last()->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
	if(scanFileOrFolderThreadsPool.size()==1)
		updateTheStatus();
	scanFileOrFolderThreadsPool.last()->setRenamingRules(firstRenamingRule,otherRenamingRule);
	return scanFileOrFolderThreadsPool.last();
}

void ListThread::scanThreadHaveFinish(bool skipFirstRemove)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listing thread have finish, skipFirstRemove: "+QString::number(skipFirstRemove));
	if(!skipFirstRemove)
	{
		scanFileOrFolder * senderThread = qobject_cast<scanFileOrFolder *>(QObject::sender());
		if(senderThread==NULL)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"sender pointer null (plugin copy engine)");
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
			delete senderThread;
			scanFileOrFolderThreadsPool.removeOne(senderThread);
			if(scanFileOrFolderThreadsPool.size()==0)
				updateTheStatus();
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
	if(scanFileOrFolderThreadsPool.size()>0)
	{
		//then start the next listing threads
		if(scanFileOrFolderThreadsPool.first()->isFinished())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Start listing thread");
			scanFileOrFolderThreadsPool.first()->start();
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"The listing thread is already running");
	}
	else
	{
		if(autoStart)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Auto start the copy");
			startGeneralTransfer();
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Put the copy engine in pause");
			putInPause=true;
			emit isInPause(true);
		}
	}
}

void ListThread::startGeneralTransfer()
{
	doNewActions_inode_manipulation();
	if(timerProgression!=NULL)
		timerProgression->start();
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"internal bug, pointer on QTimer is NULL");
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newCopy(QStringList sources,QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+sources.join(";")+", destination: "+destination);
	scanFileOrFolder * scanFileOrFolderThread = newScanThread(Copy);
	if(scanFileOrFolderThread==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to get new thread");
		return false;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the listing");
	scanFileOrFolderThread->addToList(sources,destination);
	scanThreadHaveFinish(true);
	return true;
}

// -> add thread safe, by Qt::BlockingQueuedConnection
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
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->setDrive(drives);
		index++;
	}
}

void ListThread::setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists)
{
	this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
		index++;
	}
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void ListThread::syncTransferList()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit send_syncTransferList();
}

//set the folder local colision
void ListThread::setFolderColision(FolderExistsAction alwaysDoThisActionForFolderExists)
{
	this->alwaysDoThisActionForFolderExists=alwaysDoThisActionForFolderExists;
}

bool ListThread::getReturnBoolToCopyEngine()
{
	return returnBoolToCopyEngine;
}

QPair<quint64,quint64> ListThread::getReturnPairQuint64ToCopyEngine()
{
	return returnPairQuint64ToCopyEngine;
}

ItemOfCopyList ListThread::getReturnItemOfCopyListToCopyEngine()
{
	return returnItemOfCopyListToCopyEngine;
}

void ListThread::set_doChecksum(bool doChecksum)
{
	this->doChecksum=doChecksum;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_doChecksum(doChecksum);
		index++;
	}
}

void ListThread::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
	this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
		index++;
	}
}

void ListThread::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
	this->checksumOnlyOnError=checksumOnlyOnError;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_checksumOnlyOnError(checksumOnlyOnError);
		index++;
	}
}

void ListThread::set_osBuffer(bool osBuffer)
{
	this->osBuffer=osBuffer;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_osBuffer(osBuffer);
		index++;
	}
}

void ListThread::set_osBufferLimited(bool osBufferLimited)
{
	this->osBufferLimited=osBufferLimited;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_osBufferLimited(osBufferLimited);
		index++;
	}
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
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->pause();
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
	startGeneralTransfer();
	doNewActions_start_transfer();
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->resume();
		index++;
	}
	emit isInPause(false);
}

void ListThread::skip(const quint64 &id)
{
	skipInternal(id);
//	emit newActionOnList();
}

bool ListThread::skipInternal(const quint64 &id)
{
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		if(transferThreadList.at(index)->transferId==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("skip one transfer: %1").arg(id));
			transferThreadList.at(index)->skip();
			return true;
		}
		index++;
	}
	int_for_internal_loop=0;
	loop_size=actionToDoListTransfer.size();
	while(int_for_internal_loop<loop_size)
	{
		if(actionToDoListTransfer.at(int_for_internal_loop).id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("[%1] remove at not running, for id: %2").arg(int_for_internal_loop).arg(id));
			returnActionOnCopyList newAction;
			newAction.type=RemoveItem;
			newAction.addAction.id=id;
			newAction.userAction.position=int_for_internal_loop;
			actionDone << newAction;
			actionToDoListTransfer.removeAt(int_for_internal_loop);
			if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
				updateTheStatus();
			return true;
		}
		int_for_internal_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("skip transfer not found: %1").arg(id));
	return false;
}

void ListThread::cancel()
{
	if(stopIt)
	{
		waitCancel.release();
		return;
	}
	stopIt=true;
	disconnect(this);
        int index=0;
	loop_size=transferThreadList.size();
	while(index<loop_size)
        {
		transferThreadList.at(index)->stop();
		delete transferThreadList.at(index);//->deleteLayer();
		transferThreadList[index]=NULL;
                index++;
	}
        index=0;
	loop_size=scanFileOrFolderThreadsPool.size();
	while(index<loop_size)
        {
		scanFileOrFolderThreadsPool.at(index)->stop();
		delete scanFileOrFolderThreadsPool.at(index);//->deleteLayer();
		scanFileOrFolderThreadsPool[index]=NULL;
                index++;
	}
	if(timerActionDone!=NULL)
		delete timerActionDone;
	if(timerProgression!=NULL)
		delete timerProgression;
	timerActionDone=NULL;
	timerProgression=NULL;
	quit();
	waitCancel.release();
}

//speed limitation
qint64 ListThread::getSpeedLimitation()
{
	return maxSpeed;
}

bool ListThread::setSpeedLimitation(const qint64 &speedLimitation)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"maxSpeed: "+QString::number(maxSpeed));
	maxSpeed=speedLimitation;
	int_for_loop=0;
	loop_size=transferThreadList.size();
	while(int_for_loop<loop_size)
	{
		transferThreadList.at(int_for_loop)->setMaxSpeed(speedLimitation/ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER);
		int_for_loop++;
	}
	return true;
}

void ListThread::updateTheStatus()
{
	/*if(threadOfTheTransfer.haveContent())
		copy=true;*/
	updateTheStatus_listing=scanFileOrFolderThreadsPool.size()>0;
	updateTheStatus_copying=actionToDoListTransfer.size()>0 || actionToDoListInode.size()>0 || actionToDoListInode_afterTheTransfer.size()>0;
	if(updateTheStatus_copying && updateTheStatus_listing)
		updateTheStatus_action_in_progress=CopyingAndListing;
	else if(updateTheStatus_listing)
		updateTheStatus_action_in_progress=Listing;
	else if(updateTheStatus_copying)
		updateTheStatus_action_in_progress=Copying;
	else
		updateTheStatus_action_in_progress=Idle;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"emit actionInProgess("+QString::number(updateTheStatus_action_in_progress)+")");
	emit actionInProgess(updateTheStatus_action_in_progress);
	if(updateTheStatus_action_in_progress==Idle)
		sendActionDone();
}

//set data local to the thread
void ListThread::setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists)
{
	this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
	int_for_loop=0;
	loop_size=transferThreadList.size();
	while(int_for_loop<loop_size)
	{
		transferThreadList.at(int_for_loop)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
		int_for_loop++;
	}
}

//mk path to do
quint64 ListThread::addToMkPath(const QString& folder)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"folder: "+folder);
	actionToDoInode temp;
	temp.type	= ActionType_MkPath;
	temp.id		= generateIdNumber();
	temp.folder.setFile(folder);
	temp.isRunning	= false;
	actionToDoListInode << temp;
	return temp.id;
}

//add rm path to do
void ListThread::addToRmPath(const QString& folder,const int& inodeToRemove)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"folder: "+folder+",inodeToRemove: "+QString::number(inodeToRemove));
	actionToDoInode temp;
	temp.type	= ActionType_RmPath;
	temp.id		= generateIdNumber();
	temp.size	= inodeToRemove;
	temp.folder.setFile(folder);
	temp.isRunning	= false;
	if(inodeToRemove==0)
		actionToDoListInode << temp;
	else
		actionToDoListInode_afterTheTransfer << temp;
}

//send action done
void ListThread::sendActionDone()
{
	if(actionDone.size()>0)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		emit newActionOnList(actionDone);
		actionDone.clear();
	}
	if(actionToDoListTransfer.size()>0)
	{
		if(timerActionDone!=NULL)
			timerActionDone->start();
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"internal bug, pointer on QTimer is NULL");
	}
}

//send progression
void ListThread::sendProgression()
{
	if(actionToDoListTransfer.size()==0)
		return;
	qint64 copiedSize,totalSize,localOverSize;
	QList<ProgressionItem> progressionList;
	TransferThread *thread;
	oversize=0;
	currentProgression=0;
	int_for_loop=0;
	loop_size=transferThreadList.size();
	while(int_for_loop<loop_size)
	{
		thread=transferThreadList.at(int_for_loop);
		if(thread->getStat()==TransferThread::Transfer)
		{
			copiedSize=thread->copiedSize();
			currentProgression+=copiedSize;
			if(copiedSize>(qint64)thread->transferSize)
				localOverSize=copiedSize-thread->transferSize;
			else
				localOverSize=0;
			totalSize=thread->transferSize+localOverSize;
			ProgressionItem tempItem;
			tempItem.current=copiedSize;
			tempItem.id=thread->transferId;
			tempItem.total=totalSize;
			progressionList << tempItem;
			oversize+=localOverSize;
		}
		int_for_loop++;
	}
	emit pushFileProgression(progressionList);
	emit pushGeneralProgression(bytesTransfered+currentProgression,bytesToTransfer+oversize);
	if(timerProgression!=NULL)
		timerProgression->start();
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"can't start QTimer because the pointer is NULL");
}

//send the progression, after full reset of the interface (then all is empty)
void ListThread::syncTransferList_internal()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit syncReady();
	actionDone.clear();
	//do list operation
	TransferThread *transferThread;
	loop_size=actionToDoListTransfer.size();
	loop_sub_size=transferThreadList.size();
	//this loop to have at max ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT*ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT, not ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT*transferThreadList.size()
	for(int_for_loop=0; int_for_loop<loop_size; ++int_for_loop) {
		const actionToDoTransfer &item=actionToDoListTransfer.at(int_for_loop);
		returnActionOnCopyList newAction;
		newAction.type				= PreOperation;
		newAction.addAction.id			= item.id;
		newAction.addAction.sourceFullPath	= item.source.absoluteFilePath();
		newAction.addAction.sourceFileName	= item.source.fileName();
		newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath();
		newAction.addAction.destinationFileName	= item.destination.fileName();
		newAction.addAction.size		= item.size;
		newAction.addAction.mode		= item.mode;
		actionDone << newAction;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("id: %1, size: %2, name: %3, size2: %4").arg(item.id).arg(item.size).arg(item.source.absoluteFilePath()).arg(newAction.addAction.size));
		if(item.isRunning)
		{
			for(int_for_internal_loop=0; int_for_internal_loop<loop_sub_size; ++int_for_internal_loop) {
				transferThread=transferThreadList.at(int_for_internal_loop);
				returnActionOnCopyList newAction;
				newAction.type				= PreOperation;
				newAction.addAction.id			= item.id;
				newAction.addAction.sourceFullPath	= item.source.absoluteFilePath();
				newAction.addAction.sourceFileName	= item.source.fileName();
				newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath();
				newAction.addAction.destinationFileName	= item.destination.fileName();
				newAction.addAction.size		= item.size;
				newAction.addAction.mode		= item.mode;
				actionDone << newAction;
				if(transferThread->getStat()!=TransferThread::PreOperation)
				{
					returnActionOnCopyList newAction;
					switch(transferThread->getStat())
					{
						case TransferThread::Transfer:
							newAction.type=Transfer;
						break;
						case TransferThread::PostTransfer:
							newAction.type=PostOperation;
						break;
						default:
						break;
					}
					newAction.addAction.id			= item.id;
					actionDone << newAction;
				}
			}
		}
	}
}

//add file transfer to do
quint64 ListThread::addToTransfer(const QFileInfo& source,const QFileInfo& destination,const CopyMode& mode)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"source: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
	if(actionToDoListTransfer.size()==0)
	{
		if(timerActionDone!=NULL)
			timerActionDone->start();
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"can't start QTimer because the pointer is NULL");
	}
	//add to transfer list
	numberOfTransferIntoToDoList++;
	bytesToTransfer+= source.size();
	actionToDoTransfer temp;
	temp.id		= generateIdNumber();
	temp.size	= source.size();
	temp.source	= source;
	temp.destination= destination;
	temp.mode	= mode;
	temp.isRunning	= false;
	actionToDoListTransfer << temp;
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("source: %1, destination: %2, add entry: %3, size: %4, size2: %5").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).arg(temp.id).arg(temp.size).arg(newAction.addAction.size));
	return temp.id;
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
void ListThread::removeItems(const QList<int> &ids)
{
	for(int i=0;i<ids.size();i++)
		skipInternal(ids.at(i));
//	emit newActionOnList();
}

//put on top
void ListThread::moveItemsOnTop(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int indexToMove=0;
	loop_size=actionToDoListTransfer.size();
	for (int i=0; i<loop_size; ++i) {
		if(ids.contains(actionToDoListTransfer.at(i).id))
		{
			ids.removeOne(actionToDoListTransfer.at(i).id);
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(indexToMove));
			returnActionOnCopyList newAction;
			newAction.type=MoveItem;
			newAction.addAction.id=actionToDoListTransfer.at(i).id;
			newAction.userAction.moveAt=indexToMove;
			newAction.userAction.position=i;
			actionDone << newAction;
			actionToDoListTransfer.move(i,indexToMove);
			indexToMove++;
			if(ids.size()==0)
			{
				//emit newActionOnList();
				return;
			}
		}
	}
//	emit newActionOnList();
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
	loop_size=actionToDoListTransfer.size();
	for (int i=0; i<loop_size; ++i) {
		if(ids.contains(actionToDoListTransfer.at(i).id))
		{
			if(haveGoodPosition)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i-1));
				returnActionOnCopyList newAction;
				newAction.type=MoveItem;
				newAction.addAction.id=actionToDoListTransfer.at(i).id;
				newAction.userAction.moveAt=lastGoodPositionExtern;
				newAction.userAction.position=i;
				actionDone << newAction;
				actionToDoListTransfer.swap(i,lastGoodPositionReal);
				haveChanged=true;
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
			ids.removeOne(actionToDoListTransfer.at(i).id);
			if(ids.size()==0)
			{
/*				if(haveChanged)
					emit newActionOnList();*/
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
	//emit newActionOnList();
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
	for (int i=actionToDoListTransfer.size()-1; i>=0; --i) {
		if(ids.contains(actionToDoListTransfer.at(i).id))
		{
			if(haveGoodPosition)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i+1));
				returnActionOnCopyList newAction;
				newAction.type=MoveItem;
				newAction.addAction.id=actionToDoListTransfer.at(i).id;
				newAction.userAction.moveAt=lastGoodPositionReal;
				newAction.userAction.position=i;
				actionDone << newAction;
				actionToDoListTransfer.swap(i,lastGoodPositionReal);
				haveChanged=true;
			}
			else
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
			}
			ids.removeOne(actionToDoListTransfer.at(i).id);
			if(ids.size()==0)
			{
/*				if(haveChanged)
					emit newActionOnList();*/
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
	//emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//put on bottom
void ListThread::moveItemsOnBottom(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//do list operation
	int lastGoodPositionExtern=numberOfTransferIntoToDoList;
	int lastGoodPositionReal=actionToDoListTransfer.size()-1;
	for (int i=lastGoodPositionReal; i>=0; --i) {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Check action on item ")+QString::number(i));
		if(ids.contains(actionToDoListTransfer.at(i).id))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(lastGoodPositionReal));
			ids.removeOne(actionToDoListTransfer.at(i).id);
			returnActionOnCopyList newAction;
			newAction.type=MoveItem;
			newAction.addAction.id=actionToDoListTransfer.at(i).id;
			newAction.userAction.moveAt=lastGoodPositionExtern;
			newAction.userAction.position=i;
			actionDone << newAction;
			actionToDoListTransfer.move(i,lastGoodPositionReal);
			lastGoodPositionReal--;
			lastGoodPositionExtern--;
			if(ids.size()==0)
			{
//				emit newActionOnList();
				return;
			}
		}
	}
//	emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

/** \brief give the forced mode, to export/import transfer list */
void ListThread::forceMode(const CopyMode &mode)
{
	if(mode==Copy)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Force mode to copy"));
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Force mode to move"));
	this->mode=mode;
	forcedMode=true;
}

void ListThread::exportTransferList(const QString &fileName)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QFile transferFile(fileName);
	if(transferFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
	{
		transferFile.write(QString("Ultracopier-0.3;Transfer-list;").toUtf8());
		if(!forcedMode)
			transferFile.write(QString("Transfer;").toUtf8());
		else
		{
			if(mode==Copy)
				transferFile.write(QString("Copy;").toUtf8());
			else
				transferFile.write(QString("Move;").toUtf8());
		}
		transferFile.write(QString("CopyEngine-0.3\n").toUtf8());
		bool haveError=false;
		int size=actionToDoListTransfer.size();
		for (int index=0;index<size;++index) {
			if(actionToDoListTransfer.at(index).mode==Copy)
			{
				if(!forcedMode || mode==Copy)
				{
					if(forcedMode)
						transferFile.write(QString("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
					else
						transferFile.write(QString("Copy;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
				}
				else
					haveError=true;
			}
			else if(actionToDoListTransfer.at(index).mode==Move)
			{
				if(!forcedMode || mode==Move)
				{
					if(forcedMode)
						transferFile.write(QString("Move;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
					else
						transferFile.write(QString("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
				}
				else
					haveError=true;
			}
		}
		if(haveError)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()));
			emit errorTransferList(tr("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()));
		}
		transferFile.close();
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Unable to save the transfer list: %1").arg(transferFile.errorString()));
		emit errorTransferList(tr("Unable to save the transfer list: %1").arg(transferFile.errorString()));
		return;
	}
}

void ListThread::importTransferList(const QString &fileName)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QFile transferFile(fileName);
	if(transferFile.open(QIODevice::ReadOnly))
	{
		QString content;
		QByteArray data=transferFile.readLine(64);
		if(data.size()<=0)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Problem at the reading, or file size is null"));
			emit errorTransferList(tr("Problem at the reading, or file size is null"));
			return;
		}
		content=QString::fromUtf8(data);
		if(content!="Ultracopier-0.3;Transfer-list;Transfer;CopyEngine-0.3\n" && content!="Ultracopier-0.3;Transfer-list;Copy;CopyEngine-0.3\n" && content!="Ultracopier-0.3;Transfer-list;Move;CopyEngine-0.3\n")
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Wrong header: \"%1\"").arg(content));
			emit errorTransferList(tr("Wrong header: \"%1\"").arg(content));
			return;
		}
		bool transferListMixedMode=false;
		if(content=="Ultracopier-0.3;Transfer-list;Transfer;CopyEngine-0.3\n")
		{
			if(forcedMode)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("The transfer list is in mixed mode, but this instance is not in this mode"));
				emit errorTransferList(tr("The transfer list is in mixed mode, but this instance is not in this mode"));
				return;
			}
			else
				transferListMixedMode=true;
		}
		if(content=="Ultracopier-0.3;Transfer-list;Copy;CopyEngine-0.3\n" && (!forcedMode || mode==Copy))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("The transfer list is in mixed mode, but this instance is not in this mode"));
			emit errorTransferList(tr("The transfer list is in mixed mode, but this instance is not in this mode"));
			return;
		}
		if(content=="Ultracopier-0.3;Transfer-list;Move;CopyEngine-0.3\n" && (!forcedMode || mode==Move))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("The transfer list is in mixed mode, but this instance is not in this mode"));
			emit errorTransferList(tr("The transfer list is in mixed mode, but this instance is not in this mode"));
			return;
		}
		bool errorFound=false,ignored_by_wrong_type=false;
		QRegExp correctLine;
		if(transferListMixedMode)
			correctLine=QRegExp("^(Copy|Move);[^;]+;[^;]+\n$");
		else
			correctLine=QRegExp("^[^;]+;[^;]+\n$");
		QStringList args;
		CopyMode tempMode;
		do
		{
			data=transferFile.readLine(65535*2);
			if(data.size()>0)
			{
				content=QString::fromUtf8(data);
				//do the import here
				if(content.contains(correctLine))
				{
					content.remove("\n");
					args=content.split(";");
					if(forcedMode)
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("New data to import in forced mode: %2,%3").arg(args.at(0)).arg(args.at(1)));
						addToTransfer(QFileInfo(args.at(0)),QFileInfo(args.at(1)),mode);
					}
					else
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("New data to import: %1,%2,%3").arg(args.at(0)).arg(args.at(1)).arg(args.at(2)));
						if(args.at(0)=="Copy")
							tempMode=Copy;
						else
							tempMode=Move;
						addToTransfer(QFileInfo(args.at(1)),QFileInfo(args.at(2)),tempMode);
					}
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Wrong line syntax: %1").arg(content));
					errorFound=true;
				}
			}
		}
		while(data.size()>0);
		transferFile.close();
		if(errorFound)
			emit warningTransferList(tr("Some error have been found during the line parsing"));
		else if(ignored_by_wrong_type)
			emit warningTransferList(tr("Some list is ignored because it not corresponds to the window transfer type"));
//		emit newActionOnList();
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Unable to open the transfer list: %1").arg(transferFile.errorString()));
		emit errorTransferList(tr("Unable to open the transfer list: %1").arg(transferFile.errorString()));
		return;
	}
}

//do new actions
void ListThread::doNewActions_start_transfer()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoListTransfer.size(): %1, numberOfTranferRuning: %2").arg(actionToDoListTransfer.size()).arg(numberOfTranferRuning));
	if(stopIt || putInPause)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//lunch the transfer in WaitForTheTransfer
	int_for_loop=0;
	loop_size=transferThreadList.size();
	while(int_for_loop<loop_size && numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
	{
		if(transferThreadList.at(int_for_loop)->getStat()==TransferThread::WaitForTheTransfer)
		{
			transferThreadList.at(int_for_loop)->startTheTransfer();
			numberOfTranferRuning++;
		}
		int_for_loop++;
	}
	int_for_loop=0;
	while(int_for_loop<loop_size && numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
	{
		if(transferThreadList.at(int_for_loop)->getStat()==TransferThread::PreOperation)
		{
			transferThreadList.at(int_for_loop)->startTheTransfer();
			numberOfTranferRuning++;
		}
		int_for_loop++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"numberOfTranferRuning: "+QString::number(numberOfTranferRuning));
}

/** \brief lunch the pre-op or inode op
  1) locate the next next item to do into the both list
    1a) optimisation posible on the mkpath/rmpath
  2) determine what need be lunched
  3) lunch it, rerun the 2)
  */
void ListThread::doNewActions_inode_manipulation()
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("actionToDoList.size(): %1").arg(actionToDoList.size()));
	if(stopIt || putInPause)
		return;
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//lunch the pre-op or inode op
	int_for_loop=0;
	int_for_internal_loop=0;
	number_rm_path_moved=0;
	int_for_transfer_thread_search=0;
	actionToDoListTransfer_count=actionToDoListTransfer.count();
	actionToDoListInode_count=actionToDoListInode.count();
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	//search the next transfer action to do
	while(int_for_loop<actionToDoListTransfer_count)
	{
		if(!actionToDoListTransfer[int_for_loop].isRunning)
		{
			//search the next inode action to do
			while(int_for_internal_loop<actionToDoListInode_count)
			{
				if(!actionToDoListInode[int_for_internal_loop].isRunning)
				{
					if(actionToDoListTransfer[int_for_loop].id<actionToDoListInode[int_for_internal_loop].id)
					{
						//do the tranfer action in the next code
						break;
					}
					else
					{
						//do the inode action
						#include "ListThread_InodeAction.cpp"
					}
				}
				int_for_internal_loop++;
			}
			actionToDoTransfer& currentActionToDoTransfer=actionToDoListTransfer[int_for_loop];
			//do the tranfer action
			while(int_for_transfer_thread_search<loop_sub_size_transfer_thread_search)
			{
				/**
					transferThreadList.at(int_for_transfer_thread_search)->transferId==0) /!\ important!
					Because the other thread can have call doNewAction before than this thread have the finish event parsed!
					I this case it lose all data
					*/
				currentTransferThread=transferThreadList[int_for_transfer_thread_search];
				if(currentTransferThread->getStat()==TransferThread::Idle && currentTransferThread->transferId==0) // /!\ important!
				{
					currentTransferThread->transferId=currentActionToDoTransfer.id;
					currentTransferThread->transferSize=currentActionToDoTransfer.size;
					currentTransferThread->setFiles(
						currentActionToDoTransfer.source.absoluteFilePath(),
						currentActionToDoTransfer.size,
						currentActionToDoTransfer.destination.absoluteFilePath(),
						currentActionToDoTransfer.mode
						);
					currentActionToDoTransfer.isRunning=true;

					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("[%1] id: %2 is idle, use it for %3").arg(int_for_loop).arg(currentTransferThread->transferId).arg(currentActionToDoTransfer.destination.absoluteFilePath()));

					/// \note wrong position? Else write why it's here
					returnActionOnCopyList newAction;
					newAction.type				= PreOperation;
					newAction.addAction.id			= currentActionToDoTransfer.id;
					newAction.addAction.sourceFullPath	= currentActionToDoTransfer.source.absoluteFilePath();
					newAction.addAction.sourceFileName	= currentActionToDoTransfer.source.fileName();
					newAction.addAction.destinationFullPath	= currentActionToDoTransfer.destination.absoluteFilePath();
					newAction.addAction.destinationFileName	= currentActionToDoTransfer.destination.fileName();
					newAction.addAction.size		= currentActionToDoTransfer.size;
					newAction.addAction.mode		= currentActionToDoTransfer.mode;
					actionDone << newAction;
					int_for_transfer_thread_search++;
					break;
				}
				int_for_transfer_thread_search++;
			}
			if(int_for_internal_loop==loop_sub_size_transfer_thread_search)
			{
				/// \note Can be normal when all thread is not initialized
				//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to found free thread to do the transfer");
				break;
			}
			numberOfInodeOperation++;
			if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
				return;
		}
		int_for_loop++;
	}
	//search the next inode action to do
	while(int_for_internal_loop<actionToDoListInode_count)
	{
		if(!actionToDoListInode[int_for_internal_loop].isRunning)
		{
			//do the inode action
			#include "ListThread_InodeAction.cpp"
		}
		int_for_internal_loop++;
	}
	//error  checking
	if((actionToDoListTransfer_count+actionToDoListInode_count)>ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("The index have been detected as out of max range: %1>%2").arg(actionToDoListTransfer_count+actionToDoListInode_count).arg(ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT));
		return;
	}
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

/// \brief update the transfer stat
void ListThread::newTransferStat(TransferThread::TransferStat stat,quint64 id)
{
	returnActionOnCopyList newAction;
	switch(stat)
	{
		case TransferThread::Idle:
			return;
		break;
		case TransferThread::PreOperation:
			return;
		break;
		case TransferThread::WaitForTheTransfer:
			return;
		break;
		case TransferThread::Transfer:
			newAction.type=Transfer;
		break;
		case TransferThread::PostTransfer:
			newAction.type=PostOperation;
		break;
		case TransferThread::PostOperation:
			return;
		break;
		default:
		break;
	}
	newAction.addAction.id			= id;
	actionDone << newAction;
}

void ListThread::set_osBufferLimit(unsigned int osBufferLimit)
{
	this->osBufferLimit=osBufferLimit;
	int index=0;
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		transferThreadList.at(index)->set_osBufferLimit(osBufferLimit);
		index++;
	}
}

void ListThread::set_setFilters(QList<Filters_rules> include,QList<Filters_rules> exclude)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("include.size(): %1, exclude.size(): %2").arg(include.size()).arg(exclude.size()));
	this->include=include;
	this->exclude=exclude;
	int index=0;
	while(index<scanFileOrFolderThreadsPool.size())
	{
		scanFileOrFolderThreadsPool.at(index)->setFilters(include,exclude);
		index++;
	}
}

void ListThread::set_sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
	this->firstRenamingRule=firstRenamingRule;
	this->otherRenamingRule=otherRenamingRule;
	emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void ListThread::mkPathFirstFolderFinish()
{
	int_for_loop=0;
	loop_size=actionToDoListInode.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoListInode.at(int_for_loop).type==ActionType_MkPath)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("stop mkpath: %1").arg(actionToDoListInode.at(int_for_loop).folder.absoluteFilePath()));
			actionToDoListInode.removeAt(int_for_loop);
			if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
				updateTheStatus();
			numberOfInodeOperation--;
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
	loop_size=actionToDoListInode.size();
	while(int_for_loop<loop_size)
	{
		if(actionToDoListInode.at(int_for_loop).type==ActionType_RmPath)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("stop rmpath: %1").arg(actionToDoListInode.at(int_for_loop).folder.absoluteFilePath()));
			actionToDoListInode.removeAt(int_for_loop);
			if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
				updateTheStatus();
			numberOfInodeOperation--;
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
	loop_sub_size_transfer_thread_search=transferThreadList.size();
	while(index<loop_sub_size_transfer_thread_search)
	{
		QString stat;
		switch(transferThreadList.at(index)->getStat())
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
			stat=QString("??? (%1)").arg(transferThreadList.at(index)->getStat());
			break;
		}
		newList << QString("%1) (%3,%4) %2")
			.arg(index)
			.arg(stat)
			.arg(transferThreadList.at(index)->readingLetter())
			.arg(transferThreadList.at(index)->writingLetter());
		index++;
	}
	QStringList newList2;
	index=0;
	loop_size=actionToDoListTransfer.size();
	while(index<loop_size)
	{
		newList2 << QString("%1 %2 %3")
			.arg(actionToDoListTransfer.at(index).source.absoluteFilePath())
			.arg(actionToDoListTransfer.at(index).size)
			.arg(actionToDoListTransfer.at(index).destination.absoluteFilePath());
		if(index>(actionToDoListTransfer.size()+2))
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
	timerActionDone=new QTimer();
	timerActionDone->setSingleShot(true);
	timerActionDone->setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_TRASNFER_LIST);
	timerProgression=new QTimer();
	timerProgression->setSingleShot(true);
	timerProgression->setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_PROGRESSION);
	connect(timerActionDone,SIGNAL(timeout()),							this,SLOT(sendActionDone()));
	connect(timerProgression,SIGNAL(timeout()),							this,SLOT(sendProgression()));

	exec();
}

/// \to create transfer thread
void ListThread::createTransferThread()
{
	if(stopIt)
		return;
	transferThreadList << new TransferThread();
	TransferThread * last=transferThreadList.last();
	last->transferId=0;
	last->transferSize=0;
	last->setRightTransfer(doRightTransfer);
	last->setKeepDate(keepDate);
	last->setBlockSize(blockSize);
	last->setDrive(drives);
	last->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
	last->setMaxSpeed(maxSpeed/ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER);
	last->set_doChecksum(doChecksum);
	last->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
	last->set_checksumOnlyOnError(checksumOnlyOnError);
	last->set_osBuffer(osBuffer);
	last->set_osBufferLimited(osBufferLimited);
	last->set_osBufferLimit(osBufferLimit);

	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(last,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	Qt::QueuedConnection);
	#endif // ULTRACOPIER_PLUGIN_DEBUG
	connect(last,SIGNAL(errorOnFile(QFileInfo,QString)),				this,SLOT(errorOnFile(QFileInfo,QString)),				Qt::QueuedConnection);
	connect(last,SIGNAL(fileAlreadyExists(QFileInfo,QFileInfo,bool)),		this,SLOT(fileAlreadyExists(QFileInfo,QFileInfo,bool)),			Qt::QueuedConnection);
	connect(last,SIGNAL(tryPutAtBottom()),						this,SLOT(transferPutAtBottom()),					Qt::QueuedConnection);
	connect(last,SIGNAL(readStopped()),						this,SLOT(transferIsFinished()),					Qt::QueuedConnection);
	connect(last,SIGNAL(preOperationStopped()),					this,SLOT(doNewActions_start_transfer()),				Qt::QueuedConnection);
	connect(last,SIGNAL(postOperationStopped()),					this,SLOT(transferInodeIsClosed()),					Qt::QueuedConnection);
	connect(last,SIGNAL(checkIfItCanBeResumed()),					this,SLOT(restartTransferIfItCan()),					Qt::QueuedConnection);

	connect(this,SIGNAL(send_sendNewRenamingRules(QString,QString)),		last,SLOT(setRenamingRules(QString,QString)),				Qt::QueuedConnection);

	last->start();
	last->setObjectName(QString("transfer %1").arg(transferThreadList.size()-1));
	last->setMkpathTransfer(&mkpathTransfer);
	last->setRenamingRules(firstRenamingRule,otherRenamingRule);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	last->setId(transferThreadList.size()-1);
	#endif
	if(transferThreadList.size()>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
		return;
	if(stopIt)
		return;
	doNewActions_inode_manipulation();
	emit askNewTransferThread();
}
