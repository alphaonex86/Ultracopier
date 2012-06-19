//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThread.h"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#endif

/// \todo manage case resume after error, because previously inode opt free before
/// \todo manage error in pre and post operation
/// \todo remove destination when canceled
/// \todo test if source if closed by end but write error
/// \todo pointer for readThread and writeThread to destroy the read before the write (prevent dead lock)
/// \todo read close never call
/// \todo readThreadIsReopened for the checksum, or if source read have failed (network share disconnecte and reconnected)

/// \bug continue progress when write error

TransferThread::TransferThread()
{
	start();
	moveToThread(this);
	needSkip		= false;
	stat			= TransferStat_Idle;
	stopIt			= false;
	fileExistsAction	= FileExists_NotSet;
	alwaysDoFileExistsAction= FileExists_NotSet;
	readError		= false;
	writeError		= false;
	this->mkpathTransfer	= mkpathTransfer;
	readThread.setWriteThread(&writeThread);

	connect(&clockForTheCopySpeed,	SIGNAL(timeout()),			this,	SLOT(timeOfTheBlockCopyFinished()));
	maxTime=QDateTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
}

TransferThread::~TransferThread()
{
	exit();
	disconnect(&readThread);
	disconnect(&writeThread);
	disconnect(this);
	wait();
}

void TransferThread::run()
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start: "+QString::number((qint64)QThread::currentThreadId()));
	stat			= TransferStat_Idle;
	stopIt			= false;
	fileExistsAction	= FileExists_NotSet;
	alwaysDoFileExistsAction= FileExists_NotSet;
	//the error push
	connect(&readThread,SIGNAL(error()),			this,					SLOT(getReadError()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(error()),			this,					SLOT(getWriteError()),		Qt::QueuedConnection);
	//the thread change operation
	connect(this,SIGNAL(internalStartPreOperation()),	this,					SLOT(preOperation()),		Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartPostOperation()),	this,					SLOT(postOperation()),		Qt::QueuedConnection);
        //the state change operation
	//connect(&readThread,SIGNAL(readIsStopped()),		&readThread,				SLOT(postOperation()),		Qt::QueuedConnection);//commented to do the checksum
	connect(&readThread,SIGNAL(opened()),			this,					SLOT(readIsReady()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(opened()),			this,					SLOT(writeIsReady()),		Qt::QueuedConnection);
	connect(&readThread,SIGNAL(readIsStopped()),		this,					SLOT(readIsStopped()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(writeIsStopped()),		this,					SLOT(writeIsStopped()),		Qt::QueuedConnection);
	connect(&readThread,SIGNAL(readIsStopped()),		&writeThread,				SLOT(endIsDetected()),		Qt::QueuedConnection);
	//connect(&writeThread,SIGNAL(writeIsStopped()),		&writeThread,				SLOT(postOperation()),		Qt::QueuedConnection);//commented to do the checksum
	connect(&readThread,SIGNAL(closed()),			this,					SLOT(readIsClosed()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(closed()),			this,					SLOT(writeIsClosed()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(reopened()),		this,					SLOT(writeThreadIsReopened()),	Qt::QueuedConnection);
	connect(&readThread,SIGNAL(checksumFinish(QByteArray)),	this,					SLOT(readChecksumFinish(QByteArray)),	Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(checksumFinish(QByteArray)),this,					SLOT(writeChecksumFinish(QByteArray)),	Qt::QueuedConnection);
        //error management
	connect(&readThread,SIGNAL(isSeekToZeroAndWait()),	this,					SLOT(readThreadIsSeekToZeroAndWait()),	Qt::QueuedConnection);
	connect(&readThread,SIGNAL(resumeAfterErrorByRestartAtTheLastPosition()),	this,		SLOT(readThreadResumeAfterError()),	Qt::QueuedConnection);
	connect(&readThread,SIGNAL(resumeAfterErrorByRestartAll()),                     &writeThread,	SLOT(flushAndSeekToZero()),		Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(flushedAndSeekedToZero()),                          this,           SLOT(readThreadResumeAfterError()),	Qt::QueuedConnection);
	connect(this,SIGNAL(internalTryStartTheTransfer()),	this,					SLOT(internalStartTheTransfer()),	Qt::QueuedConnection);

	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(&readThread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),Qt::QueuedConnection);
	connect(&writeThread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),Qt::QueuedConnection);
	#endif

	/// \todo do the current post opt only after the read write opt
	exec();
}

TransferStat TransferThread::getStat()
{
	return stat;
}

void TransferThread::startTheTransfer()
{
	emit internalTryStartTheTransfer();
}

void TransferThread::internalStartTheTransfer()
{
	if(stat==TransferStat_Idle)
	{
		if(mode!=Move)
		{
			/// \bug can pass here because in case of direct move on same media, it return to idle stat directly
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at idle");
		}
		return;
	}
	if(stat==TransferStat_PostOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at PostOperation");
		return;
	}
	if(stat==TransferStat_Transfer)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] can't start transfert at Transfer");
		return;
	}
	if(canStartTransfer)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] canStartTransfer is already set to true");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] check how start the transfer");
	canStartTransfer=true;
	if(readIsReadyVariable && writeIsReadyVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start directly the transfer");
		ifCanStartTransfer();
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start the transfer as delayed");
}

