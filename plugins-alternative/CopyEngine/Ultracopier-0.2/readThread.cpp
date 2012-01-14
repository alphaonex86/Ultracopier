/***************************************************************************
				readThread.cpp
				-------------------

     Begin	: Fri Dec 21 2007 22:48 alpha_one_x86
     Project	: Ultracopier
     Email	: ultracopier@first-world.info
     Note	 : See README for copyright and developer
     Target	 : Define the class of the copy threads

****************************************************************************/

#include "readThread.h"

#include <QMessageBox>

/*! \todo checksun while copy
 \todo remove folder in move when the counter is 0
 \todo remove from transfer list only when write thread is finish
 \todo stop transfer only when write thread is finish
  */

/*! \brief Initialise the copy thread
*/

readThread::readThread()
{
	writeThreadSem.release(MAXWRITETHREAD);
	waitWriteThread.release();//keep 1 free to acquire and release
	stopIt				= false;
	maxSpeed			= 0;
	blockSizeCurrent		= 1024*1024;
	preallocation			= false;
	keepDate			= false;
	bytesToTransfer			= 0;
	bytesTransfered			= 0;
	idIncrementNumber		= 0;
	currentTransferedSize		= 0;
	currentTransferedSizeOversize	= 0;
	totalTransferListSize		= 0;
	currentPositionInSource		= 0;
	actualRealByteTransfered	= 0;
	needInPause			= false;
	isInPause			= false;
	stopped				= true;
	dialogInWait			= false;

	connect(&clockForTheCopySpeed,	SIGNAL(timeout()),			this,	SLOT(timeOfTheBlockCopyFinished()));
	connect(this,			SIGNAL(started()),			this,	SLOT(pre_operation()));
	connect(this,			SIGNAL(finished()),			this,	SLOT(post_operation()));
	connect(this,			SIGNAL(queryNewWriteThread()),		this,	SLOT(createNewWriteThread()));
	connect(this,			SIGNAL(queryStartWriteThread(int)),	this,	SLOT(startWriteThread(int)));

	#ifdef DEBUG_WINDOW
	theDebugDialog.show();
	updateDialogTimer.setInterval(50);
	updateDialogTimer.start();
	connect(&updateDialogTimer,	SIGNAL(timeout()),			this,	SLOT(updateDebugInfo()));
	isBlocked=false;
	#endif // DEBUG_WINDOW
	setObjectName("Read thread");
}

readThread::~readThread()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopTheTransfer();
	{
		QMutexLocker lock_mutex(&MultiThreadWriteThreadList);
		while(theWriteThreadList.size()>0) {
			theWriteThreadList.first()->stop();
			theWriteThreadList.first()->disconnect();
			delete theWriteThreadList.first();
			theWriteThreadList.removeFirst();
		}
        }
}

void readThread::setDrive(QStringList drives)
{
	mountSysPoint=drives;
}

bool readThread::isFinished()
{
	return stopped;
}

/// \brief For give timer envery X ms
void readThread::timeOfTheBlockCopyFinished()
{
	if(waitNewClockForSpeed.available()<NUMSEMSPEEDMANAGEMENT)
		waitNewClockForSpeed.release();
}

/*! \brief Query for stop the current thread

  For stop in urgence the copy thread, use only with cancel!
\see stopIt, run() and stop()
*/
void readThread::stopTheTransfer()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	/// \note never do the unlock() of mutex here, cause crash because it need by close by the opener thread
	if(stopIt)//drop multiple call
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"multiple calls are detected");
		return;
	}
	stopIt=true;

	if(!this->isFinished())
	{
		QMutexLocker lock_mutex(&MultiThreadWriteThreadList);
		for (int i = 0; i < theWriteThreadList.size(); ++i) {
			theWriteThreadList.at(i)->stop();
		}
		theWriteThreadList.clear();
	}
	for(int i = 0;i<FileExistsActionList.size();++i)
		FileExistsActionList.at(i).waitSemaphore->release();
	for(int i = 0;i<FileErrorActionList.size();++i)
		FileErrorActionList.at(i).waitSemaphore->release();

	while(waitNewClockForSpeed.available()<=0)
		waitNewClockForSpeed.release();

	while(waitErrorAction.available()<=0)
		waitErrorAction.release();

	while(waitFileExistsAction.available()<=0)
		waitFileExistsAction.release();

	while(waitInPause.available()<=0)
		waitInPause.release();

	while(waitWriteThread.available()<=0)
		waitWriteThread.release();

	while(writeThreadSem.available()<=0)
		writeThreadSem.release();

	writeThreadSem.release(MAXWRITETHREAD);
	waitNewWriteThread.release();

	if(!stopped)
        {
                wait();
                for(int i = 0;i<FileExistsActionList.size();++i)
                        delete FileExistsActionList.at(i).waitSemaphore;
                for(int i = 0;i<FileErrorActionList.size();++i)
                        delete FileErrorActionList.at(i).waitSemaphore;
                FileExistsActionList.clear();
                FileErrorActionList.clear();
        }
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

void readThread::setFileExistsAction(FileExistsAction action,QString newName)
{
	if(action==FileExists_Rename)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action: "+QString::number(action)+", newName: "+newName);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action: "+QString::number(action));
	dialogInWait=false;
	this->newName=newName;
	if(action==FileExists_Cancel)
		stop();
	else
	{
		QMutexLocker lock_mutex(&dialogActionMutex);
		*(FileExistsActionList.first().action)=action;
		FileExistsActionList.first().waitSemaphore->release();
                waitCanDestroyGuiSemaphore.acquire();
		delete FileExistsActionList.first().waitSemaphore;
		FileExistsActionList.removeFirst();
	}
	//out of the previous {} else the mutex is double locked and freeze
	emitSignalToShowDialog();
}

void readThread::setFileErrorAction(FileErrorAction action)
{
	dialogInWait=false;
	if(action==FileError_Cancel)
		stop();
	else
	{
		QMutexLocker lock_mutex(&dialogActionMutex);
		*(FileErrorActionList.first().action)=action;
		FileErrorActionList.first().waitSemaphore->release();
                waitCanDestroyGuiSemaphore.acquire();
		delete FileErrorActionList.first().waitSemaphore;
		FileErrorActionList.removeFirst();
	}
	//out of the previous {} else the mutex is double locked and freeze
	emitSignalToShowDialog();
}

