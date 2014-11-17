/** \file ListThread.h
\brief Define the list thread, and management to the action to do
\author alpha_one_x86
\licence GPL3, see the file COPYING */

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
#include <QTimer>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "ScanFileOrFolder.h"
#include "TransferThread.h"
#include "MkPath.h"
#include "Environment.h"
#include "DriveManagement.h"

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
    bool haveSameSource(const QStringList &sources);
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const QString &destination);
    /// \return empty if multiple or no destination
    QString getUniqueDestinationFolder() const;
    //external soft like file browser have send copy/move list to do
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const QStringList &sources,const QString &destination);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const QStringList &sources,const QString &destination);
    /** \brief to set drives detected
     * specific to this copy engine */
    void setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType);
    /// \brief to set the collision action
    void setCollisionAction(const FileExistsAction &alwaysDoThisActionForFileExists);
    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList();
    /// \brief to store one action to do
    struct ActionToDoTransfer
    {
        quint64 id;
        qint64 size;///< Used to set: used in case of transfer or remainingInode for drop folder
        QFileInfo source;///< Used to set: source for transfer, folder to create, folder to drop
        QFileInfo destination;
        Ultracopier::CopyMode mode;
        bool isRunning;///< store if the action si running
        //TransferThread * transfer; // -> see transferThreadList
    };
    QList<ActionToDoTransfer> actionToDoListTransfer;
    /// \brief to store one action to do
    struct ActionToDoInode
    {
        ActionType type;///< \see ActionType
        quint64 id;
        qint64 size;///< Used to set: used in case of transfer or remainingInode for drop folder
        QFileInfo source;///< Keep to copy the right/date, to remove (for move)
        QFileInfo destination;///< Used to set: folder to create, folder to drop
        bool isRunning;///< store if the action si running
    };
    QList<ActionToDoInode> actionToDoListInode;
    QList<ActionToDoInode> actionToDoListInode_afterTheTransfer;
    int numberOfInodeOperation;
    struct ErrorLogEntry
    {
        QFileInfo source;
        QFileInfo destination;
        QString error;
        Ultracopier::CopyMode mode;
    };
    QList<ErrorLogEntry> errorLog;
    //dir operation thread queue
    MkPath mkPathQueue;
    //to get the return value from copyEngine
    bool getReturnBoolToCopyEngine() const;
    QPair<quint64,quint64> getReturnPairQuint64ToCopyEngine() const;
    Ultracopier::ItemOfCopyList getReturnItemOfCopyListToCopyEngine() const;

    void set_doChecksum(bool doChecksum);
    void set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible);
    void set_checksumOnlyOnError(bool checksumOnlyOnError);
    void set_osBuffer(bool osBuffer);
    void set_osBufferLimited(bool osBufferLimited);
    void autoStartIfNeeded();
public slots:
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
    void moveItemsOnTop(QList<int> ids);
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(QList<int> ids);
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(QList<int> ids);
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(QList<int> ids);

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode);
    /// \brief export the transfer list into a file
    void exportTransferList(const QString &fileName);
    /// \brief import the transfer list into a file
    void importTransferList(const QString &fileName);

    /// \brief set the folder local collision
    void setFolderCollision(const FolderExistsAction &alwaysDoThisActionForFolderExists);
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
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    /// \brief set rsync
    void setRsync(const bool rsync);
    #endif
    /// \brief set check destination folder
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    /// \brief set data local to the thread
    void setAlwaysFileExistsAction(const FileExistsAction &alwaysDoThisActionForFileExists);
    /// \brief do new actions, start transfer
    void doNewActions_start_transfer();
    /** \brief lunch the pre-op or inode op
      1) locate the next next item to do into the both list
        1a) optimisation posible on the mkpath/rmpath
      2) determine what need be lunched
      3) lunch it, rerun the 2)
      */
    void doNewActions_inode_manipulation();
    /// \brief restart transfer if it can
    void restartTransferIfItCan();
    void getNeedPutAtBottom(const QFileInfo &fileInfo, const QString &errorString, TransferThread *thread,const ErrorType &errorType);

    /// \brief update the transfer stat
    void newTransferStat(const TransferStat &stat,const quint64 &id);

    void set_osBufferLimit(const unsigned int &osBufferLimit);
    void set_setFilters(const QList<Filters_rules> &include,const QList<Filters_rules> &exclude);
    void set_sendNewRenamingRules(const QString &firstRenamingRule,const QString &otherRenamingRule);

    //send action done
    void sendActionDone();
    //send progression
    void sendProgression();

    void setTransferAlgorithm(const TransferAlgorithm &transferAlgorithm);
    void setParallelBuffer(int parallelBuffer);
    void setSequentialBuffer(int sequentialBuffer);
    void setParallelizeIfSmallerThan(const unsigned int &parallelizeIfSmallerThan);
    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    void setFollowTheStrictOrder(const bool &followTheStrictOrder);
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setInodeThreads(const int &inodeThreads);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void setCheckDiskSpace(const bool &checkDiskSpace);
    void setCopyListOrder(const bool &order);
    void exportErrorIntoTransferList(const QString &fileName);