void TransferThread::setFiles(const QString &source,const qint64 &size,const QString &destination,const CopyMode &mode)
{
	if(stat!=TransferStat_Idle)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
		return;
	}
	//to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
	stat			= TransferStat_PreOperation;
	//emit pushStat(stat,transferId);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start, source: "+source+", destination: "+destination);
	this->source			= source;
	this->destination		= destination;
	this->mode			= mode;
	this->size			= size;
	fileExistsAction		= FileExists_NotSet;
	canStartTransfer		= false;
	sended_state_preOperationStopped= false;
	canBeMovedDirectlyVariable	= false;
	fileContentError		= false;
	resetExtraVariable();
	emit internalStartPreOperation();
}

void TransferThread::setFileExistsAction(const FileExistsAction &action)
{
	if(stat!=TransferStat_PreOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] action: "+QString::number(action));
	if(action!=FileExists_Rename)
		fileExistsAction	= action;
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] rename at the wrong part, source: "+source+", destination: "+destination);
	if(action==FileExists_Skip)
	{
		skip();
		return;
	}
	resetExtraVariable();
	emit internalStartPreOperation();
}

void TransferThread::setFileRename(const QString &nameForRename)
{
	if(stat!=TransferStat_PreOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] nameForRename: "+nameForRename);
	destinationInfo.setFile(destination);
	destination=destinationInfo.absolutePath();
	destination+=QDir::separator()+nameForRename;
	destinationInfo.setFile(destination);
	fileExistsAction	= FileExists_NotSet;
	resetExtraVariable();
	emit internalStartPreOperation();
}

void TransferThread::setAlwaysFileExistsAction(const FileExistsAction &action)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] action to do always: "+QString::number(action));
	alwaysDoFileExistsAction=action;
}

void TransferThread::resetExtraVariable()
{
	sended_state_readStopped	= false;
	sended_state_writeStopped	= false;
	writeError			= false;
	readError			= false;
	readIsReadyVariable		= false;
	writeIsReadyVariable		= false;
	readIsFinishVariable		= false;
	writeIsFinishVariable		= false;
	readIsClosedVariable		= false;
	writeIsClosedVariable		= false;
	needSkip			= false;
	retry				= false;
	readIsOpenVariable		= false;
	writeIsOpenVariable		= false;
}

void TransferThread::preOperation()
{
	if(stat!=TransferStat_PreOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] already used, source: "+source+", destination: "+destination);
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	needRemove=false;
	sourceInfo.setFile(source);
	destinationInfo.setFile(destination);
	if(isSame())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] is same"+source);
		return;
	}
	if(destinationExists())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] destination exists: "+source);
		return;
	}
	if(canBeMovedDirectly())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] need moved directly: "+source);
		canBeMovedDirectlyVariable=true;
		readThread.fakeOpen();
		writeThread.fakeOpen();
		return;
	}
	tryOpen();
}

void TransferThread::tryOpen()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start source and destination: "+source+" and "+destination);
	if(!readIsOpenVariable)
	{
		readError=false;
		readThread.open(source,mode);
	}
	if(!writeIsOpenVariable)
	{
		writeError=false;
		writeThread.open(destination,size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)));
	}
}

bool TransferThread::isSame()
{
	//check if source and destination is not the same
	if(sourceInfo==destinationInfo)
	{
		emit fileAlreadyExists(sourceInfo,destinationInfo,true);
		return true;
	}
	return false;
}

