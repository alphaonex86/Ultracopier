#include "ReadThread.h"

ReadThread::ReadThread()
{
	start();
	moveToThread(this);
	stopIt=false;
	putInPause=false;
	blockSize=1024*1024;
	setObjectName("read");
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	stat=Idle;
	#endif
	isInReadLoop=false;
        tryStartRead=false;
	isOpen.release();
}

ReadThread::~ReadThread()
{
	stopIt=true;
	disconnect(this);
	waitNewClockForSpeed.release();
	isOpen.acquire();
	exit();
	wait();
}

void ReadThread::run()
{
	connect(this,SIGNAL(internalStartOpen()),	this,SLOT(internalOpen()),	Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartReopen()),	this,SLOT(internalReopen()),	Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartRead()),	this,SLOT(internalRead()),	Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartClose()),	this,SLOT(internalClose()),	Qt::QueuedConnection);
	connect(this,SIGNAL(checkIfIsWait()),		this,SLOT(isInWait()),		Qt::QueuedConnection);
	connect(this,SIGNAL(internalStartChecksum()),	this,SLOT(checkSum()),		Qt::QueuedConnection);
	exec();
}

void ReadThread::open(const QString &name,const CopyMode &mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] open source: "+name);
        if(file.isOpen())
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] previous file is already open: "+file.fileName()+", try open: "+this->name);
                return;
        }
        if(isInReadLoop)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] previous file is already readding: "+file.fileName()+", try open: "+this->name);
                return;
        }
        if(tryStartRead)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] previous file is already try read: "+file.fileName()+", try open: "+this->name);
                return;
        }
	fakeMode=false;
	this->name=name;
        this->mode=mode;
	emit internalStartOpen();
}

QString ReadThread::errorString()
{
	return errorString_internal;
}

void ReadThread::stop()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] stop()");
	stopIt=true;
	emit internalStartClose();
}

bool ReadThread::pause()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] try put read thread in pause");
	putInPause=true;
	stopIt=true;
	return isInReadLoop;
}

void ReadThread::resume()
{
	if(putInPause)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
		putInPause=false;
		stopIt=false;
	}
	else
		return;
	if(tryStartRead)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] already in try start");
		return;
	}
        tryStartRead=true;
	if(isInReadLoop)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] is in read loop");
		return;
	}
	if(!file.isOpen())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] file is not open");
		return;
	}
	emit internalStartRead();
}

bool ReadThread::seek(qint64 position)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start with: "+QString::number(position));
	if(position>file.size())
		return false;
	lastGoodPosition=position;
	return file.seek(position);
}

qint64 ReadThread::size()
{
	return file.size();
}

void ReadThread::postOperation()
{
	emit internalStartClose();
}

void ReadThread::checkSum()
{
	QByteArray blockArray;
	QCryptographicHash hash;
	isInReadLoop=true;
	lastGoodPosition=0;
	do
	{
		//read one block
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Read;
		#endif
		blockArray=file.read(blockSize);
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Idle;
		#endif

		if(file.error()!=QFile::NoError)
		{
			errorString_internal=tr("Unable to read the source file: ")+file.errorString()+" ("+QString::number(file.error())+")";
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("file.error()!=QFile::NoError: %1, error: %2").arg(QString::number(file.error())).arg(errorString_internal));
			emit error();
			isInReadLoop=false;
			return;
		}
		sizeReaden=blockArray.size();
		if(sizeReaden>0)
		{
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			stat=Checksum;
			#endif
			hash.addData(blockArray);
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			stat=Idle;
			#endif

			if(stopIt)
				break;

			lastGoodPosition+=blockArray.size();

			//wait for limitation speed if stop not query
			if(maxSpeed>0)
			{
				numberOfBlockCopied++;
				if(numberOfBlockCopied>=MultiForBigSpeed)
				{
					numberOfBlockCopied=0;
					waitNewClockForSpeed.acquire();
					if(stopIt)
						break;
				}
			}
			/*if(lastGoodPosition>size)
				oversize=lastGoodPosition-size;*/
		}
	}
	while(sizeReaden>0 && !stopIt);
	if(lastGoodPosition>file.size())
	{
		errorString_internal=tr("File truncated during the read, possible data change");
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Source truncated during the read: %1 (%2)").arg(file.errorString()).arg(QString::number(file.error())));
		emit error();
		isInReadLoop=false;
		return;
	}
	isInReadLoop=false;
	if(stopIt)
	{
		if(putInPause)
			emit isInPause();
		stopIt=false;
		return;
	}
	emit checksumFinish(hash.result());
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] stop the read");
}

