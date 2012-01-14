/***************************************************************************
				  writeThread.cpp
			      -------------------

     Begin        : Web May 6 2009 19:11 alpha_one_x86
     Project      : Ultracopier
     Email        : ultracopier@first-world.info
     Note         : See README for copyright and developer
     Target       : Define the class of the writeThread

****************************************************************************/

/// \todo test write error durring the copy
/// \todo skip byte to not put error in speed calculation

#include "writeThread.h"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <errno.h>
#endif

writeThread::writeThread(QObject * parent) :
	QThread(parent)
{
	//initialize the variable
	status			= NotRunning;
	stopIt			= false;
	endOfSource		= false;
	movingMode		= false;
	sizeList		= 0;
	CurentCopiedSize	= 0;
	preallocation		= false;
	stopThreadWhenFinish	= false;
	#ifdef DEBUG_WINDOW
	isBlocked		= false;
	#endif // DEBUG_WINDOW
	resetSemaphore();
	setObjectName("Write thread");
}

writeThread::~writeThread()
{
	stop();
}

void writeThread::setFiles(QString sourcePath,QString destinationPath)
{
	//set the copy variable
	this->source.setFileName(sourcePath);
	this->destination.setFileName(destinationPath);
}

QString writeThread::openDestination()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try open destination into the write thread: "+destination.fileName());
	//try open file destination
	if(destination.open(QIODevice::WriteOnly))
	{
		//set the status to running, because the thread will be started just after
		status	= writeThread::Running;
                stopIt = false;
                fileErrorActionSuspended = FileError_NotSet;
		//set destination position
		destination.seek(0);
		waitDestionFile.release();
		needSkipTheCurrentFile=false;
		return "";
	}
	else
        {
                ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"error at open the destination: "+destination.errorString());
		return destination.errorString();
        }
}

//stop it only in urgency case, with cancel
void writeThread::stop()
{
	/// \todo fix here clean
	/// \note never do the unlock() of mutex here, cause crash because it need by close by the opener thread
	//for prevent double call which crash ultracopier, discovered in version 0.2.0.11
	if(stopIt)
		return;
	stopIt			= true;
	//close directly the source
	if(destination.isOpen())
	{
		destination.close();
		if(destination.exists())
			destination.remove();
	}
	//release the mutex for prevent infinity loop by blocking
	while(waitErrorAction.available()<=0)
		waitErrorAction.release();
	while(waitDestionFile.available()<=0)
		waitDestionFile.release();
	while(usedBlock.available()<=0)
		usedBlock.release();
	while(freeBlock.available()<=0)
		freeBlock.release();
	wait();
}

void writeThread::addNewBlock(QByteArray newBlock)
{
	if(stopIt)
		return;
	//mutex for stream this data
	freeBlock.acquire();
	{
		QMutexLocker lock_mutex(&accessList);
		theBlockList.append(newBlock);
		CurentCopiedSize+=newBlock.size();
		sizeList++;
	}
	usedBlock.release();
}

/// \brief end of source detected
void writeThread::endOfSourceDetected()
{
	//set that's end of source is detected, and add empty block for prevent that's thread wait more data
	endOfSource=true;
	addNewBlock(QByteArray());
}

/// \brief set action on error
void writeThread::errorAction(FileErrorAction action)
{
	if(status==writeThread::WaitingTask || status==writeThread::NotRunning || status==writeThread::Terminated)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"action send when not started");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"errorAction("+QString::number(action)+")");
	fileErrorActionSuspended=action;
	if(action==FileError_Cancel)
		stop();
	else
		waitErrorAction.release();
}

/// \brief return the statut
writeThread::currentStat writeThread::getStatus()
{
	return status;
}

void writeThread::setKeepDate(bool keepDate)
{
	this->keepDate=keepDate;
}

void writeThread::setMovingMode(CopyMode mode)
{
	this->mode=mode;
}

void writeThread::setPreallocation(bool preallocation)
{
	this->preallocation=preallocation;
}