bool TransferThread::destinationExists()
{
	/// \todo do the overwrite: FileExists_OverwriteIfNotSameModificationDate
	//check if destination exists
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] overwrite: "+QString::number(fileExistsAction)+", always action: "+QString::number(alwaysDoFileExistsAction));
	if(alwaysDoFileExistsAction==FileExists_Overwrite || readError || writeError)
		return false;
	if(destinationInfo.exists())
	{
		if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
		{
			stat=TransferStat_Idle;
			emit postOperationStopped();
			return true;
		}
		if(alwaysDoFileExistsAction==FileExists_Rename)
		{
			QString absolutePath=destinationInfo.absolutePath();
			QString fileName=destinationInfo.fileName();
			QString suffix="";
			QString newFileName;
			//resolv the suffix
			if(fileName.contains(QRegExp("^(.*)(\\.[a-z0-9]+)$")))
			{
				suffix=fileName;
				suffix.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\2");
				fileName.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\1");
			}
			//resolv the new name
			int num=1;
			do
			{
				if(num==1)
				{
					if(firstRenamingRule=="")
						newFileName=tr("%1 - copy").arg(fileName);
					else
					{
						newFileName=firstRenamingRule;
						newFileName.replace("%name%",fileName);
					}
				}
				else
				{
					if(otherRenamingRule=="")
						newFileName=tr("%1 - copy (%2)").arg(fileName).arg(num);
					else
					{
						newFileName=otherRenamingRule;
						newFileName.replace("%name%",fileName);
						newFileName.replace("%number%",QString::number(num));
					}
				}
				destination=absolutePath+QDir::separator()+newFileName+suffix;
				destinationInfo.setFile(destination);
				num++;
			}
			while(destinationInfo.exists());
			return false;
		}
		if(fileExistsAction==FileExists_OverwriteIfNewer || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNewer))
		{
			if(destinationInfo.lastModified()<sourceInfo.lastModified())
				return false;
			else
			{
				stat=TransferStat_Idle;
				emit postOperationStopped();
				return true;
			}
		}
		if(fileExistsAction==FileExists_OverwriteIfNotSameModificationDate || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNotSameModificationDate))
		{
			if(destinationInfo.lastModified()!=sourceInfo.lastModified())
				return false;
			else
			{
				stat=TransferStat_Idle;
				emit postOperationStopped();
				return true;
			}
		}
		if(fileExistsAction==FileExists_NotSet)
		{
			emit fileAlreadyExists(sourceInfo,destinationInfo,false);
			return true;
		}
	}
	return false;
}

void TransferThread::tryMoveDirectly()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start the system move");

	//move if on same mount point
	QFile sourceFile(sourceInfo.absoluteFilePath());
	QFile destinationFile(destinationInfo.absoluteFilePath());
	if(destinationFile.exists() && !destinationFile.remove())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+destinationFile.fileName()+", error: "+destinationFile.errorString());
		emit errorOnFile(destinationInfo,destinationFile.errorString());
		return;
	}
	QDir dir(destinationInfo.absolutePath());
	{
		mkpathTransfer->acquire();
		if(!dir.exists())
			dir.mkpath(destinationInfo.absolutePath());
		mkpathTransfer->release();
	}
	if(!sourceFile.rename(destinationFile.fileName()))
	{
		if(sourceFile.exists())
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("file not not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
		else if(!dir.exists())
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("destination folder not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("unable to do real move %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()));
		emit errorOnFile(sourceFile,sourceFile.errorString());
		return;
	}
	readThread.fakeReadIsStarted();
	writeThread.fakeWriteIsStarted();
	readThread.fakeReadIsStopped();
	writeThread.fakeWriteIsStopped();
}

bool TransferThread::canBeMovedDirectly()
{
	//move if on same mount point
	#if defined (Q_OS_LINUX) || defined (Q_OS_WIN32)
	if(mode!=Move)
		return false;
	if(mountSysPoint.size()==0)
		return false;
	if(getDrive(destinationInfo.fileName())==getDrive(sourceInfo.fileName()))
		return true;
	#endif
	return false;
}

