/** \file ListThread.h
\brief Define the list thread, and management to the action to do
\author alpha_one_x86
\version 0.3
\date 2011 */

#ifndef LISTTHREAD_H
#define LISTTHREAD_H

#include <QThread>
#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QSemaphore>
#include <QTextStream>
#include <QFile>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "scanFileOrFolder.h"
#include "TransferThread.h"
#include "MkPath.h"
#include "RmPath.h"
#include "Environment.h"

/// \brief Define the list thread, and management to the action to do
class ListThread : public QThread
{
	Q_OBJECT
public:
	explicit ListThread(FacilityInterface * facilityInterface);
	~ListThread();
	//duplication copy detection
	/** \brief compare the current sources of the copy, with the passed arguments
	 * \param sources the sources list to compares with the current sources list
	 * \return true if have same sources, else false (or empty) */
	bool haveSameSource(QStringList sources);
	/** \brief compare the current destination of the copy, with the passed arguments
	 * \param destination the destination to compares with the current destination
	 * \return true if have same destination, else false (or empty) */
	bool haveSameDestination(QString destination);
	//external soft like file browser have send copy/move list to do
	/** \brief send copy with destination
	 * \param sources the sources list to copy
	 * \param destination the destination to copy
	 * \return true if the copy have been accepted */
	bool newCopy(QStringList sources,QString destination);
	/** \brief send move without destination, ask the destination
	 * \param sources the sources list to move
	 * \param destination the destination to move
	 * \return true if the move have been accepted */
	bool newMove(QStringList sources,QString destination);
	/** \brief get the speed limitation
	 * < -1 if not able, 0 if disabled */
	qint64 getSpeedLimitation();
	/** \brief to set drives detected
	 * specific to this copy engine */
	void setDrive(QStringList drives);
	/// \brief to set the collision action
	void setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists);
	/// \brief get action type
	enum ActionType
	{
		ActionType_Transfer=0,
		ActionType_MkPath=1,
		ActionType_RmPath=2
	};
	/// \brief to store one action to do
	struct actionToDo
	{
		ActionType type;///< \see ActionType
		quint64 id;
		qint64 size;///< Used to set: used in case of transfer or remainingInode for drop folder
		QFileInfo source;///< Used to set: source for transfer, folder to create, folder to drop
		QFileInfo destination;
		CopyMode mode;
		bool isRunning;///< store if the action si running
		//TransferThread * transfer; // -> see transferThreadList
	};
	QList<actionToDo> actionToDoList;
	int				numberOfInodeOperation;
	//dir operation thread queue
	MkPath mkPathQueue;
	RmPath rmPathQueue;
	//to get the return value from copyEngine
	bool getReturnBoolToCopyEngine();
	QPair<quint64,quint64> getReturnPairQuint64ToCopyEngine();
	returnSpecificFileProgression getReturnSpecificFileProgressionToCopyEngine();
	QList<returnActionOnCopyList> getReturnActionOnListToCopyEngine();
	QList<ItemOfCopyList> getReturnListItemOfCopyListToCopyEngine();
	ItemOfCopyList getReturnItemOfCopyListToCopyEngine();
public slots:
	//get information about the copy
	/** \brief to get the general progression
	 * first = current transfered byte, second = byte to transfer */
	void getGeneralProgression();
	/** \brief to get the progression for a specific file
	 * \param id the id of the transfer, id send during population the transfer list
	 * first = current transfered byte, second = byte to transfer */
	void getFileProgression(const quint64 &id);
	/** \brief to get the action in waiting on the transfer list */
	void getActionOnList();
	//transfer list
	/** \brief to get the transfer list
	 * Used when the interface is changed, useful to minimize the memory size */
	void getTransferList();
	/** \brief to get one transfer info
	 * Used by the interface which show multiple transfer */
	void getTransferListEntry(const quint64 &id);
	//action on the copy
	/// \brief put the transfer in pause
	void pause();
	/// \brief resume the transfer
	void resume();
	/** \brief skip one transfer entry
	 * \param id id of the file to remove */
	void skip(const quint64 &id);
	/** \brief skip as interanl one transfer entry
	 * \param id id of the file to remove */
	bool skipInternal(const quint64 &id);
	/// \brief cancel all the transfer
	void cancel();
	//edit the transfer list
	/** \brief remove the selected item
	 * \param ids ids is the id list of the selected items */
	void removeItems(const QList<int> &ids);
	/** \brief move on top of the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsOnTop(const QList<int> &ids);
	/** \brief move up the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsUp(const QList<int> &ids);
	/** \brief move down the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsDown(const QList<int> &ids);
	/** \brief move on bottom of the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsOnBottom(const QList<int> &ids);
	/// \brief export the transfer list into a file
	void exportTransferList(const QString &fileName);
	/// \brief import the transfer list into a file
	void importTransferList(const QString &fileName);
	/// \brief set the folder local colision
	void setFolderColision(FolderExistsAction alwaysDoThisActionForFolderExists);
	/** \brief to set the speed limitation
	 * -1 if not able, 0 if disabled */
	bool setSpeedLimitation(const qint64 &speedLimitation);
	/// \brief set the copy info and options before runing
	void setRightTransfer(const bool doRightTransfer);
	/// \brief set keep date
	void setKeepDate(const bool keepDate);
	/// \brief set block size in KB
	void setBlockSize(const int blockSize);
	/// \brief set auto start
	void setAutoStart(const bool autoStart);
	/// \brief set check destination folder
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
	/// \brief set data local to the thread
	void setAlwaysFileExistsAction(FileExistsAction alwaysDoThisActionForFileExists);
	/// \brief do new actions, start transfer
	void doNewActions_start_transfer();
	/// \brief do new actions, do the inode manipulation
	void doNewActions_inode_manipulation();
	/// \brief restart transfer if it can
	void restartTransferIfItCan();
private:
	QSemaphore mkpathTransfer;
	QString sourceDrive;
	bool sourceDriveMultiple;
	bool stopIt;
	QString destinationDrive;
	bool destinationDriveMultiple;
	QList<scanFileOrFolder *> scanFileOrFolderThreadsPool;
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
	//to return value to the copyEngine
	bool returnBoolToCopyEngine;
	QPair<quint64,quint64> returnPairQuint64ToCopyEngine;
	returnSpecificFileProgression returnSpecificFileProgressionToCopyEngine;
	QList<returnActionOnCopyList> returnActionOnListToCopyEngine;
	QList<ItemOfCopyList> returnListItemOfCopyListToCopyEngine;
	ItemOfCopyList returnItemOfCopyListToCopyEngine;
private slots:
	void scanThreadHaveFinish(bool skipFirstRemove=false);
	void updateTheStatus();
	void folderTransfer(const QString &source,const QString &destination,const int &numberOfItem,const CopyMode &mode);
	void fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo,const CopyMode &mode);
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

	void warningTransferList(QString warning);
	void errorTransferList(QString error);
};

#endif // LISTTHREAD_H
