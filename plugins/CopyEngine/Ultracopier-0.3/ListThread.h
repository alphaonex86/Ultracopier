#ifndef LISTTHREAD_H
#define LISTTHREAD_H

#include <QThread>
#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QSemaphore>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "scanFileOrFolder.h"
#include "TransferThread.h"
#include "MkPath.h"
#include "RmPath.h"
#include "Environment.h"

class ListThread : public QThread
{
	Q_OBJECT
public:
	explicit ListThread(FacilityInterface * facilityInterface);
	~ListThread();
	//duplication copy detection
	bool haveSameSource(QStringList sources);
	bool haveSameDestination(QString destination);
	//external soft like file browser have send copy/move list to do
	bool newCopy(QStringList sources,QString destination);
	bool newMove(QStringList sources,QString destination);
	//get information about the copy
	QPair<quint64,quint64> getGeneralProgression();
	returnSpecificFileProgression getFileProgression(quint64 id);
	QList<returnActionOnCopyList> getActionOnList();
	quint64 realByteTransfered();		//real size transfered to right speed calculation
	//speed limitation
	qint64 getSpeedLimitation();
	//transfer list
	QList<ItemOfCopyList> getTransferList();
	ItemOfCopyList getTransferListEntry(quint64 id);
	void setDrive(QStringList drives);
	//action
	void setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists);
	//structure
	enum ActionType
	{
		ActionType_Transfer=0,
		ActionType_MkPath=1,
		ActionType_RmPath=2
	};
	struct actionToDo
	{
		ActionType type;
		quint64 id;
		qint64 size;///< Used to set: used in case of transfer or remainingInode for drop folder
		QFileInfo source;///< Used to set: source for transfer, folder to create, folder to drop
		QFileInfo destination;
		CopyMode mode;
		bool isRunning;
		//TransferThread * transfer; // -> see transferThreadList
	};
	QList<actionToDo> actionToDoList;
	int				numberOfInodeOperation;
	//dir operation thread queue
	MkPath mkPathQueue;
	RmPath rmPathQueue;
	//set the folder local colision
	void setFolderColision(FolderExistsAction alwaysDoThisActionForFolderExists);
public slots:
	//action on the copy
	void pause();
	void resume();
	void skip(quint64 id);
	bool skipInternal(quint64 id);//internal skip
	void cancel();
	//edit the transfer list
	void removeItems(QList<int> ids);
	void moveItemsOnTop(QList<int> ids);
	void moveItemsUp(QList<int> ids);
	void moveItemsDown(QList<int> ids);
	void moveItemsOnBottom(QList<int> ids);
	//speed limitation
	bool setSpeedLimitation(qint64 speedLimitation);
	//set the copy info and options before runing
	void setRightTransfer(const bool doRightTransfer);
	//set keep date
	void setKeepDate(const bool keepDate);
	//set block size in KB
	void setBlockSize(const int blockSize);
	//set auto start
	void setAutoStart(const bool autoStart);
	//set check destination folder
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
	//set data local to the thread
	void setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists);
	//do new actions
	void doNewActions_start_transfer();
	void doNewActions_inode_manipulation();
	//restart transfer if it can
	void restartTransferIfItCan();