void TransferThread::readIsReady()
{
	if(readIsReadyVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	readIsReadyVariable=true;
	readIsOpenVariable=true;
	readIsClosedVariable=false;
	ifCanStartTransfer();
}

void TransferThread::ifCanStartTransfer()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] readIsReadyVariable: "+QString::number(readIsReadyVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable));
	if(readIsReadyVariable && writeIsReadyVariable)
	{
		stat=TransferStat_WaitForTheTransfer;
		sended_state_readStopped	= false;
		sended_state_writeStopped	= false;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] stat=WaitForTheTransfer");
		if(!sended_state_preOperationStopped)
		{
			sended_state_preOperationStopped=true;
			emit preOperationStopped();
		}
		if(canStartTransfer)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] stat=Transfer");
			stat=TransferStat_Transfer;
			if(!canBeMovedDirectlyVariable)
			{
				needRemove=true;
				readThread.startRead();
			}
			else
				tryMoveDirectly();
			emit pushStat(stat,transferId);
		}
		//else
			//emit pushStat(stat,transferId);
	}
}

void TransferThread::writeIsReady()
{
	if(writeIsReadyVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	writeIsReadyVariable=true;
	writeIsOpenVariable=true;
	writeIsClosedVariable=false;
	ifCanStartTransfer();
}


//set the copy info and options before runing
void TransferThread::setRightTransfer(const bool doRightTransfer)
{
	this->doRightTransfer=doRightTransfer;
}

//set keep date
void TransferThread::setKeepDate(const bool keepDate)
{
	this->keepDate=keepDate;
}

//set the current max speed in KB/s
void TransferThread::setMaxSpeed(int maxSpeed)
{
	int interval=readThread.setMaxSpeed(maxSpeed);
	writeThread.setMaxSpeed(maxSpeed);
	if(maxSpeed>0)
	{
		clockForTheCopySpeed.setInterval(interval);
		if(!clockForTheCopySpeed.isActive())//seam useless !this->isFinished()
			clockForTheCopySpeed.start();
	}
	else
	{
		if(clockForTheCopySpeed.isActive())
			clockForTheCopySpeed.stop();
	}
}

//set block size in KB
bool TransferThread::setBlockSize(const unsigned int blockSize)
{
	this->blockSize=blockSize;
	return readThread.setBlockSize(blockSize) && writeThread.setBlockSize(blockSize);
}

//pause the copy
void TransferThread::pause()
{
	if(stat==TransferStat_Idle)
		return;
	readThread.pause();
}

//resume the copy
void TransferThread::resume()
{
	if(stat==TransferStat_Idle)
		return;
	readThread.resume();
}

//stop the current copy
void TransferThread::stop()
{
	stopIt=true;
	if(stat==TransferStat_Idle)
		return;
	readThread.stop();
	writeThread.stop();
}

void TransferThread::readIsFinish()
{
	if(readIsFinishVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	readIsFinishVariable=true;
	canStartTransfer=false;
	//check here if need start checksuming or not
	real_doChecksum=doChecksum && (!checksumOnlyOnError || fileContentError);
	if(real_doChecksum)
	{
		readIsFinishVariable=false;
		stat=TransferStat_Checksum;
		sourceChecksum=QByteArray();
		destinationChecksum=QByteArray();
		readThread.startCheckSum();
	}
	else
	{
		stat=TransferStat_PostTransfer;
		readThread.postOperation();
	}
	emit pushStat(stat,transferId);
}

void TransferThread::writeIsFinish()
{
	if(writeIsFinishVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	writeIsFinishVariable=true;
	//check here if need start checksuming or not
	if(real_doChecksum)
	{
		writeIsFinishVariable=false;
		stat=TransferStat_Checksum;
		writeThread.startCheckSum();
	}
	else
		writeThread.postOperation();
}

void TransferThread::readChecksumFinish(const QByteArray& checksum)
{
	sourceChecksum=checksum;
	compareChecksum();
}

void TransferThread::writeChecksumFinish(const QByteArray& checksum)
{
	destinationChecksum=checksum;
	compareChecksum();
}

void TransferThread::compareChecksum()
{
	if(sourceChecksum.size()==0)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] the checksum of source is missing");
		return;
	}
	if(destinationChecksum.size()==0)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] the checksum of destination is missing");
		return;
	}
	if(sourceChecksum==destinationChecksum)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] the checksum match");
		readThread.postOperation();
		writeThread.postOperation();
		stat=TransferStat_PostTransfer;
		emit pushStat(stat,transferId);
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] the checksum not match");
		//emit error here, and wait to resume
		emit errorOnFile(destinationInfo,tr("The checksums not match"));
	}
}

