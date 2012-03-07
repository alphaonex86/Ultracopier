/***************************************************************************
                                  readThread.h
                              -------------------
 
     Begin        : Fri Dec 21 2007 22:42 alpha_one_x86
     Project      : Ultracopier
     Email        : ultracopier@first-world.info
     Note         : See README for copyright and developer
     Target       : Define the class of the readThread
 
****************************************************************************/

/** \file readThread.h
\brief Thread changed to open/close and read the source file
\author alpha_one_x86
\version 0.3
\date 2011 */

#include <QObject>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QWaitCondition>
#include <QMutex>
#include <QTimer>
#include <QTime>
#include <QByteArray>
#include <QMutexLocker>

#include "writeThread.h"
#include "Environment.h"

#ifdef DEBUG_WINDOW
#include "debugDialog.h"
#endif // DEBUG_WINDOW

#define STOPORPAUSEMACRO if(stopIt){goto SkipFile;};waitInPauseIfNeeded();if(stopIt){goto SkipFile;};
#define STOPORPAUSEMACROPREOPEN if(stopIt){return;};waitInPauseIfNeeded();if(stopIt){return;};

#ifndef READ_THREAD_H
#define READ_THREAD_H

/** \brief Thread for do the copy.

This thread do the copy, it store the permanently options, and use shared memory for get and set the variable, and use signal/slot for the events.
*/
class readThread : public QThread
{
	Q_OBJECT
public slots:
	//set the copy info and options before runing
	void setRightTransfer(const bool doRightTransfer);
	//set keep date
	void setKeepDate(const bool keepDate);
	//set the current max speed in KB/s
	void setMaxSpeed(int tempMaxSpeed);
	//set prealoc file size
	void setPreallocateFileSize(const bool prealloc);
	//set block size in KB
	bool setBlockSize(const int blockSize);
	//pause the copy
	void pauseTransfer();
	//resume the copy
	void resumeTransfer();
	//stop the current copy
	void stopTheTransfer();
	//skip the current file
	void skipCurrentTransfer(quint64 id);
	//action on item into the list
	void removeItems(QList<int> ids);
	void moveItemsOnTop(QList<int> ids);
	void moveItemsUp(QList<int> ids);
	void moveItemsDown(QList<int> ids);
	void moveItemsOnBottom(QList<int> ids);
public:
	//the constructor
	readThread();
	//the destructor
	~readThread();
	//append to action/transfer list
	quint64 addToMkPath(const QDir& folder);
	quint64 addToTransfer(const QFileInfo& source,const QFileInfo& destination,const CopyMode& mode);
	void addToRmPath(const QDir& folder,const int& inodeToRemove);
	QPair<quint64,quint64> getGeneralProgression();
	returnSpecificFileProgression getFileProgression(const quint64& id);
	quint64 realByteTransfered();
	int lenghtOfCopyList();
	QList<returnActionOnCopyList> getActionOnList();
	QList<ItemOfCopyList> getTransferList();
	ItemOfCopyList getTransferListEntry(quint64 id);
	//fonction to return the correct code and unlock the wait
	void setFileExistsAction(FileExistsAction action,QString newName="");
	void setFileErrorAction(FileErrorAction action);
	void setFileErrorActionInWriteThread(FileErrorAction action,int index);
	bool haveContent();
	bool isFinished();
	QString getDrive(QString fileOrFolder);
	void setDrive(QStringList drives);
signals:
	//signal to internal parsing
	void queryNewWriteThread();
	void queryStartWriteThread(int);
	//to the messages
	void fileAlreadyExists(QFileInfo,QFileInfo,bool);
	void errorOnFile(QFileInfo,QString,bool);
	void errorOnFileInWriteThread(QFileInfo,QString,int indexOfWriteThread);
	//other
	void transferIsInPause(bool);
	void transferIsStarted(bool);
	void newTransferStart(ItemOfCopyList);//should update interface information on this event
	void newTransferStop(quint64 id);//should update interface information on this event
	void newActionOnList();
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	void cancelAll();
	//for the extra logging
	void rmPath(QString path);
	void mkPath(QString path);
protected:
	void run();
	void stop();
private:
	volatile bool stopped;
	//list to action to create folder
	struct toMkPath
	{
		quint64 id;
		QDir path;
	};
	QList<toMkPath> listMkPath;
	//list to do to file transfer
	struct toTransfer
	{
		quint64 id;
		quint64 size;
		QFileInfo source;
		QFileInfo destination;
		CopyMode mode;
		writeThread * thread;
	};
	QList<toTransfer> listTransfer;
	//list of folder to remove
	struct toRmPath
	{
		QDir path;
		int remainingInode;
		quint64 id;
		bool movedToEnd;
	};
	QList<toRmPath> listRmPath;
	volatile quint64 bytesToTransfer;
	volatile quint64 bytesTransfered;
	quint64 idIncrementNumber;
	quint64 generateIdNumber();
	quint64 currentPositionInSource;
	quint64 currentTransferedSize;
	quint64 currentTransferedSizeOversize;
	quint64 totalTransferListSize;
	//check if need wait write thread
	void checkIfNeedWaitWriteThread();
	