private:
	QSemaphore mkpathTransfer;
	QString sourceDrive;
	bool sourceDriveMultiple;
	bool stopIt;
	QString destinationDrive;
	bool destinationDriveMultiple;
	struct scanFileOrFolderThread
	{
		scanFileOrFolder * thread;
		CopyMode mode;
	};
	QList<scanFileOrFolderThread> scanFileOrFolderThreadsPool;
	int numberOfTransferIntoToDoList;
	//list of transfer thread
	struct transfer
	{
		TransferThread * thread;
		quint64 id;
		qint64 size;
	};//for quick current running search
	QList<transfer>			transferThreadList;
	scanFileOrFolder *		newScanThread(CopyMode mode);
	quint64				bytesToTransfer;
	quint64				bytesTransfered;
	bool				autoStart;
	bool				putInPause;
	QList<returnActionOnCopyList>	actionDone;///< to action to send to the interface
	quint64				idIncrementNumber;///< to store the last id returned
	qint64				actualRealByteTransfered;
	int				preOperationNumber;
	int				numberOfTranferRuning;
	QList<quint64>			orderStarted;///< list the order id
	int				maxSpeed;
	FolderExistsAction		alwaysDoThisActionForFolderExists;
	bool				checkDestinationFolderExists;
	//mk path to do
	quint64 addToMkPath(const QDir& folder);
	//add file transfer to do
	quint64 addToTransfer(const QFileInfo& source,const QFileInfo& destination,const CopyMode& mode);
	//add rm path to do
	void addToRmPath(const QDir& folder,const int& inodeToRemove);
	//generate id number
	quint64 generateIdNumber();
	//warning the first entry is accessible will copy
	bool removeItems(quint64 id);
	//put on top
	bool moveOnTopItem(quint64 id);
	//move up
	bool moveUpItem(quint64 id);
	//move down
	bool moveDownItem(quint64 id);
	//put on bottom
	bool moveOnBottomItem(quint64 id);
	//debug windows if needed
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	QTimer timerUpdateDebugDialog;
	#endif
	FacilityInterface * facilityInterface;
	//temp variable for not always alocate the memory
	int int_for_loop,int_for_internal_loop,loop_size,lopp_sub_size,number_rm_path_moved;
	TransferThread *temp_transfer_thread;
	bool isFound;
	bool updateTheStatus_listing,updateTheStatus_copying;
	EngineActionInProgress updateTheStatus_action_in_progress;
	QSemaphore waitConstructor,waitCancel;
	//memory variable for transfer thread creation
	bool doRightTransfer;
	bool keepDate;
	int blockSize;
	QStringList drives;
	FileExistsAction alwaysDoThisActionForFileExists;
private slots:
	void scanThreadHaveFinish(bool skipFirstRemove=false);
	void updateTheStatus();
	void folderTransfer(const QString &source,const QString &destination,const int &numberOfItem);
	void fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo);
	//mkpath event
	void mkPathFirstFolderFinish();
	//rmpath event
	void rmPathFirstFolderFinish();
	//transfer is finished
	void transferIsFinished();
	//put the current file at bottom
	void transferPutAtBottom();
	//transfer is finished
	void transferInodeIsClosed();
	//debug windows if needed
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	void timedUpdateDebugDialog();
	#endif
	//dialog message
	/// \note Can be call without queue because all call will be serialized
	void fileAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame);
	/// \note Can be call without queue because all call will be serialized
	void errorOnFile(const QFileInfo &fileInfo,const QString &errorString);
	/// \note Can be call without queue because all call will be serialized
	void folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame);
	/// \note Can be call without queue because all call will be serialized
	void errorOnFolder(const QFileInfo &fileInfo,const QString &errorString);
	//to run the thread
	void run();
	/// \to create transfer thread
	void createTransferThread();
signals:
        //send information about the copy
        void actionInProgess(EngineActionInProgress);	//should update interface information on this event

        void newTransferStart(ItemOfCopyList);		//should update interface information on this event
        void newTransferStop(quint64 id);		//should update interface information on this event, is stopped, example: because error have occurred, and try later, don't remove the item!

        void newFolderListing(QString path);
        void newCollisionAction(QString action);
        void newErrorAction(QString action);
        void isInPause(bool);

        void newActionOnList();
	void cancelAll();

        //send error occurred
        void error(QString path,quint64 size,QDateTime mtime,QString error);
        //for the extra logging
        void rmPath(QString path);
        void mkPath(QString path);
        /// \brief To debug source
        void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	void updateTheDebugInfo(QStringList,QStringList,int);
	#endif

	//other signal
	/// \note Can be call without queue because all call will be serialized
	void send_fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread);
	/// \note Can be call without queue because all call will be serialized
	void send_errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread);
	/// \note Can be call without queue because all call will be serialized
	void send_folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread);
	/// \note Can be call without queue because all call will be serialized
	void send_errorOnFolder(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread);
	//to close
	void tryCancel();
	//to ask new transfer thread
	void askNewTransferThread();
};

#endif // LISTTHREAD_H