void ReadThread::startRead()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	if(tryStartRead)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] already in try start");
		return;
	}
	if(isInReadLoop)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
	else
	{
		tryStartRead=true;
		emit internalStartRead();
	}
	emit checksumFinish(QByteArray().resize(2));
}

bool ReadThread::internalOpen(bool resetLastGoodPosition)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] internalOpen source: "+name);
	stopIt=false;
	putInPause=false;
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	stat=InodeOperation;
	#endif
        if(file.isOpen())
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] already open! source: "+name);
                #ifdef ULTRACOPIER_PLUGIN_DEBUG
                stat=Idle;
                #endif
                return false;
        }
	file.setFileName(name);
	QIODevice::OpenMode openMode=QIODevice::ReadOnly;
	if(mode==Move)
		openMode=QIODevice::ReadWrite;
	seekToZero=false;
	if(file.open(openMode))
	{
                size_at_open=file.size();
                mtime_at_open=QFileInfo(file).lastModified();
		putInPause=false;
		if(resetLastGoodPosition)
		{
			seek(0);
			emit opened();
		}
		else if(!seek(lastGoodPosition))
		{
			errorString_internal=file.errorString();
                        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to seek after open: %1, error: %2").arg(name).arg(errorString_internal));
			emit error();
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			stat=Idle;
			#endif
			return false;
		}
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Idle;
		#endif
		isOpen.acquire();
		return true;
	}
	else
	{
		errorString_internal=file.errorString();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Unable to open: %1, error: %2").arg(name).arg(errorString_internal));
		emit error();
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Idle;
		#endif
		return false;
	}
}

void ReadThread::internalRead()
{
	isInReadLoop=true;
        tryStartRead=false;
        if(stopIt)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] stopIt == true, then quit");
		internalClose();
                return;
	}
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	stat=InodeOperation;
	#endif
	int sizeReaden=0;
	if(!file.isOpen())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] is not open!");
		return;
	}
	QByteArray blockArray;
	//numberOfBlockCopied	= 0;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start the copy");
	emit readIsStarted();
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	stat=Idle;
	#endif
	if(stopIt)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] stopIt == true, then quit");
		internalClose();
		return;
	}
	do
	{
		//read one block
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Read;
		#endif
		blockArray=file.read(blockSize);
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		stat=Idle;
		#endif

		if(file.error()!=QFile::NoError)
		{
			errorString_internal=tr("Unable to read the source file: ")+file.errorString()+" ("+QString::number(file.error())+")";
                        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("file.error()!=QFile::NoError: %1, error: %2").arg(QString::number(file.error())).arg(errorString_internal));
			emit error();
                        isInReadLoop=false;
			return;
		}
		sizeReaden=blockArray.size();
		if(sizeReaden>0)
		{
			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			stat=WaitWritePipe;
			#endif
			if(!writeThread->write(blockArray))
			{
				if(!stopIt)
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] stopped because the write is stopped: "+QString::number(lastGoodPosition));
					stopIt=true;
				}
			}

			#ifdef ULTRACOPIER_PLUGIN_DEBUG
			stat=Idle;
			#endif

			if(stopIt)
				break;

			lastGoodPosition+=blockArray.size();

			//wait for limitation speed if stop not query
			if(maxSpeed>0)
			{
				numberOfBlockCopied++;
				if(numberOfBlockCopied>=MultiForBigSpeed)
				{
					numberOfBlockCopied=0;
					waitNewClockForSpeed.acquire();
					if(stopIt)
						break;
				}
			}
                        /*if(lastGoodPosition>size)
                                oversize=lastGoodPosition-size;*/
		}
		/*
		if(lastGoodPosition>16*1024)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Test error in reading: %1 (%2)").arg(file.errorString()).arg(file.error()));
			errorString_internal=QString("Test error in reading: %1 (%2)").arg(file.errorString()).arg(file.error());
			emit error();
			isInReadLoop=false;
			return;
		}
		*/
	}
	while(sizeReaden>0 && !stopIt);
	if(lastGoodPosition>file.size())
	{
		errorString_internal=tr("File truncated during the read, possible data change");
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] "+QString("Source truncated during the read: %1 (%2)").arg(file.errorString()).arg(QString::number(file.error())));
		emit error();
		isInReadLoop=false;
		return;
	}
	isInReadLoop=false;
	if(stopIt)
	{
		if(putInPause)
			emit isInPause();
		stopIt=false;
		return;
	}
	emit readIsStopped();//will product by signal connection writeThread->endIsDetected();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] stop the read");
}