void readThread::setFileErrorActionInWriteThread(FileErrorAction action,int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"write thread reply: "+QString::number(action)+", for the index: "+QString::number(index));
	dialogInWait=false;
	if(action==FileError_Cancel)
		stop();
	else
	{
		{
			QMutexLocker lock_mutex(&dialogActionMutex);
			theWriteThreadList.at(index)->errorAction(action);
			WriteThreadErrorList.removeFirst();
			if(WriteThreadErrorList.size()==0)
				waitWriteThread.release();
		}
		//out of the previous {} else the mutex is double locked and freeze
		emitSignalToShowDialog();
	}
}

void readThread::emitSignalToShowDialog()
{
	if(dialogInWait)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"dialog is already displayed");
		return;
	}
	{
		QMutexLocker lock_mutex(&dialogActionMutex);
		if(WriteThreadErrorList.size()>0)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog WriteThreadErrorList");
			dialogInWait=true;
			emit errorOnFileInWriteThread(WriteThreadErrorList.first().fileInfo,WriteThreadErrorList.first().errorString,WriteThreadErrorList.first().index);
		}
		else if(FileExistsActionList.size()>0)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog FileExistsActionList");
			dialogInWait=true;
			emit fileAlreadyExists(FileExistsActionList.first().fileInfoSource,FileExistsActionList.first().fileInfoDestination,FileExistsActionList.first().isSame);
		}
		else if(FileErrorActionList.size()>0)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"show dialog FileErrorActionList");
			dialogInWait=true;
			emit errorOnFile(FileErrorActionList.first().fileInfo,FileErrorActionList.first().errorString,FileErrorActionList.first().canPutAtTheEndOfList);
		}
	}
}

void readThread::manageTheWriteThreadError(QFileInfo fileInfo,QString errorString)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"on file: "+fileInfo.absoluteFilePath()+", error: "+errorString);
	QObject * senderThread = sender();
	int index=0;
	while(index<theWriteThreadList.size())
	{
		if(theWriteThreadList.at(index)==senderThread)
		{
			WriteThreadErrorStruct newItem;
			newItem.errorString	= errorString;
			newItem.fileInfo	= fileInfo;
			newItem.index		= index;
			if(theWriteThreadList.at(index)->getStatus()!=writeThread::Running)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"bug because the write thread is not copying");
				return;
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"this write thread has emitted the error: "+QString::number(index));
			{
				QMutexLocker lock_mutex(&dialogActionMutex);
				WriteThreadErrorList << newItem;
			}
			if(WriteThreadErrorList.size()==1) // if is newly >0
				waitWriteThread.acquire();
			emitSignalToShowDialog();
			return;
		}
		index++;
	}
}

//should never return the code NotSet
FileExistsAction readThread::waitNeedFileExistsAction(QFileInfo fileInfoSource,QFileInfo fileInfoDestination,bool isSame)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start()");
	FileExistsAction fileExistsActionSuspended;
	FileExistsActionStruct newItem;
	newItem.fileInfoSource		= fileInfoSource;
	newItem.fileInfoDestination	= fileInfoDestination;
	newItem.isSame			= isSame;
	newItem.waitSemaphore		= new QSemaphore();
	newItem.action			= &fileExistsActionSuspended;
	{
		QMutexLocker lock_mutex(&dialogActionMutex);
		FileExistsActionList << newItem;
	}
	emitSignalToShowDialog();
	newItem.waitSemaphore->acquire();
        waitCanDestroyGuiSemaphore.release();
	if(fileExistsActionSuspended==FileExists_Skip)
		skipThecurrentFile=true;
	return fileExistsActionSuspended;
}

//should never return the code NotSet
FileErrorAction readThread::waitNeedFileErrorAction(QFileInfo fileInfo,QString errorString,bool canPutAtTheEndOfList)
{
	FileErrorAction	fileErrorActionSuspended;
	FileErrorActionStruct newItem;
	newItem.fileInfo		= fileInfo;
	newItem.errorString		= errorString;
	newItem.canPutAtTheEndOfList	= canPutAtTheEndOfList;
	newItem.waitSemaphore		= new QSemaphore();
	newItem.action			= &fileErrorActionSuspended;
	{
		QMutexLocker lock_mutex(&dialogActionMutex);
		FileErrorActionList << newItem;
	}
	emitSignalToShowDialog();
	newItem.waitSemaphore->acquire();
        waitCanDestroyGuiSemaphore.release();
	if(fileErrorActionSuspended==FileError_Skip)
		skipThecurrentFile=true;
	return fileErrorActionSuspended;
}

/// \brief check if need wait write thread
void readThread::checkIfNeedWaitWriteThread()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"WriteThreadErrorList.size(): "+QString::number(WriteThreadErrorList.size()));
	waitWriteThread.acquire();
	waitWriteThread.release();
}

/// \brief create new write thread
void readThread::createNewWriteThread()
{
	theNewCreatedWriteThread=new writeThread(this);
	theNewCreatedWriteThread->start(this->priority());
	{
		QMutexLocker lock_mutex(&MultiThreadWriteThreadList);
		theWriteThreadList.append(theNewCreatedWriteThread);
	}
	qRegisterMetaType<DebugLevel>("DebugLevel");
	connect(theNewCreatedWriteThread,SIGNAL(finished()),							this,	SLOT(newWriteThreadFinish()));
	connect(theNewCreatedWriteThread,SIGNAL(errorOnFile(QFileInfo,QString)),				this,	SLOT(manageTheWriteThreadError(QFileInfo,QString)));
	connect(theNewCreatedWriteThread,SIGNAL(haveFinishFileOperation()),					this,	SLOT(writeThreadOperationFinish()));
	connect(theNewCreatedWriteThread,SIGNAL(haveStartFileOperation()),					this,	SLOT(writeThreadOperationStart()));
	connect(theNewCreatedWriteThread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),	this,	SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	waitNewWriteThread.release();
}

void readThread::startWriteThread(int index)
{
	theWriteThreadList.at(index)->start();
	waitNewWriteThread.release();
}

void readThread::writeThreadOperationStart()
{
	writeThreadSem.acquire();
}