void TransferThread::readIsClosed()
{
	if(readIsClosedVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	readIsClosedVariable=true;
	checkIfAllIsClosed();
}

void TransferThread::writeIsClosed()
{
	if(writeIsClosedVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] double event dropped");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	writeIsClosedVariable=true;
	checkIfAllIsClosed();
}

bool TransferThread::checkIfAllIsClosed()
{
	if((readError || writeError) && !needSkip)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] resolve error before progress");
		return false;
	}
	if((!readIsReadyVariable || readIsClosedVariable) && (!writeIsReadyVariable || writeIsClosedVariable))
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] emit internalStartPostOperation() to do the real post operation");
		stat=TransferStat_PostOperation;
		//emit pushStat(stat,transferId);
		emit internalStartPostOperation();
		return true;
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] "+QString("wait self close: readIsReadyVariable: %1, readIsClosedVariable: %2, writeIsReadyVariable: %3, writeIsClosedVariable: %4")
					 .arg(readIsReadyVariable)
					 .arg(readIsClosedVariable)
					 .arg(writeIsReadyVariable)
					 .arg(writeIsClosedVariable)
					 );
		return false;
	}
}

/// \todo found way to retry that's
/// \todo the rights copy
void TransferThread::postOperation()
{
	if(stat!=TransferStat_PostOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] need be in transfer, source: "+source+", destination: "+destination+", stat:"+QString::number(stat));
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	//all except closing
	if((readError || writeError) && !needSkip)//normally useless by checkIfAllIsFinish()
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] resume after error");
		return;
	}

	if(!needSkip)
	{
		if(!doFilePostOperation())
			return;

		//remove source in moving mode
		if(mode==Move && !canBeMovedDirectlyVariable)
		{
			if(QFile::exists(destination))
			{
				QFile sourceFile(source);
				if(!sourceFile.remove())
				{
					emit errorOnFile(sourceInfo,sourceFile.errorString());
					return;
				}
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] try remove source but destination not exists!");
		}
	}
	else//do difference skip a file and skip this error case
	{
		if(needRemove && QFile::exists(destination))
		{
			QFile destinationFile(destination);
			if(!destinationFile.remove())
			{
				//emit errorOnFile(sourceInfo,destinationFile.errorString());
				//return;
			}
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] try remove destination but not exists!");
	}
	stat=TransferStat_Idle;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] emit postOperationStopped()");
	emit postOperationStopped();
}

bool TransferThread::doFilePostOperation()
{
	//do operation needed by copy
	if(!canBeMovedDirectlyVariable)
	{
		//set the time if no write thread used
		if(keepDate)
			changeFileDateTime(destination,source);//can't do that's after move because after move the source not exist
			/*
			  ignore it, because need correct management, mainly with move
			if(!)
			{
				emit errorOnFile(destinationInfo,tr("Unable to change the date"));//destination.errorString()
				return false;
			}*/
	}

	if(stopIt)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::getWriteError()
{
        if(writeError)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] already in write error!");
                return;
        }
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	fileContentError		= true;
	writeError			= true;
	writeIsReadyVariable		= false;
	writeError_source_seeked	= false;
	writeError_destination_reopened	= false;
	emit errorOnFile(destinationInfo,writeThread.errorString());
}

void TransferThread::getReadError()
{
        if(readError)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] already in read error!");
                return;
        }
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	fileContentError	= true;
	readError		= true;
	writeIsReadyVariable	= false;
	readIsReadyVariable	= false;
	emit errorOnFile(sourceInfo,readThread.errorString());
}