private:
    QSemaphore          mkpathTransfer;
    QString             sourceDrive;
    bool                sourceDriveMultiple;
    QString             destinationDrive;
    QString             destinationFolder;
    QStringList         mountSysPoint;
    QList<QStorageInfo::DriveType> driveType;
    bool                destinationDriveMultiple;
    bool                destinationFolderMultiple;
    DriveManagement     driveManagement;

    bool                stopIt;
    QList<ScanFileOrFolder *> scanFileOrFolderThreadsPool;
    int                 numberOfTransferIntoToDoList;
    QList<TransferThread *>		transferThreadList;
    ScanFileOrFolder *		newScanThread(Ultracopier::CopyMode mode);
    quint64				bytesToTransfer;
    quint64				bytesTransfered;
    bool				autoStart;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool                rsync;
    #endif
    bool				putInPause;
    QList<Ultracopier::ReturnActionOnCopyList>	actionDone;///< to action to send to the interface
    quint64				idIncrementNumber;///< to store the last id returned
    qint64				actualRealByteTransfered;
    int                 maxSpeed;///< in KB/s, assume as 0KB/s as default like every where
    FolderExistsAction	alwaysDoThisActionForFolderExists;
    bool				checkDestinationFolderExists;
    bool				doChecksum;
    bool				checksumIgnoreIfImpossible;
    bool				checksumOnlyOnError;
    bool				osBuffer;
    bool				osBufferLimited;
    unsigned int        parallelizeIfSmallerThan;
    bool                moveTheWholeFolder;
    bool                followTheStrictOrder;
    bool                deletePartiallyTransferredFiles;
    int                 sequentialBuffer;
    int                 parallelBuffer;
    int                 inodeThreads;
    bool                renameTheOriginalDestination;
    bool                checkDiskSpace;
    bool                copyListOrder;
    QHash<QString,quint64> requiredSpace;
    unsigned int        putAtBottom;
    unsigned int		osBufferLimit;
    QList<Filters_rules>		include,exclude;
    Ultracopier::CopyMode		mode;
    bool				forcedMode;
    QString				firstRenamingRule;
    QString				otherRenamingRule;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    int                 multiForBigSpeed;
    #endif
    /* here to prevent:
    QObject::killTimer: timers cannot be stopped from another thread
    QObject::startTimer: timers cannot be started from another thread */
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QTimer *clockForTheCopySpeed;	///< For the speed throttling
    #endif

    inline static Ultracopier::ItemOfCopyList actionToDoTransferToItemOfCopyList(const ActionToDoTransfer &actionToDoTransfer);
    //add file transfer to do
    quint64 addToTransfer(const QFileInfo& source,const QFileInfo& destination,const Ultracopier::CopyMode& mode);
    //generate id number
    quint64 generateIdNumber();
    //warning the first entry is accessible will copy
    bool removeSingleItem(const quint64 &id);
    //put on top
    bool moveOnTopItem(const quint64 &id);
    //move up
    bool moveUpItem(const quint64 &id);
    //move down
    bool moveDownItem(const quint64 &id);
    //put on bottom
    bool moveOnBottomItem(const quint64 &id);
    //general transfer
    void startGeneralTransfer();
    //debug windows if needed
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    QTimer timerUpdateDebugDialog;
    #endif
    void detectDrivesOfCurrentTransfer(const QStringList &sources,const QString &destination);
    FacilityInterface * facilityInterface;
    QSemaphore waitConstructor,waitCancel;
    int actionToDoListTransfer_count,actionToDoListInode_count;
    bool doTransfer,doInode;
    qint64 oversize;//used as temp variable
    qint64 currentProgression;
    qint64 copiedSize,totalSize,localOverSize;
    QList<Ultracopier::ProgressionItem> progressionList;
    //memory variable for transfer thread creation
    bool doRightTransfer;
    bool keepDate;
    int blockSize;//in Bytes
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    int blockSizeAfterSpeedLimitation;//in Bytes
    #endif
    QStringList drives;
    FileExistsAction alwaysDoThisActionForFileExists;
    //to return value to the copyEngine
    bool returnBoolToCopyEngine;
    QPair<quint64,quint64> returnPairQuint64ToCopyEngine;
    QList<Ultracopier::ItemOfCopyList> returnListItemOfCopyListToCopyEngine;
    Ultracopier::ItemOfCopyList returnItemOfCopyListToCopyEngine;
    Ultracopier::ProgressionItem tempItem;

    void realByteTransfered();
    int getNumberOfTranferRuning() const;
    bool needMoreSpace() const;