void readThread::writeThreadOperationFinish()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");

	QObject * senderObject=sender();
	if(senderObject==NULL)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Qt sender() NULL");
	else
	{
		int currentIndex=-1;
                quint64 currentId=0;
		returnActionOnCopyList newAction;
                CopyMode mode=Copy;
		QDir sourceFolder;
		{
			QMutexLocker lock_mutex(&MultiThreadTransferList);
			int index=0;
			while(index<listTransfer.size())
			{
				if(listTransfer.at(index).thread==senderObject)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the thread ("+QString::number(index)+") is the sender");
					currentIndex=index;
					break;
				}
				index++;
			}
			if(currentIndex!=-1)
			{
				newAction.type=OtherAction;
				newAction.userAction.id=listTransfer.at(index).id;
				currentId=listTransfer.at(index).id;
				mode=listTransfer.at(index).mode;
				newAction.userAction.type=RemoveItem;
				sourceFolder=listTransfer.at(index).source.absoluteDir();
				actionDone << newAction;
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listTransfer.removeAt("+QString::number(currentIndex)+"), with id: "+QString::number(listTransfer.at(index).id)+", with source: "+listTransfer.at(index).source.absoluteFilePath());
				listTransfer.removeAt(currentIndex);
			}
		}
		if(currentIndex!=-1)//put in write thread
		{
			emit newTransferStop(currentId);
			if(mode==Move)
				removeFolderIfNeeded(sourceFolder);
		}
		else
		{
			QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and copy plugin. Please report this bug."));
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Sender not located in the list");//have crash here after multiple write thread error
		}
	}

	if(writeThreadSem.available()>MAXWRITETHREAD)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"The authorised max thread is not limited! Internal bug.");
	writeThreadSem.release();
	if(writeThreadIsFinish() && stopped)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"write thread is finished");
		emit transferIsStarted(false);
	}
}

//insert at end without count
/*void readThread::insterAtEndWithoutCount(copyItemInternal theItem)
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	NumberOfFileCopied--;
	firstScanCur-=theItem.size;
	listTransfer.append(theItem);
}*/

QString readThread::getDrive(QString fileOrFolder)
{
	for (int i = 0; i < mountSysPoint.size(); ++i) {
		if(fileOrFolder.startsWith(mountSysPoint.at(i)))
			return mountSysPoint.at(i);
	}
	//if unable to locate the right mount point
	return "";
}

//pause the copy
void readThread::pauseTransfer()
{
	if(isInPause)
		//alpha_one_x86: possibility to be where, I don't know why
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Copy thread not running");
	needInPause=true;
}

//resume the copy
void readThread::resumeTransfer()
{
	needInPause=false;
	if(isInPause)
	{
		//this line is uncommented because, the resume not work, repported by user
		//should stay uncomment, if bug fix it by other ways
		waitInPause.release();
	}
	else
	{
		if(this->isFinished())
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Copy thread stopped, cannot be resumed");
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Copy thread not in pause, is running?");
	}
}

//skip the current file
void readThread::skipCurrentTransfer(quint64 id)
{
	removeItemInTransferList(id,true);
}

void readThread::removeItemInTransferList(quint64 id,bool skipIfIsRunning,bool checkWriteThread)
{
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		int index=0;
		while(index<listTransfer.size())
		{
			if(listTransfer.at(index).id==id)
			{
				if(checkWriteThread && listTransfer.at(index).thread!=NULL)
				{
					if(skipIfIsRunning)
					{
						if(currentReadTransferId==id && !this->isFinished())
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"this transfer thread is running, then skip it");
							stopIt=true;
							skipThecurrentFile=true;
						}
						else
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"not reading because the current id is: "+QString::number(currentReadTransferId)+", then skip write thread");
							listTransfer.at(index).thread->skipTheCurrentFile();
						}
					}
					else
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"this transfer is running, then the \"remove order\" is dropped");
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"this transfer is not running, just remove of the list");
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=listTransfer.at(index).id;
					newAction.userAction.type=RemoveItem;
					actionDone << newAction;
					listTransfer.removeAt(index);
					emit newActionOnList();
				}
				return;
			}
			index++;
		}
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"this id is not found: "+QString::number(id));
	}
}

//warning the first entry is accessible will copy
void readThread::removeItems(QList<int> ids)
{
	for(int i=0;i<ids.size();i++)
		removeItemInTransferList(ids.at(i),false);
}

//put on top
void readThread::moveItemsOnTop(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	bool haveChanged=false;
	//do list operation
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		if(ids.size()==0 || listTransfer.size()<=1)
			return;
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) {
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," before argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id));
		}
		#endif
		int indexToMove=0;
		for (int i=0; i<listTransfer.size(); ++i) {
			if(ids.indexOf(listTransfer.at(i).id)!=-1)
			{
				ids.removeOne(listTransfer.at(i).id);
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(indexToMove));
				returnActionOnCopyList newAction;
				newAction.type=OtherAction;
				newAction.userAction.id=listTransfer.at(i).id;
				newAction.userAction.type=MoveItem;
				newAction.position=indexToMove;
				actionDone << newAction;
				listTransfer.move(i,indexToMove);
				indexToMove++;
				haveChanged=true;
			}
		}
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) {
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," after argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id));
		}
		#endif
	}
	if(haveChanged)
		emit newActionOnList();
}

//move up
void readThread::moveItemsUp(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	bool haveChanged=false;
	//do list operation
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		if(ids.size()==0 || listTransfer.size()<=1)
			return;
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," before argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
		int first=1;
		bool entryIsMovable=(ids.indexOf(listTransfer.at(first-1).id)==-1);
		for (int i=first; i<listTransfer.size(); ++i) {
			if(ids.indexOf(listTransfer.at(i).id)!=-1)
			{
				if(entryIsMovable)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i-1));
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=listTransfer.at(i).id;
					newAction.userAction.type=MoveItem;
					newAction.position=i-1;
					actionDone << newAction;
					listTransfer.swap(i,i-1);
					haveChanged=true;
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
				}
				ids.removeOne(listTransfer.at(i).id);
			}
			else
			{
				entryIsMovable=true;
			}
		}
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," after argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
	}
	if(haveChanged)
		emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//move down