//retry after error
void TransferThread::retryAfterError()
{
	//opening error
	if(stat==TransferStat_PreOperation)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] is not idle, source: "+source+", destination: "+destination+", stat: "+QString::number(stat));
		tryOpen();
		return;
	}
	//data streaming error
	if(stat!=TransferStat_PostOperation && stat!=TransferStat_Transfer && stat!=TransferStat_Checksum)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] is not idle, source: "+source+", destination: "+destination+", stat: "+QString::number(stat));
		return;
	}
	if(canBeMovedDirectlyVariable)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] retry the system move");
		tryMoveDirectly();
		return;
	}
	if(stat==TransferStat_Checksum)
	{
		if(writeError)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start and resume the write error");
			writeThread.reopen();
		}
		else if(readError)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start and resume the read error");
			readThread.reopen();
		}
		else //only checksum difference
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] retry all the transfer");
			canStartTransfer=true;
			ifCanStartTransfer();
		}
		return;
	}
	if(writeError)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start and resume the write error");
		readThread.seekToZeroAndWait();
		writeThread.reopen();
	}
	else if(readError)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start and resume the read error");
		readThread.reopen();
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] unknow error resume");
}

void TransferThread::writeThreadIsReopened()
{
	if(writeError_destination_reopened)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
		return;
	}
	writeError_destination_reopened=true;
	if(stat==TransferStat_Checksum)
	{
		writeThread.startCheckSum();
		return;
	}
	if(writeError_source_seeked && writeError_destination_reopened)
		resumeTransferAfterWriteError();
}

void TransferThread::readThreadIsSeekToZeroAndWait()
{
	if(writeError_source_seeked)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
		return;
	}
	writeError_source_seeked=true;
	if(writeError_source_seeked && writeError_destination_reopened)
		resumeTransferAfterWriteError();
}

void TransferThread::resumeTransferAfterWriteError()
{
	writeError=false;
/********************************
 if(canStartTransfer)
	 readThread.startRead();
useless, because the open destination event
will restart the transfer as normal
*********************************/
/*********************************
if(!canStartTransfer)
	stat=WaitForTheTransfer;
useless because already do at open event
**********************************/
	//if is in wait
	if(!canStartTransfer)
		emit checkIfItCanBeResumed();
}

void TransferThread::readThreadResumeAfterError()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
        readError=false;
        writeIsReady();
        readIsReady();
}

//////////////////////////////////////////////////////////////////
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::readIsStopped()
{
	if(!sended_state_readStopped)
	{
		sended_state_readStopped=true;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] emit readIsStopped()");
		emit readStopped();
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] drop dual read stopped");
	readIsFinish();
}

void TransferThread::writeIsStopped()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	if(!sended_state_writeStopped)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] emit writeStopped()");
		sended_state_writeStopped=true;
		emit writeStopped();
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] double event dropped");
	writeIsFinish();
}

void TransferThread::timeOfTheBlockCopyFinished()
{
	readThread.timeOfTheBlockCopyFinished();
}

//get drive of an file or folder
QString TransferThread::getDrive(QString fileOrFolder)
{
	for (int i = 0; i < mountSysPoint.size(); ++i) {
		if(fileOrFolder.startsWith(mountSysPoint.at(i)))
			return mountSysPoint.at(i);
	}
	//if unable to locate the right mount point
	return "";
}

//set drive list, used in getDrive()
void TransferThread::setDrive(QStringList drives)
{
	mountSysPoint=drives;
}

//fonction to edit the file date time
bool TransferThread::changeFileDateTime(const QString &source,const QString &destination)
{
	/*
	  if(maxTime>=sourceInfo.lastModified())
		return;
	  */
	QFileInfo fileInfo(destination);
	time_t ctime=fileInfo.created().toTime_t();
	time_t actime=fileInfo.lastRead().toTime_t();
	time_t modtime=fileInfo.lastModified().toTime_t();
	#ifdef Q_CC_GNU
		//this function avalaible on unix and mingw
		utimbuf butime;
		butime.actime=actime;
		butime.modtime=modtime;
		//creation time not exists into unix world
		Q_UNUSED(ctime)
		return utime(source.toLatin1().data(),&butime)==0;
	#else
		return false;
	#endif
	return true;
}

