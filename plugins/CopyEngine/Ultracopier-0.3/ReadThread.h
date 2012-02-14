#ifndef READTHREAD_H
#define READTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QSemaphore>
#include <QFile>
#include <QTimer>
#include <QDateTime>
#include <QFileInfo>

#include "WriteThread.h"
#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"

class ReadThread : public QThread
{
	Q_OBJECT
public:
	explicit ReadThread();
	~ReadThread();
protected:
	void run();
public:
	void open(const QString &name,const CopyMode &mode);
	QString errorString();
	//QByteArray read(qint64 position,qint64 maxSize);
	void stop();
	bool pause();
	void resume();
	qint64 size();
	qint64 getLastGoodPosition();
	void startRead();
	//set the current max speed in KB/s
	int setMaxSpeed(int maxSpeed);
	//set block size in KB
	bool setBlockSize(const int blockSize);
	//reopen after an error
        void reopen();
	//set the write thread
	void setWriteThread(WriteThread * writeThread);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	//to set the id
	void setId(int id);
	//stat
	enum ReadStat
	{
		Idle=0,
		InodeOperation=1,
		Read=2,
		WaitWritePipe=3
	};
	ReadStat stat;
	#endif
        bool isReading();
	void timeOfTheBlockCopyFinished();
public slots:
	void seekToZeroAndWait();
signals:
	void error();
	void isInPause();
	void opened();
	void readIsStarted();
	void readIsStopped();
	void closed();
	void isSeekToZeroAndWait();
	void checkIfIsWait();
	void resumeAfterErrorByRestartAll();
	void resumeAfterErrorByRestartAtTheLastPosition();
        // internal signals
        void internalStartOpen();
        void internalStartReopen();
        void internalStartRead();
        void internalStartClose();
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
private:
	QString		name;
	QString		errorString_internal;
	QFile		file;
	volatile bool	stopIt;
	CopyMode	mode;
	qint64		lastGoodPosition;
	volatile int	blockSize;
	volatile int	maxSpeed;		///< The max speed in KB/s, 0 for no limit
	QSemaphore	waitNewClockForSpeed;
	volatile int	numberOfBlockCopied;		///< Multiple for count the number of block copied
	volatile int	multiplicatorForBigSpeed;	///< Multiple for count the number of block needed
	volatile int	MultiForBigSpeed;
	WriteThread*	writeThread;
	int		id;
	QSemaphore	isOpen;
	volatile bool	putInPause;
	volatile bool	isInReadLoop;
	volatile bool	seekToZero;
        volatile bool	tryStartRead;
        qint64		size_at_open;
	QDateTime	mtime_at_open;
        //internal function
        bool seek(qint64 position);/// \todo search if is use full
private slots:
	bool internalOpen(bool resetLastGoodPosition=true);
	bool internalReopen();
	void internalRead();
	void internalClose(bool callByTheDestructor=false);
	void postOperation();
	void isInWait();
};

#endif // READTHREAD_H
