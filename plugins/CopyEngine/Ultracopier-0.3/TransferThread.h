#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QList>
#include <QStringList>
#include <QDateTime>
#include <QDir>

#include "ReadThread.h"
#include "WriteThread.h"
#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"

class TransferThread : public QThread
{
	Q_OBJECT
public:
	explicit TransferThread();
	~TransferThread();
	enum TransferStat
	{
		Idle=0,
		PreOperation=1,
		WaitForTheTransfer=2,
		Transfer=3,
		PostTransfer=4,
		PostOperation=5
	};
	TransferStat getStat();
	//get drive of an file or folder
	QString getDrive(QString fileOrFolder);
	//set drive list, used in getDrive()
	void setDrive(QStringList drives);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	//to set the id
	void setId(int id);
	QChar readingLetter();
	QChar writingLetter();
	#endif
protected:
	void run();
signals:
	//to send state
	void preOperationStopped();
	void checkIfItCanBeResumed();
	//void transferStarted();//not sended (and not used then)
	void readStopped();
	void writeStopped();
	void postOperationStopped();
	//get dialog
	void fileAlreadyExists(QFileInfo,QFileInfo,bool isSame);
	void errorOnFile(QFileInfo,QString);
	//internal signal
	void internalStartPostOperation();
	void internalStartPreOperation();
	void internalStartResumeAfterErrorAndSeek();
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	void tryPutAtBottom();
	//force into the right thread
	void internalTryStartTheTransfer();
public slots:
	void startTheTransfer();
	void setFiles(const QString &source,const qint64 &size,const QString &destination,const CopyMode &mode);
	void setFileExistsAction(const FileExistsAction &action);
	void setFileRename(const QString &nameForRename);
	void setAlwaysFileExistsAction(const FileExistsAction &action);
	//set the copy info and options before runing
	void setRightTransfer(const bool doRightTransfer);
	//set keep date
	void setKeepDate(const bool keepDate);
	//set the current max speed in KB/s
	void setMaxSpeed(int maxSpeed);
	//set block size in KB
	bool setBlockSize(const unsigned int blockSize);
	//pause the copy
	void pause();
	//resume the copy
	void resume();
	//stop the copy
	void stop();
	//skip the copy
	void skip();
	//retry after error
	void retryAfterError();
	//return info about the copied size
	qint64 copiedSize();
	//put the current file at bottom
	void putAtBottom();
private slots:
	void preOperation();
	void readIsReady();
	void writeIsReady();
	void readIsFinish();
	void readIsClosed();
	void writeIsClosed();
	void postOperation();
	void getWriteError();
	void getReadError();
	//void syncAfterErrorAndReadFinish();
	void readThreadIsSeekToZeroAndWait();
	void writeThreadIsReopened();
        void readThreadResumeAfterError();
        //to filter the emition of signal
	void readIsStopped();
	void writeIsStopped();
	//speed limitation
	void timeOfTheBlockCopyFinished();
	//force into the right thread
	void internalStartTheTransfer();
private:
	enum MoveReturn
	{
		MoveReturn_skip=0,
		MoveReturn_moved=1,
		MoveReturn_error=2
	};
	TransferStat		stat;
	ReadThread		readThread;
	WriteThread		writeThread;
	QString			source;
	QString			destination;
	CopyMode		mode;
	QTimer			clockForTheCopySpeed;	///< For the speed throttling
	bool			doRightTransfer;
	bool			keepDate;
	bool			readIsReadyVariable;
	bool			writeIsReadyVariable;
	bool			readIsOpenVariable;
	bool			writeIsOpenVariable;
	bool			readIsFinishVariable;
	bool			writeIsFinishVariable;
	bool			readIsClosedVariable;
	bool			writeIsClosedVariable;
	volatile bool		stopIt;
	volatile bool		canStartTransfer;
	int			blockSize;
	bool			retry;
	QFileInfo		sourceInfo;
	QFileInfo		destinationInfo;
	QStringList		mountSysPoint;
	qint64			size;
	FileExistsAction	fileExistsAction;
	FileExistsAction	alwaysDoFileExistsAction;
	bool			needSkip,needRemove;
	int			id;
	//error management
	bool			writeError,writeError_source_seeked,writeError_destination_reopened;
	bool			readError;
	//different pre-operation
	bool isSame();
	bool destinationExists();
	MoveReturn isMovedDirectly();
	void ifCanStartTransfer();
	//fonction to edit the file date time
	bool changeFileDateTime(const QString &source,const QString &destination);
	void resetExtraVariable();
	//error management function
	void resumeTransferAfterWriteError();
	//to send state
	bool sended_state_preOperationStopped;
        bool sended_state_readStopped;
	bool sended_state_writeStopped;
        //different post-operation
	bool checkIfAllIsClosed();
	bool doFilePostOperation();
};

#endif // TRANSFERTHREAD_H