void readThread::moveItemsDown(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	bool haveChanged=false;
	//do list operation
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		if(ids.size()==0 || listTransfer.size()<=1)
			return;
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," before argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
		int first=listTransfer.size()-1;
		bool entryIsMovable=(ids.indexOf(listTransfer.at(listTransfer.size()-1).id)==-1);
		for (int i=first; i>=0; --i) {
			if(ids.indexOf(listTransfer.at(i).id)!=-1)
			{
				if(entryIsMovable)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i+1));
					returnActionOnCopyList newAction;
					newAction.type=OtherAction;
					newAction.userAction.id=listTransfer.at(i).id;
					newAction.userAction.type=MoveItem;
					newAction.position=i+1;
					actionDone << newAction;
					listTransfer.swap(i,i+1);
					haveChanged=true;
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
				}
				ids.removeOne(listTransfer.at(i).id);
			}
			else
				entryIsMovable=true;
		}
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," after argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
	}
	if(haveChanged)
		emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

//put on bottom
void readThread::moveItemsOnBottom(QList<int> ids)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	bool haveChanged=false;
	//do list operation
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		if(ids.size()==0 || listTransfer.size()<=1)
			return;
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," before argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
		int onTop=0;
		int indexToMove=listTransfer.size()-1;
		for (int i=listTransfer.size()-1; i>=onTop; --i) {
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Check action on item ")+QString::number(i));
			if(ids.indexOf(listTransfer.at(i).id)!=-1)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(indexToMove));
				ids.removeOne(listTransfer.at(i).id);
				returnActionOnCopyList newAction;
				newAction.type=OtherAction;
				newAction.userAction.id=listTransfer.at(i).id;
				newAction.userAction.type=MoveItem;
				newAction.position=indexToMove;
				actionDone << newAction;
				listTransfer.move(i,indexToMove);
				indexToMove--;
				haveChanged=true;
			}
		}
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		for(int i=0; i<listTransfer.size(); ++i) { ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice," after argument["+QString::number(i)+"]: "+QString::number(listTransfer.at(i).id)); }
		#endif
	}
	if(haveChanged)
		emit newActionOnList();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop");
}

/** \brief set block size
\param block the new block size in KB
\return Return true if succes */
bool readThread::setBlockSize(const int blockSize)
{
	if(blockSize<1 || blockSize>16384)
	{
		blockSizeCurrent=blockSize*1024;
		//set the new max speed because the timer have changed
		setMaxSpeed(maxSpeed);
		return true;
	}
	else
		return false;
}

/** \brief set keep date
\param value The value setted */
void readThread::setKeepDate(const bool value)
{
	keepDate=value;
}

quint64 readThread::generateIdNumber()
{
	idIncrementNumber++;
	if(idIncrementNumber>(((quint64)1024*1024)*1024*1024*2))
		idIncrementNumber=0;
	return idIncrementNumber;
}

//append to action/transfer list
quint64 readThread::addToMkPath(const QDir& folder)
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	toMkPath temp;
	temp.id		= generateIdNumber();
	temp.path	= folder;
	listMkPath << temp;
	return temp.id;
}

quint64 readThread::addToTransfer(const QFileInfo& source,const QFileInfo& destination,const CopyMode& mode)
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	toTransfer temp;
	temp.id		= generateIdNumber();
	temp.size	= source.size();
	temp.source	= source;
	temp.destination= destination;
	temp.mode	= mode;
	temp.thread	= NULL;
	listTransfer << temp;
	totalTransferListSize+=temp.size;
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

bool readThread::haveContent()
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	return (listTransfer.size()>0);
}

void readThread::addToRmPath(const QDir& folder,const int& inodeToRemove)
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	toRmPath temp;
	temp.remainingInode	= inodeToRemove;
	temp.path		= folder;
	temp.id			= generateIdNumber();
	temp.movedToEnd		= false;
	listRmPath << temp;
}

int readThread::lenghtOfCopyList()
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	return listTransfer.size();
}

/** \brief set prealoc file size
\param prealloc True if should be prealloc */
void readThread::setPreallocateFileSize(const bool prealloc)
{
	preallocation=prealloc;
}

/// \brief For give timer every X ms
/*void readThread::timeOfTheBlockCopyFinished()
{
	if(waitNewClockForSpeed.available()<NUMSEMSPEEDMANAGEMENT)
		waitNewClockForSpeed.release();
}*/

/*! \brief Set the max speed
\param tempMaxSpeed Set the max speed in KB/s, 0 for no limit */
void readThread::setMaxSpeed(const int tempMaxSpeed)
{
	if(tempMaxSpeed>=0)
	{
		if(maxSpeed==0 && tempMaxSpeed==0 && waitNewClockForSpeed.available()>0)
			waitNewClockForSpeed.tryAcquire(waitNewClockForSpeed.available());
		maxSpeed=tempMaxSpeed;
		if(maxSpeed>0)
		{
			int NewInterval,newMultiForBigSpeed=0;
			do
			{
				newMultiForBigSpeed++;
				NewInterval=(blockSizeCurrent*newMultiForBigSpeed)/(maxSpeed);
			}
			while (NewInterval<MINTIMERINTERVAL);
			if(NewInterval>MAXTIMERINTERVAL)
			{
				NewInterval=MAXTIMERINTERVAL;
				newMultiForBigSpeed=1;
				blockSizeCurrent=maxSpeed*NewInterval;
			}
			MultiForBigSpeed=newMultiForBigSpeed;
				clockForTheCopySpeed.setInterval(NewInterval);
			if(!clockForTheCopySpeed.isActive() && !this->isFinished())
				clockForTheCopySpeed.start();
		}
		else
		{
			if(clockForTheCopySpeed.isActive())
				clockForTheCopySpeed.stop();
			waitNewClockForSpeed.release();
		}
	}
}

void readThread::putFirstFileAtEnd()
{
	QMutexLocker lock_mutex(&MultiThreadTransferList);
	listTransfer << listTransfer.first();
	stopIt=true;
	skipThecurrentFile=true;
}