	//internal variable
	volatile quint64 currentReadTransferId;
	volatile int	maxSpeed;		///< The max speed in KB/s, 0 for no limit
	volatile bool	doRightTransfer;	///< Do rights copy
	volatile bool	preallocation;		///< Flag for know if need preallocation
	volatile bool	keepDate;		///< To keep date file
	volatile bool	isInPause;		///< To know if the thread is in pause
	volatile int	MultiForBigSpeed;
	QTimer		clockForTheCopySpeed;	///< For the speed throttling
	//boolean to do atomic action
	volatile bool	needInPause;		///< To put in pause
	volatile bool	skipThecurrentFile;	///< Skip the current copying file
	volatile bool	stopIt;			///< Stop the read thread if is running
	volatile bool	removeDestination;
	/** Variable because should be editable to put big block size when speed is low
	 * to keep time throttling quickly and keep the interface reactive */
	volatile int	blockSizeCurrent;
	QSemaphore	waitNewWriteThread;		///< For wait the new write thread
	volatile int	multiplicatorForBigSpeed;	///< Multiple for count the number of block needed
	volatile int	numberOfBlockCopied;		///< Multiple for count the number of block copied
	QMutex		MultiThreadTransferList;	///< For be thread safe while operate on copy list
	QMutex		MultiThreadWriteThreadList;	///< For be thread safe while operate with write thread
	QMutex		dialogActionMutex;		///< For be thread safe while operate with dialog
	QList<writeThread*>	theWriteThreadList;	///< For store the write thread list
	/** \brief to pass variable to 2 method to create the thread
	  * The writeThread is created by the main thread, the blocking methode wait this creation
	  * When it's created, then this variable is used to pass the variable and unblock the other fonction */
	writeThread*	theNewCreatedWriteThread;
	QSemaphore	waitNewClockForSpeed;
	QSemaphore	waitErrorAction;
	QSemaphore	waitFileExistsAction;
	QSemaphore	waitInPause;
	QSemaphore	waitWriteThread;
	QSemaphore	writeThreadSem;///< ???
        QSemaphore	waitCanDestroyGuiSemaphore;		///< For wait the new write thread
	/** This method return the action really do on the action list to do it on the interface */
	QList<returnActionOnCopyList> actionDone;
	//to wait and return action
	FileExistsAction waitNeedFileExistsAction(QFileInfo fileInfoSource,QFileInfo fileInfoDestination,bool isSame=false);		//should never return the code NotSet
	FileErrorAction waitNeedFileErrorAction(QFileInfo fileInfo,QString errorString,bool canPutAtTheEndOfList=true);//should never return the code NotSet
	//other method
	void putFirstFileAtEnd();
	void waitInPauseIfNeeded();
	//to clean dir
	bool removeFolderIfNeeded(QDir basePath,bool needDecreaseNumberOfInode=true);
	//dialog managing
	bool dialogInWait;
	struct WriteThreadErrorStruct
	{
		QFileInfo fileInfo;
		QString errorString;
		int index;
	};
	QList<WriteThreadErrorStruct> WriteThreadErrorList;
	struct FileExistsActionStruct
	{
		QFileInfo fileInfoSource;
		QFileInfo fileInfoDestination;
		QSemaphore * waitSemaphore;
		bool isSame;
		FileExistsAction * action;
	};
	QList<FileExistsActionStruct> FileExistsActionList;
	struct FileErrorActionStruct
	{
		QFileInfo fileInfo;
		QString errorString;
		QSemaphore * waitSemaphore;
		bool canPutAtTheEndOfList;
		FileErrorAction * action;
	};
	QList<FileErrorActionStruct> FileErrorActionList;
	void emitSignalToShowDialog();
	QStringList mountSysPoint;
	QString newName;
	QSemaphore waitOutVariable;
	void removeItemInTransferList(quint64 id,bool skipIfIsRunning,bool checkWriteThread=true);
	quint64 actualRealByteTransfered;
	#ifdef DEBUG_WINDOW
	debugDialog theDebugDialog;
	QTimer updateDialogTimer;
	volatile bool isBlocked;
	#endif // DEBUG_WINDOW
private slots:
	/// \brief create new  write thread object
	void createNewWriteThread();
	/// \brief write thread have finish
	void newWriteThreadFinish();
	/// \brief call when the write thread have end their task
	void writeThreadOperationFinish();
	void writeThreadOperationStart();
	/// \brief for restart write thread which have been stop
	void startWriteThread(int id);
	void manageTheWriteThreadError(QFileInfo,QString);
	void pre_operation();
	void post_operation();
	bool writeThreadIsFinish();
	void timeOfTheBlockCopyFinished();
	#ifdef DEBUG_WINDOW
	void updateDebugInfo();
	#endif // DEBUG_WINDOW
};

#endif // READ_THREAD_H