//skip the copy
void TransferThread::skip()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start with stat: "+QString::number(stat));
	switch(stat)
	{
	case TransferStat_PreOperation:
	case TransferStat_WaitForTheTransfer:
		needSkip=true;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] case WaitForTheTransfer or PreOperation, readIsReadyVariable: "+QString::number(readIsReadyVariable)+", readIsClosedVariable: "+QString::number(readIsClosedVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable)+", writeIsClosedVariable: "+QString::number(writeIsClosedVariable));
		//check if all is source and destination is closed
		if(!checkIfAllIsClosed())
		{
			if(readIsReadyVariable && !readIsClosedVariable)
				readThread.stop();
			if(writeIsReadyVariable && !writeIsClosedVariable)
				writeThread.stop();
		}
		break;
	case TransferStat_Transfer:
		needSkip=true;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] case Transfer, readIsReadyVariable: "+QString::number(readIsReadyVariable)+", readIsClosedVariable: "+QString::number(readIsClosedVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable)+", writeIsClosedVariable: "+QString::number(writeIsClosedVariable));
		if(!checkIfAllIsClosed())
		{
			if(readIsReadyVariable && !readIsClosedVariable)
				readThread.stop();
			if(writeIsReadyVariable && !writeIsClosedVariable)
				writeThread.stop();
		}
		break;
	case TransferStat_Checksum:
		needSkip=true;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] case Transfer, readIsReadyVariable: "+QString::number(readIsReadyVariable)+", readIsClosedVariable: "+QString::number(readIsClosedVariable)+", writeIsReadyVariable: "+QString::number(writeIsReadyVariable)+", writeIsClosedVariable: "+QString::number(writeIsClosedVariable));
		if(!checkIfAllIsClosed())
		{
			if(readIsReadyVariable && !readIsClosedVariable)
				readThread.stop();
			if(writeIsReadyVariable && !writeIsClosedVariable)
				writeThread.stop();
		}
		break;
	case TransferStat_PostOperation:
		//do nothing because here is closing...
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] is already in post op");
		break;
	default:
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] can skip in this state!");
		return;
	}
}

//return info about the copied size
qint64 TransferThread::copiedSize()
{
	switch(stat)
	{
	case TransferStat_Transfer:
	case TransferStat_PostOperation:
		return readThread.getLastGoodPosition();
	case TransferStat_Checksum:
		return transferSize;
	default:
		return 0;
	}
}

//retry after error
void TransferThread::putAtBottom()
{
	emit tryPutAtBottom();
}

void TransferThread::set_osBufferLimit(unsigned int osBufferLimit)
{
	this->osBufferLimit=osBufferLimit;
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThread::setId(int id)
{
	this->id=id;
	readThread.setId(id);
	writeThread.setId(id);
}

QChar TransferThread::readingLetter()
{
	switch(readThread.stat)
	{
	case ReadThread::Idle:
		return '_';
	break;
	case ReadThread::InodeOperation:
		return 'I';
	break;
	case ReadThread::Read:
		return 'R';
	break;
	case ReadThread::WaitWritePipe:
		return 'W';
	break;
	case ReadThread::Checksum:
		return 'S';
	break;
	default:
		return '?';
	}
}

QChar TransferThread::writingLetter()
{
	switch(writeThread.stat)
	{
	case WriteThread::Idle:
		return '_';
	break;
	case WriteThread::InodeOperation:
		return 'I';
	break;
	case WriteThread::Write:
		return 'W';
	break;
	case WriteThread::Close:
		return 'C';
	break;
	case WriteThread::Read:
		return 'R';
	break;
	case WriteThread::Checksum:
		return 'S';
	break;
	default:
		return '?';
	}
}

#endif

void TransferThread::setMkpathTransfer(QSemaphore *mkpathTransfer)
{
	this->mkpathTransfer=mkpathTransfer;
	writeThread.setMkpathTransfer(mkpathTransfer);
}

void TransferThread::set_doChecksum(bool doChecksum)
{
	this->doChecksum=doChecksum;
}

void TransferThread::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
	this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void TransferThread::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
	this->checksumOnlyOnError=checksumOnlyOnError;
}

void TransferThread::set_osBuffer(bool osBuffer)
{
	this->osBuffer=osBuffer;
}

void TransferThread::set_osBufferLimited(bool osBufferLimited)
{
	this->osBufferLimited=osBufferLimited;
}

//not copied size, because that's count to the checksum, ...
quint64 TransferThread::realByteTransfered()
{
	switch(stat)
	{
	case TransferStat_Transfer:
	case TransferStat_PostOperation:
	case TransferStat_Checksum:
		return readThread.getLastGoodPosition();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] transferSize: "+QString::number(transferSize));
		return transferSize;
	default:
		return 0;
	}
}

void TransferThread::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
	this->firstRenamingRule=firstRenamingRule;
	this->otherRenamingRule=otherRenamingRule;
}