/*! \brief Run the copy
\warning The pointer sould be given and thread initilised
\see QObjectOfMwindow(), stopTheTransfer() and stop()
*/
void readThread::run()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopped = false;
	while((listMkPath.size()>0 || listTransfer.size()>0) && !stopIt)
	{
		quint64 realTransferId=0;
		int listMkPathSize=0;
		bool waitNewTransfer=false;
		{
			QMutexLocker lock_mutex(&MultiThreadTransferList);
			int index=0;
			while(index<listTransfer.size())
			{
				if(listTransfer.at(index).thread==NULL)
				{
					waitNewTransfer=true;
					realTransferId=listTransfer.at(index).id;
					break;
				}
				index++;
			}
			listMkPathSize=listMkPath.size();
		}
		if(listMkPathSize>0)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listMkPath.size(): "+QString::number(listMkPathSize)+", id: "+QString::number(listMkPath.first().id));
		if(waitNewTransfer)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listTransfer.size(): "+QString::number(listTransfer.size())+", wait new transfer id: "+QString::number(realTransferId));
		if(listRmPath.size()>0)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"listRmPath.size(): "+QString::number(listRmPath.size())+", id: "+QString::number(listRmPath.first().id));
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"do the next action");
		if(stopIt)
			return;
		//put the list transfer
		if(listRmPath.size()>0 && (!waitNewTransfer || listRmPath.first().id<realTransferId) && (listMkPathSize==0 || listRmPath.first().id<listMkPath.first().id))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start rm path");
			emit rmPath(listRmPath.first().path.absolutePath());
			if(!removeFolderIfNeeded(listRmPath.first().path,false))
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to remove the folder");
			QMutexLocker lock_mutex(&MultiThreadTransferList);
			if(listRmPath.first().remainingInode!=0)
			{
				if(listRmPath.first().movedToEnd)
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Folder already put at the end, drop it to prevent infinite loop");
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Put at the end for future removal: "+listRmPath.first().path.absolutePath());
					listRmPath << listRmPath.first();
					listRmPath.last().movedToEnd=true;
					listRmPath.last().id=generateIdNumber();
				}
			}
			listRmPath.removeFirst();
		}
		else if(waitNewTransfer && (listMkPath.size()==0 || realTransferId<listMkPath.first().id))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start transfer");
			checkIfNeedWaitWriteThread();

			skipThecurrentFile			= false;
			stopIt					= false;
			int bytesWriten				= 0;
			theNewCreatedWriteThread		= NULL;
			QByteArray blockArray;
			QFile sourceFile;
			QFile destinationFile;
			QFileInfo sourceFileInfo;
			QFileInfo destinationFileInfo;
                        CopyMode mode=Copy;
                        quint64 size=0;
                        quint64 id=0;
			int listSize=0;
			int blockArraySize=0;
			ItemOfCopyList newItemToSend;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start stop or pause macro");
			STOPORPAUSEMACROPREOPEN;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"check if need wait write thread");
			checkIfNeedWaitWriteThread();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"load the file name");

			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try acquire write thread");
			writeThreadSem.acquire();
			writeThreadSem.release();
			STOPORPAUSEMACROPREOPEN;
			{
				//search write thread free
				QMutexLocker lock_mutex(&MultiThreadWriteThreadList);
				for (int i=0;i<theWriteThreadList.size();++i) {
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"The thread id "+QString::number(i)+": "+QString::number(theWriteThreadList.at(i)->getStatus()));
					if(theWriteThreadList.at(i)->getStatus()!=writeThread::Running)
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Use the thread id "+QString::number(i));
						if(theWriteThreadList.at(i)->getStatus()!=writeThread::WaitingTask)
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Recycle previous allocated write thread");
						//if have been closed by previous copy list finish restart it
						if(theWriteThreadList.at(i)->isFinished())
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"The thread is finished but still in the list! Start it!");
							//use signal/slot and mutex for create it in main thread
							emit queryStartWriteThread(i);
							if(stopIt)
								return;
							waitNewWriteThread.acquire();
						}
						//set the pointer on it for use it
						theNewCreatedWriteThread=theWriteThreadList.at(i);
						break;
					}
				}
			}
			//check if no other thread is in error
			checkIfNeedWaitWriteThread();
			if(stopIt)
				return;
			//if no existing thread is loaded, create it
			if(theNewCreatedWriteThread==NULL)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"create write thread");
				//use signal/slot and mutex for create it in main thread
				emit queryNewWriteThread();
				waitNewWriteThread.acquire();
			}
			STOPORPAUSEMACROPREOPEN;
			if(theNewCreatedWriteThread->isFinished())
			{
				stopIt=true;
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Existing write thread not running! (copy engine plugin)");
				goto SkipFile;
			}

			//load info from list
			{
				QMutexLocker lock_mutex(&MultiThreadTransferList);
				int index=0;
				while(index<listTransfer.size())
				{
					if(listTransfer.at(index).id==realTransferId)
					{
						sourceFileInfo=listTransfer.at(index).source;
						destinationFileInfo=listTransfer.at(index).destination;
						sourceFile.setFileName(sourceFileInfo.absoluteFilePath());
						destinationFile.setFileName(destinationFileInfo.absoluteFilePath());
						mode=listTransfer.at(index).mode;
						size=listTransfer.at(index).size;
						id=listTransfer.at(index).id;
						listSize=listTransfer.size();
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"set write thread for list at index: "+QString::number(index)+", for the file: "+listTransfer.at(index).source.absoluteFilePath());
						listTransfer[index].thread=theNewCreatedWriteThread;
						break;
					}
					index++;
				}
			}

			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"sourceFileInfo.fileName(): "+sourceFileInfo.absoluteFilePath());
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"destinationFileInfo.fileName(): "+destinationFileInfo.absoluteFilePath());

			currentReadTransferId=id;
			newItemToSend.id=id;
			newItemToSend.sourceFullPath=sourceFileInfo.absoluteFilePath();
			newItemToSend.sourceFileName=sourceFileInfo.fileName();
			newItemToSend.destinationFullPath=destinationFileInfo.absoluteFilePath();
			newItemToSend.destinationFileName=destinationFileInfo.fileName();
			newItemToSend.size=size;
			newItemToSend.mode=mode;
			emit newTransferStart(newItemToSend);

			STOPORPAUSEMACROPREOPEN;

			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"check if file is not same");
			/////////// start FS check /////////////
			if(sourceFile.fileName()==destinationFile.fileName())
			{
				switch(waitNeedFileExistsAction(sourceFile,destinationFile,true))
				{
					case FileExists_Rename:
						if(newName=="")
							destinationFile.setFileName(QFileInfo(destinationFile.fileName()).absolutePath()+QDir::separator()+tr("Copy of ")+QFileInfo(destinationFile.fileName()).fileName());
						else
							destinationFile.setFileName(QFileInfo(destinationFile.fileName()).absolutePath()+QDir::separator()+newName);
					break;
					default:
						goto SkipFile;
				}
			}

			STOPORPAUSEMACROPREOPEN;

			//check if destination exists
			CheckIfExists_Move:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"check if destination exists: "+destinationFile.fileName());
			if(destinationFile.exists())
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the destination exists");
				switch(waitNeedFileExistsAction(sourceFile,destinationFile))
				{
					case FileExists_Rename:
						if(newName=="")
							destinationFile.setFileName(QFileInfo(destinationFile.fileName()).absolutePath()+QDir::separator()+tr("Copy of ")+QFileInfo(destinationFile.fileName()).fileName());
						else
							destinationFile.setFileName(QFileInfo(destinationFile.fileName()).absolutePath()+QDir::separator()+newName);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"rename the destination: "+destinationFile.fileName());
						goto CheckIfExists_Move;
					break;
					case FileExists_OverwriteIfNewer:
						if(QFileInfo(sourceFile).lastModified()<QFileInfo(destinationFile).lastModified())
							goto SkipFile;
						else
							break;
					case FileExists_OverwriteIfNotSameModificationDate:
						if(sourceFile.size()==destinationFile.size() || QFileInfo(sourceFile).lastModified()==QFileInfo(destinationFile).lastModified())
							goto SkipFile;
						else
							break;
					case FileExists_Overwrite:
					break;
					default:
						goto SkipFile;
				}
			}
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"destination ok");
			STOPORPAUSEMACROPREOPEN;

			//move if on same mount point
			#if defined (Q_OS_LINUX) || defined (Q_OS_WIN32)
			if(mode==Move && getDrive(destinationFile.fileName())==getDrive(sourceFile.fileName()) && mountSysPoint.size()>0)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Move under windows/linux");
				retryMoveRealRemoveDestination:
				if(destinationFile.exists())
					if(!destinationFile.remove())
					{
						switch(waitNeedFileErrorAction(destinationFile,tr("Unable to remove the destination file: ")+destinationFile.errorString(),(listSize>1)))
						{
							case FileError_Retry:
								goto retryMoveRealRemoveDestination;
							case FileError_Skip:
								skipCurrentTransfer(id);
							case FileError_PutToEndOfTheList:
								putFirstFileAtEnd();
							default:
								goto SkipFile;
						}
					}
				retryMoveReal:
				if(!sourceFile.rename(destinationFile.fileName()))
				{
					switch(waitNeedFileErrorAction(destinationFile,tr("Unable to move the file: ")+sourceFile.errorString(),(listSize>1)))
					{
						case FileError_Retry:
							goto retryMoveReal;
						case FileError_Skip:
							skipCurrentTransfer(id);
						case FileError_PutToEndOfTheList:
							putFirstFileAtEnd();
						default:
							goto SkipFile;
					}
				}
				//file moved skip to the next file
				goto SkipFile;
			}
			STOPORPAUSEMACROPREOPEN;
			#endif

			if(QFileInfo(sourceFile).isSymLink())
			{
				if(destinationFile.exists())
				{
					retrySymLinkDestRemove:
					if(!destinationFile.remove())
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"error while removing the symLink : "+destinationFile.errorString());
						switch(waitNeedFileErrorAction(destinationFile,destinationFile.errorString(),0))
						{
							case FileError_Retry:
								goto retrySymLinkDestRemove;
							case FileError_Skip:
								skipCurrentTransfer(id);
							case FileError_PutToEndOfTheList:
								putFirstFileAtEnd();
								goto SkipFile;
							default:
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unknow action at SymLink");
								goto SkipFile;
						}
					}
				}
				retrySymLink:
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"QFileInfo(\""+QFileInfo(sourceFile).absoluteFilePath()+"\").symLinkTarget(): "+QFileInfo(sourceFile).symLinkTarget());
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"destinationFile: "+destinationFile.fileName());
				if(!QFile::link(QFileInfo(sourceFile).symLinkTarget(),destinationFile.fileName()))
				{
					//QFileInfo(sourceFile).symLinkTarget()
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"error from the symLinkTarget: "+destinationFile.errorString());
					switch(waitNeedFileErrorAction(destinationFile,destinationFile.errorString(),0))
					{
						case FileError_Retry:
							goto retrySymLink;
						case FileError_Skip:
							skipCurrentTransfer(id);
						case FileError_PutToEndOfTheList:
							putFirstFileAtEnd();
							goto SkipFile;
						default:
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unknow action from mkpath");
							goto SkipFile;
					}
				}
			}
			else
			{

				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Open the file source");
				/** \note symbolic link support drop because incomplete support into Qt (unable to not resolve the target) */
				//try open source is read only for copy and read/write for move
				retryOpenSource:
				if(!sourceFile.open(QIODevice::ReadOnly))
				{
					switch(waitNeedFileErrorAction(destinationFile,tr("Unable to open the source file: ")+sourceFile.errorString(),(listSize>1)))
					{
						case FileError_Retry:
							goto retryOpenSource;
						case FileError_Skip:
							skipCurrentTransfer(id);
						case FileError_PutToEndOfTheList:
							putFirstFileAtEnd();
						default:
							goto SkipFile;
					}
				}
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"source file opened: "+sourceFile.fileName());

				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try opening the destination");
				//for the current thread selected, set the actual options
				theNewCreatedWriteThread->setKeepDate(keepDate);
				theNewCreatedWriteThread->setPreallocation(preallocation);
				theNewCreatedWriteThread->setFiles(sourceFile.fileName(),destinationFile.fileName());
				theNewCreatedWriteThread->setMovingMode(mode);

				retryOpenDestination:
				if(theNewCreatedWriteThread->openDestination()!="")
				{
					switch(waitNeedFileErrorAction(destinationFile,tr("Unable to open the destination file: ")+sourceFile.errorString(),(listSize>1)))
					{
						case FileError_PutToEndOfTheList:
							putFirstFileAtEnd();
							goto SkipFile;
						case FileError_Retry:
							goto retryOpenDestination;
						case FileError_Skip:
							skipCurrentTransfer(id);
						default:
							goto SkipFile;
					}
				}

				STOPORPAUSEMACRO;

				numberOfBlockCopied	= 0;
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the copy");
				do
				{
					STOPORPAUSEMACRO;

					//read one block
					retryReadThisBlock:
					#ifdef DEBUG_WINDOW
					isBlocked=true;
					#endif // DEBUG_WINDOW
					blockArray=sourceFile.read(blockSizeCurrent);
					#ifdef DEBUG_WINDOW
					isBlocked=false;
					#endif // DEBUG_WINDOW

					STOPORPAUSEMACRO;

					if(sourceFile.error()!=QFile::NoError)
					{
						QString errorString=tr("Unable to read the source file: ")+sourceFile.errorString()+" ("+QString::number(sourceFile.error())+")";
						retrySeekSource:
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"read at read: "+sourceFile.errorString()+" ("+QString::number(sourceFile.error())+")");
						switch(waitNeedFileErrorAction(destinationFile,errorString,(listSize>1)))
						{
							case FileError_Retry:
								//reopen if the mount point or device is hard removed, because all descriptor is wrong
								//bug in Qt 4.7
								sourceFile.close();
								if(!sourceFile.open(QIODevice::ReadOnly))
								{
									errorString=tr("Unable to re-open the source file: %1 (%2)").arg(sourceFile.errorString()).arg(sourceFile.error());
									goto retrySeekSource;
								}
								if(!sourceFile.seek(currentPositionInSource))
								{
									errorString=tr("Unable to go to the right possition into the source file: %1 (%2)").arg(sourceFile.errorString()).arg(sourceFile.error());
									goto retrySeekSource;
								}
								goto retryReadThisBlock;
							case FileError_Skip:
								skipCurrentTransfer(id);
							case FileError_PutToEndOfTheList:
								putFirstFileAtEnd();
							default:
								goto SkipFile;
						}
					}
					blockArraySize=blockArray.size();
					actualRealByteTransfered+=blockArraySize;
					currentPositionInSource+=blockArraySize;

					STOPORPAUSEMACRO;

					if(!blockArray.isEmpty()) //write one block
					{
						bytesWriten=blockArray.size();
						theNewCreatedWriteThread->addNewBlock(blockArray);
					}

					STOPORPAUSEMACRO;

					//wait for limitation speed if stop not query
					if(maxSpeed>0)
					{
						numberOfBlockCopied++;
						if(numberOfBlockCopied>=MultiForBigSpeed)
						{
							numberOfBlockCopied=0;
							waitNewClockForSpeed.acquire();
						}
					}
					if(currentPositionInSource>size)
						currentTransferedSizeOversize=currentPositionInSource-size;
				}
				while(!blockArray.isEmpty() && bytesWriten==blockArray.size() && !stopIt);
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"currentPositionInSource: "+QString::number(currentPositionInSource));

				STOPORPAUSEMACRO;

				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"endOfSourceDetected() start");
				theNewCreatedWriteThread->endOfSourceDetected();
			}

	/////////////////////////////////
	SkipFile:
	/////////////////////////////////
			sourceFile.close();

			if(skipThecurrentFile)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"skip the current file");
				theNewCreatedWriteThread->skipTheCurrentFile();
				stopIt=false;
				skipThecurrentFile=false;
				removeItemInTransferList(id,false,false);
			}
			{
				QMutexLocker lock_mutex(&MultiThreadTransferList);
				if(listTransfer.size())
				{
					totalTransferListSize+=currentTransferedSizeOversize;
					currentTransferedSize+=size+currentTransferedSizeOversize;
				}
			}
			if(mode==Move)
			{
				int index=0;
				while(index<listRmPath.size())
				{
					if(listRmPath.at(index).path==sourceFileInfo.absoluteDir())
					{
						listRmPath[index].remainingInode--;
						break;
					}
					index++;
				}
			}
			currentPositionInSource=0;
			currentTransferedSizeOversize=0;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"to the next file");
		}
		else if(listMkPathSize>0)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start mkpath");
			QDir path;/// \bug crash here at force close
			//load info from list
			{
				QMutexLocker lock_mutex(&MultiThreadTransferList);
				path=listMkPath.first().path;
			}
			emit mkPath(path.absolutePath());
			if(!path.exists())
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try mkdir: "+path.absolutePath());
				retryMkpath:
				if(!path.mkpath(path.absolutePath()))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"mkdir command could not be performed");
					switch(waitNeedFileErrorAction(path.absolutePath(),tr("Error while creating destination folder"),false))
					{
						case FileError_Retry:
							goto retryMkpath;
						default:
						break;
					}
				}
			}
			{
				QMutexLocker lock_mutex(&MultiThreadTransferList);
				listMkPath.removeFirst();
			}
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"the list of actions is empty");
			break;
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the transfer is finished");
	stopped = true;
}