private slots:
    void scanThreadHaveFinishSlot();
    void scanThreadHaveFinish(bool skipFirstRemove=false);
    void autoStartAndCheckSpace();
    void updateTheStatus();
    void fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo,const Ultracopier::CopyMode &mode);
    //mkpath event
    void mkPathFirstFolderFinish();
    /** \brief put the current file at bottom in case of error
    \note ONLY IN CASE OF ERROR */
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
    void errorOnFile(const QFileInfo &fileInfo,const QString &errorString, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(const QFileInfo &fileInfo, const QString &errorString, const ErrorType &errorType);
    //to run the thread
    void run();
    /// \to create transfer thread
    void createTransferThread();
    void deleteTransferThread();
    //mk path to do
    quint64 addToMkPath(const QFileInfo& source, const QFileInfo& destination, const int &inode);
    //add rm path to do
    void addToMovePath(const QFileInfo& source,const QFileInfo& destination, const int& inodeToRemove);
    //add to real move
    void addToRealMove(const QFileInfo& source,const QFileInfo& destination);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    //rsync rm
    void addToRmForRsync(const QFileInfo& destination);
    #endif
    //send the progression, after full reset of the interface (then all is empty)
    void syncTransferList_internal();

    void checkIfReadyToCancel();
signals:
    //send information about the copy
    void actionInProgess(const Ultracopier::EngineActionInProgress &) const;	//should update interface information on this event

    void newActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &) const;///very important, need be temporized to group the modification to do and not flood the interface
    void syncReady() const;

    /** \brief to get the progression for a specific file
     * \param id the id of the transfer, id send during population the transfer list
     * first = current transfered byte, second = byte to transfer */
    void pushFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList) const;
    //get information about the copy
    /** \brief to get the general progression
     * first = current transfered byte, second = byte to transfer */
    void pushGeneralProgression(const quint64 &,const quint64 &) const;

    void newFolderListing(const QString &path) const;
    void isInPause(const bool &) const;

    //when can be deleted
    void canBeDeleted() const;
    void haveNeedPutAtBottom(bool needPutAtBottom,const QFileInfo &fileInfo,const QString &errorString,TransferThread * thread,const ErrorType &errorType) const;

    //send error occurred
    void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error) const;
    void errorToRetry(const QString &source,const QString &destination,const QString &error) const;
    //for the extra logging
    void rmPath(const QString &path) const;
    void mkPath(const QString &path) const;
    /// \brief To debug source
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne) const;
    #endif
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    void updateTheDebugInfo(const QStringList &,const QStringList&,const int &) const;
    #endif

    //other signal
    /// \note Can be call without queue because all call will be serialized
    void send_fileAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame,TransferThread * thread) const;
    /// \note Can be call without queue because all call will be serialized
    void send_errorOnFile(const QFileInfo &fileInfo,const QString &errorString,TransferThread * thread, const ErrorType &errorType) const;
    /// \note Can be call without queue because all call will be serialized
    void send_folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame,ScanFileOrFolder * thread) const;
    /// \note Can be call without queue because all call will be serialized
    void send_errorOnFolder(const QFileInfo &fileInfo,const QString &errorString,ScanFileOrFolder * thread, const ErrorType &errorType) const;
    //send the progression
    void send_syncTransferList() const;
    //mkpath error event
    void mkPathErrorOnFolder(const QFileInfo &fileInfo,const QString &errorString,const ErrorType &errorType) const;
    //to close
    void tryCancel() const;
    //to ask new transfer thread
    void askNewTransferThread() const;

    void warningTransferList(const QString &warning) const;
    void errorTransferList(const QString &error) const;
    void send_sendNewRenamingRules(const QString &firstRenamingRule,const QString &otherRenamingRule) const;
    void send_realBytesTransfered(const quint64 &) const;
    void send_setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType) const;

    void send_setTransferAlgorithm(TransferAlgorithm transferAlgorithm) const;
    void send_parallelBuffer(const int &parallelBuffer) const;
    void send_sequentialBuffer(const int &sequentialBuffer) const;
    void send_parallelizeIfSmallerThan(const int &parallelizeIfSmallerThan) const;
    void missingDiskSpace(QList<Diskspace> list) const;
};

#endif // LISTTHREAD_H