void ReadThread::startRead()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
        if(tryStartRead)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] already in try start");
                return;
        }
	if(isInReadLoop)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] double event dropped");
	else
        {
                tryStartRead=true;
		emit internalStartRead();
        }
}

void ReadThread::internalClose(bool callByTheDestructor)
{
	/// \note never send signal here, because it's called by the destructor
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	if(!fakeMode)
		file.close();
	if(!callByTheDestructor)
		emit closed();

	/// \note always the last of this function
	if(!fakeMode)
		isOpen.release();
}

/** \brief set block size
\param block the new block size in KB
\return Return true if succes */
bool ReadThread::setBlockSize(const int blockSize)
{
	if(blockSize<1 || blockSize>16384)
	{
		this->blockSize=blockSize*1024;
		//set the new max speed because the timer have changed
		setMaxSpeed(maxSpeed);
		return true;
	}
	else
		return false;
}

/*! \brief Set the max speed
\param tempMaxSpeed Set the max speed in KB/s, 0 for no limit */
int ReadThread::setMaxSpeed(const int maxSpeed)
{
	if(this->maxSpeed==0 && maxSpeed==0 && waitNewClockForSpeed.available()>0)
		waitNewClockForSpeed.tryAcquire(waitNewClockForSpeed.available());
	this->maxSpeed=maxSpeed;
	if(this->maxSpeed>0)
	{
		int NewInterval,newMultiForBigSpeed=0;
		do
		{
			newMultiForBigSpeed++;
			NewInterval=(blockSize*newMultiForBigSpeed)/(this->maxSpeed);
		}
		while (NewInterval<ULTRACOPIER_PLUGIN_MINTIMERINTERVAL);
		if(NewInterval>ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL)
		{
			NewInterval=ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL;
			newMultiForBigSpeed=1;
			blockSize=this->maxSpeed*NewInterval;
		}
		MultiForBigSpeed=newMultiForBigSpeed;
		return NewInterval;
	}
	else
	{
		waitNewClockForSpeed.release();
		return 0;
	}
}

/// \brief For give timer every X ms
void ReadThread::timeOfTheBlockCopyFinished()
{
	if(waitNewClockForSpeed.available()<ULTRACOPIER_PLUGIN_NUMSEMSPEEDMANAGEMENT)
		waitNewClockForSpeed.release();
	//why not just use waitNewClockForSpeed.release() ?
}

/// \brief do the fake open
void ReadThread::fakeOpen()
{
	fakeMode=true;
	emit opened();
}

/// \brief do the fake writeIsStarted
void ReadThread::fakeReadIsStarted()
{
	emit readIsStarted();
}

/// \brief do the fake writeIsStopped
void ReadThread::fakeReadIsStopped()
{
	emit readIsStopped();
}

/// do the checksum
void ReadThread::startCheckSum()
{
	emit internalStartChecksum();
}

qint64 ReadThread::getLastGoodPosition()
{
	if(lastGoodPosition>file.size())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"["+QString::number(id)+"] Bug, the lastGoodPosition is greater than the file size!");
		return file.size();
	}
	else
		return lastGoodPosition;
}

//reopen after an error
void ReadThread::reopen()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
        if(isInReadLoop)
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] try reopen where read is not finish");
                return;
        }
	stopIt=true;
	emit internalStartReopen();
}

bool ReadThread::internalReopen()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	stopIt=false;
	file.close();
        if(size_at_open!=file.size() && mtime_at_open!=QFileInfo(file).lastModified())
        {
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"["+QString::number(id)+"] source file have changed since the last open, restart all");
                //fix this function like the close function
                lastGoodPosition=0;
                if(internalOpen(false))
                {
                        emit resumeAfterErrorByRestartAll();
                        return true;
                }
                else
                        return false;
        }
	else
        {
                //fix this function like the close function
                if(internalOpen(false))
                {
                        emit resumeAfterErrorByRestartAtTheLastPosition();
                        return true;
                }
                else
                        return false;
        }
}

//set the write thread
void ReadThread::setWriteThread(WriteThread * writeThread)
{
	this->writeThread=writeThread;
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void ReadThread::setId(int id)
{
	this->id=id;
}
#endif

void ReadThread::seekToZeroAndWait()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	stopIt=true;
	seekToZero=true;
	emit checkIfIsWait();
}

void ReadThread::isInWait()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"["+QString::number(id)+"] start");
	if(seekToZero)
	{
		seekToZero=false;
		if(file.isOpen())
			seek(0);
		else
			internalOpen(true);
		emit isSeekToZeroAndWait();
	}
}

bool ReadThread::isReading()
{
        return isInReadLoop;
}