bool readThread::removeFolderIfNeeded(QDir basePath,bool needDecreaseNumberOfInode)
{
	int index=0;
	int size=listRmPath.size();
	while(index<size)
	{
		if(listRmPath.at(index).path==basePath)
		{
			if(needDecreaseNumberOfInode)
			{
				QMutexLocker lock_mutex(&MultiThreadTransferList);
				listRmPath[index].remainingInode--;
			}
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			if(needDecreaseNumberOfInode)
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"basePath: "+basePath.absolutePath()+" remaining inode to remove: "+QString::number(listRmPath[index].remainingInode));
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"basePath: "+basePath.absolutePath()+", if empty remove it, remaining inode to remove: "+QString::number(listRmPath[index].remainingInode));
			#endif // ULTRACOPIER_DEBUG
			if(listRmPath.at(index).remainingInode==0)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Remove the folder to move: "+listRmPath.at(index).path.absolutePath());
				if(!listRmPath.at(index).path.rmpath(listRmPath.at(index).path.absolutePath()))
				{
					#ifdef ULTRACOPIER_PLUGIN_DEBUG
					QFileInfoList entryList=listRmPath.at(index).path.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);//possible wait time here
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to remove the foler: "+listRmPath.at(index).path.absolutePath()+", with "+QString::number(entryList.size())+" inode(s)");
					#endif // ULTRACOPIER_DEBUG
				}
				QDir parent=listRmPath.at(index).path;//load it before the remove else crash because list is empty
				if(needDecreaseNumberOfInode)
				{
					QMutexLocker lock_mutex(&MultiThreadTransferList);
					listRmPath.removeAt(index);
				}
				parent.cdUp();
				removeFolderIfNeeded(parent);
				return true;
			}
			return true;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to found the foler: "+basePath.absolutePath());
	return false;
}