void writeThread::run()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the write thread start");
	QByteArray blockArray;
	//For this file only reset this variable
	qint64 positionInDestination=0,bytesWriten=0;
	status	= WaitingTask;
	do
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the write thread in wait mode");
		//block in the stopped mode
		waitDestionFile.acquire();
		status	= Running;
		//stop this thread for urgence or end of copy
		if(stopIt || stopThreadWhenFinish)
			break;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the write thread leave the wait mode");
		//the destination should be open here
		if(!destination.isOpen())
		{
                        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"the destination should be open here");
			stopIt=true;
			break;
		}
		//start write the data stream

		if(!stopIt && !stopThreadWhenFinish)
		{
			emit haveStartFileOperation();
			if(preallocation)
			{
				retryResizeFirst:
				if(!destination.resize(source.size()))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"the destination cannot be resized");
					switch(sendErrorOnFile(destination,tr("cannot be resized: ")+destination.errorString()))
					{
						case FileError_Retry:
							goto retryResizeFirst;
						default:
							goto SkipFile;
					}
				}
			}
			do
			{
				retryUsedBlock:
				if(stopIt)
					goto SkipFile;
				usedBlock.acquire();
				if(stopIt)
					goto SkipFile;
				//read one block
				if(!theBlockList.size())
					goto retryUsedBlock;
				else
				{
					QMutexLocker lock_mutex(&accessList);
					blockArray=theBlockList.first();
					theBlockList.removeFirst();
					sizeList--;
				}
				//write one block
				freeBlock.release();
				if(stopIt)
					goto SkipFile;
				if(!blockArray.isEmpty())
				{
					positionInDestination=destination.pos();
					retryWriteThisBlock:
					#ifdef DEBUG_WINDOW
					isBlocked=true;
					#endif // DEBUG_WINDOW
					bytesWriten=destination.write(blockArray);
					#ifdef DEBUG_WINDOW
					isBlocked=false;
					#endif // DEBUG_WINDOW
					if(stopIt)
						goto SkipFile;
					if((destination.error()!=QFile::NoError || bytesWriten!=blockArray.size()))
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Error in writing: "+destination.errorString()+" ("+QString::number(destination.error())+")");
						QString errorString=QString("Error in writing: %1 (%2)").arg(destination.errorString()).arg(destination.error());
						retrySeekDestination:
						switch(sendErrorOnFile(destination,errorString))
						{
							case FileError_Retry:
								if(!destination.seek(positionInDestination))
									goto retrySeekDestination;
								goto retryWriteThisBlock;
							default:
								goto SkipFile;
						}
					}
				}
				if(stopIt)
					goto SkipFile;
			}
			while(!blockArray.isEmpty() && (sizeList || !endOfSource));

			retryResizeLast:
			if(!destination.resize(CurentCopiedSize))
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"the destination cannot be resized");
				switch(sendErrorOnFile(destination,destination.errorString()))
				{
					case FileError_Retry:
						goto retryResizeLast;
					default:
						goto SkipFile;
				}
			}
			CurentCopiedSize=0;
			
			if(stopIt)
				goto SkipFile;

			destination.close();
	
			if(stopIt)
				goto SkipFile;
			
			//set the time if no write thread used
			if(keepDate)
			{
                                #ifndef Q_OS_WIN32
				retrySetDateLastRead:
                                #endif
				if(!destination.setLastRead(QFileInfo(source).lastRead()))
				{
					//ignore some error on windows here
					#ifndef Q_OS_WIN32
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,destination.errorString());
					switch(sendErrorOnFile(destination,destination.errorString()))
					{
						case FileError_Retry:
							goto retrySetDateLastRead;
						default:
							goto SkipFile;
					}
					#endif
				}
				retrySetDateLastModified:
				if(!destination.setLastModified(QFileInfo(source).lastModified()))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,destination.errorString());
					switch(sendErrorOnFile(destination,destination.errorString()))
					{
						case FileError_Retry:
							goto retrySetDateLastModified;
						default:
							goto SkipFile;
					}
				}
				retrySetDateCreated:
				if(!destination.setCreated(QFileInfo(source).created()))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,destination.errorString());
					switch(sendErrorOnFile(destination,destination.errorString()))
					{
						case FileError_Retry:
							goto retrySetDateCreated;
						default:
							goto SkipFile;
					}
				}
			}

			if(stopIt)
				goto SkipFile;
	
			//remove source in moving mode
			if(mode==Move)
			{
				if(destination.exists())
				{
					retryRemoveSource:
					if(!source.remove())
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to remove the source");
						switch(sendErrorOnFile(source,source.errorString()))
						{
							case FileError_Retry:
								goto retryRemoveSource;
							default:
								goto SkipFile;
						}
					}
				}
				else
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"try remove source but destination not exists!");
			}
	
	/////////////////////////
	SkipFile:
	/////////////////////////
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try close the destination");
			destination.close();
			if((fileErrorActionSuspended==FileError_Skip || fileErrorActionSuspended==FileError_PutToEndOfTheList || fileErrorActionSuspended==FileError_Cancel || stopIt) && destination.exists())
			{
				if(!source.exists())
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"try remove destination but source not exists!");
				else
                                {
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"fileErrorActionSuspended: "+QString::number(fileErrorActionSuspended));
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stopIt: "+QString::number(stopIt));
					destination.remove(); //skip the verification, because it's will repasse here then
                                }
			}
			resetSemaphore();
			if(needSkipTheCurrentFile)
				stopIt=false;
			endOfSource	= false;
			CurentCopiedSize= 0;
			theBlockList.clear();
			status	= WaitingTask;
			emit haveFinishFileOperation();
		}
	} while(!stopIt && !stopThreadWhenFinish);
	status	= Terminated;
}

void writeThread::stopWhenIsFinish(bool stopThreadWhenFinish)
{
        this->stopThreadWhenFinish=stopThreadWhenFinish;
	addNewBlock(QByteArray());
}

/// \brief error on file or folder
FileErrorAction writeThread::sendErrorOnFile(QFileInfo fileInfo,QString errorString)
{
	fileErrorActionSuspended=FileError_NotSet;
	emit errorOnFile(fileInfo,errorString);
	waitErrorAction.acquire();
	if(fileErrorActionSuspended==FileError_NotSet)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Action not set, then quit");
		fileErrorActionSuspended=FileError_Cancel;
	}
	return fileErrorActionSuspended;
}

void writeThread::skipTheCurrentFile()
{
	if(status==writeThread::WaitingTask)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Already prepared for the next file");
		return;
	}
	if(status==writeThread::NotRunning || status==writeThread::Terminated)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"The thread is already out of contect of file skipping: "+QString::number(status));
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Skip the current file");
	needSkipTheCurrentFile=true;
	stopIt=true;
	addNewBlock(QByteArray());
}

void writeThread::resetSemaphore()
{
	//set to 0
	while(waitErrorAction.available()<=0)
		waitErrorAction.release();
	waitErrorAction.acquire(waitErrorAction.available());
	//set to 0
	while(waitDestionFile.available()<=0)
		waitDestionFile.release();
	waitDestionFile.acquire(waitDestionFile.available());
	//set to 0
	while(usedBlock.available()<=0)
		usedBlock.release();
	usedBlock.acquire(usedBlock.available());
	//set to LISTBLOCKSIZE
	while(freeBlock.available()<LISTBLOCKSIZE)
		freeBlock.release();
	if(freeBlock.available()>LISTBLOCKSIZE)
		freeBlock.acquire(freeBlock.available()-LISTBLOCKSIZE);
}