/*! \brief Stop the copy thread
\see stopIt, stopTheTransfer() and run()
*/
void readThread::stop()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopTheTransfer();
}

void readThread::waitInPauseIfNeeded()
{
	if(!stopIt && needInPause)
	{
		isInPause=true;
		emit transferIsInPause(isInPause);
		waitInPause.acquire();
		isInPause=false;
		emit transferIsInPause(isInPause);
	}
}

QList<returnActionOnCopyList> readThread::getActionOnList()
{
	QList<returnActionOnCopyList> sendList;
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		sendList = actionDone;
		actionDone.clear();
	}
	return sendList;
}

void readThread::pre_operation()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(maxSpeed>0 && !clockForTheCopySpeed.isActive())
		clockForTheCopySpeed.start();
	currentPositionInSource		= 0;
	currentTransferedSizeOversize	= 0;
	emit transferIsStarted(true);
}

void readThread::post_operation()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(clockForTheCopySpeed.isActive())
		clockForTheCopySpeed.stop();
	if(writeThreadIsFinish())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"all write thread is finished");
		emit transferIsStarted(false);
	}
	while(waitNewClockForSpeed.available()<=0)
		waitNewClockForSpeed.release();
	waitNewClockForSpeed.acquire();
	currentPositionInSource		= 0;
	currentTransferedSizeOversize	= 0;
}

bool readThread::writeThreadIsFinish()
{
	bool allWriteThreadIsFinish=true;
	for (int i = 0; i < theWriteThreadList.size(); ++i)
	{
		if(theWriteThreadList.at(i)->getStatus()==writeThread::Running)
		{
			allWriteThreadIsFinish=false;
			break;
		}
	}
	return allWriteThreadIsFinish;
}

void readThread::newWriteThreadFinish()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	checkIfNeedWaitWriteThread();
	if(writeThreadIsFinish())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"all write thread is finished");
		emit transferIsStarted(false);
	}
}

QPair<quint64,quint64> readThread::getGeneralProgression()
{
	QPair<quint64,quint64> temp;
	temp.first=currentTransferedSize+currentPositionInSource;
	temp.second=totalTransferListSize+currentTransferedSizeOversize;
	return temp;
}

quint64 readThread::realByteTransfered()
{
	return actualRealByteTransfered;
}


returnSpecificFileProgression readThread::getFileProgression(const quint64& id)
{
	returnSpecificFileProgression temp;
	temp.haveBeenLocated=false;
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		int size=listTransfer.size();
		for (int index=0;index<size;++index) {
			if(id==listTransfer.at(index).id)
			{
				temp.haveBeenLocated=true;
				temp.copiedSize=currentPositionInSource;
				temp.totalSize=listTransfer.at(index).size+currentTransferedSizeOversize;
				return temp;
			}
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Warning the file cannot be located");
	return temp;
}

QList<ItemOfCopyList> readThread::getTransferList()
{
	QList<ItemOfCopyList> listToSend;
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		int size=listTransfer.size();
		for (int index=0;index<size;++index) {
			ItemOfCopyList newItemToSend;
			newItemToSend.sourceFullPath=listTransfer.at(index).source.absoluteFilePath();
			newItemToSend.sourceFileName=listTransfer.at(index).source.fileName();
			newItemToSend.destinationFullPath=listTransfer.at(index).destination.absoluteFilePath();
			newItemToSend.destinationFileName=listTransfer.at(index).destination.fileName();
			newItemToSend.size=listTransfer.at(index).size;
			newItemToSend.mode=listTransfer.at(index).mode;
			newItemToSend.id=listTransfer.at(index).id;
			listToSend << newItemToSend;
		}
	}
	return listToSend;
}

ItemOfCopyList readThread::getTransferListEntry(quint64 id)
{
	ItemOfCopyList newItemToSend;
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		int size=listTransfer.size();
		for (int index=0;index<size;++index) {
			if(id==listTransfer.at(index).id)
			{
				newItemToSend.sourceFullPath=listTransfer.at(index).source.absolutePath();
				newItemToSend.sourceFileName=listTransfer.at(index).source.fileName();
				newItemToSend.destinationFullPath=listTransfer.at(index).destination.absolutePath();
				newItemToSend.destinationFileName=listTransfer.at(index).destination.fileName();
				newItemToSend.size=listTransfer.at(index).size;
				newItemToSend.mode=listTransfer.at(index).mode;
				newItemToSend.id=listTransfer.at(index).id;
				break;
			}
		}
	}
	return newItemToSend;
}

//set the copy info and options before runing
void readThread::setRightTransfer(const bool doRightTransfer)
{
	this->doRightTransfer=doRightTransfer;
}

#ifdef DEBUG_WINDOW
void readThread::updateDebugInfo()
{
	theDebugDialog.setReadBlocking(isBlocked);
	theDebugDialog.setReadStatus(!this->isFinished());
	QStringList transferList;
	{
		QMutexLocker lock_mutex(&MultiThreadTransferList);
		int size=listTransfer.size();
		for (int index=0;index<size;++index) {
			transferList << QString::number(listTransfer.at(index).id)+") "+listTransfer.at(index).source.absoluteFilePath();
		}
	}
	theDebugDialog.setTransferList(transferList);
	QList<DebugWriteThread> writeList;
	{
		QMutexLocker lock_mutex(&MultiThreadWriteThreadList);
		int size=theWriteThreadList.size();
		for (int index=0;index<size;++index) {
			if(theWriteThreadList.at(index)->getStatus()!=writeThread::Running)
				writeList << Not_Running;
			else if(theWriteThreadList.at(index)->isBlocked)
				writeList << Running_Blocked;
			else
				writeList << Running;
		}
	}
	theDebugDialog.setWriteList(writeList);
}
#endif // DEBUG_WINDOW
