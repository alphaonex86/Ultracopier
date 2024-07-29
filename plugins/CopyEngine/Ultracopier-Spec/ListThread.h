/** \file ListThread.h
\brief Define the list thread, and management to the action to do
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef LISTTHREAD_H
#define LISTTHREAD_H

#include <QThread>
#include <QObject>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <QSemaphore>
#include <QTextStream>
#include <QFile>
#include <QTimer>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "ScanFileOrFolder.h"
#include "MkPath.h"
#include "Environment.h"
#include "DriveManagement.h"
#include "async/TransferThreadAsync.h"

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
    bool haveSameSource(const std::vector<std::string> &sources);
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const std::string &destination);
    /// \return empty if multiple or no destination
    std::string getUniqueDestinationFolder() const;
    //external soft like file browser have send copy/move list to do
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources,const std::string &destination);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources,const std::string &destination);
    /** \brief to set drives detected
     * specific to this copy engine */
    /// \brief to set the collision action
    void setCollisionAction(const FileExistsAction &alwaysDoThisActionForFileExists);
    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList();
    /// \brief to store one action to do
    struct ActionToDoTransfer
    {
        uint64_t id;
        uint64_t size;///< Used to set: used in case of transfer or remainingInode for drop folder
        #ifdef WIDESTRING
        std::wstring source;///< Used to set: source for transfer, folder to create, folder to drop
        std::wstring destination;
        #else
        std::string source;///< Used to set: source for transfer, folder to create, folder to drop
        std::string destination;
        #endif
        Ultracopier::CopyMode mode;
        bool isRunning;///< store if the action si running
        //TTHREAD * transfer; // -> see transferThreadList
    };
    std::vector<ActionToDoTransfer> actionToDoListTransfer;
    /// \brief to store one action to do
    struct ActionToDoInode
    {
        ActionType type;///< \see ActionType
        uint64_t id;
        int64_t size;///< Used to set: used in case of transfer or remainingInode for drop folder
        #ifdef WIDESTRING
        std::wstring source;///< Keep to copy the right/date, to remove (for move)
        std::wstring destination;///< Used to set: folder to create, folder to drop
        #else
        std::string source;///< Keep to copy the right/date, to remove (for move)
        std::string destination;///< Used to set: folder to create, folder to drop
        #endif
        bool isRunning;///< store if the action si running
    };
    std::vector<ActionToDoInode> actionToDoListInode;
    std::vector<ActionToDoInode> actionToDoListInode_afterTheTransfer;
    int numberOfInodeOperation;
    struct ErrorLogEntry
    {
        std::string source;
        std::string destination;
        std::string error;
        Ultracopier::CopyMode mode;
    };
    std::vector<ErrorLogEntry> errorLog;
    //dir operation thread queue
    MkPath mkPathQueue;
    //to get the return value from copyEngine
    bool getReturnBoolToCopyEngine() const;
    std::pair<quint64,quint64> getReturnPairQuint64ToCopyEngine() const;
    Ultracopier::ItemOfCopyList getReturnItemOfCopyListToCopyEngine() const;
    void setMkFullPath(const bool mkFullPath);
    std::unordered_set<void *> overCheckUsedThread;

    void autoStartIfNeeded();
    /// \brief set auto start
    void setAutoStart(const bool autoStart);
public slots:
    //action on the copy
    /// \brief put the transfer in pause
    void pause();
    /// \brief resume the transfer
    void resume();
    /** \brief skip one transfer entry
     * \param id id of the file to remove */
    void skip(const uint64_t &id);
    /** \brief skip as interanl one transfer entry
     * \param id id of the file to remove */
    bool skipInternal(const uint64_t &id);
    /// \brief cancel all the transfer
    void cancel();
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const std::vector<uint64_t> &ids);
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(std::vector<uint64_t> ids);
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(std::vector<uint64_t> ids);
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(std::vector<uint64_t> ids);
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(std::vector<uint64_t> ids);

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode);
    /// \brief export the transfer list into a file
    void exportTransferList(const std::string &fileName);
    /// \brief import the transfer list into a file
    void importTransferList(const std::string &fileName);

    /// \brief set the folder local collision
    void setFolderCollision(const FolderExistsAction &alwaysDoThisActionForFolderExists);
    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const int64_t &speedLimitation);
    /// \brief set the copy info and options before runing
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    void setOsSpecFlags(bool os_spec_flags);
    void setNativeCopy(bool native_copy);
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
    void getNeedPutAtBottom(const INTERNALTYPEPATH &fileInfo, const std::string &errorString, TransferThreadAsync *thread, const ErrorType &errorType);

    /// \brief update the transfer stat
    void newTransferStat(const TransferStat &stat,const quint64 &id);

    void set_setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude);
    void set_sendNewRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule);
    void set_updateMount();

    //send action done
    void sendActionDone();
    //send progression
    void sendProgression();

    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    void setFollowTheStrictOrder(const bool &followTheStrictOrder);
    void setignoreBlackList(const bool &ignoreBlackList);
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setInodeThreads(const int &inodeThreads);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void setCheckDiskSpace(const bool &checkDiskSpace);
    void setBuffer(const bool &buffer);
    void exportErrorIntoTransferList(const std::string &fileName);
private:
    //can't be static into WriteThread, linked by instance then by ListThread
    QMultiHash<QString,WriteThread *> *writeFileList;
    QMutex       *writeFileListMutex;

    QSemaphore          mkpathTransfer;
    std::string             sourceDrive;
    bool                sourceDriveMultiple;
    std::string             destinationDrive;
    INTERNALTYPEPATH             destinationFolder;
    bool                destinationDriveMultiple;
    bool                destinationFolderMultiple;
    DriveManagement     driveManagement;

    bool                stopIt;
    std::vector<ScanFileOrFolder *> scanFileOrFolderThreadsPool;
    int                 numberOfTransferIntoToDoList;
    std::vector<TransferThreadAsync *>		transferThreadList;
    ScanFileOrFolder *		newScanThread(Ultracopier::CopyMode mode);
    uint64_t				bytesToTransfer;
    uint64_t				bytesTransfered;
    bool				autoStart;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool                rsync;
    #endif
    std::vector<Ultracopier::ReturnActionOnCopyList>	actionDone;///< to action to send to the interface
    uint64_t				idIncrementNumber;///< to store the last id returned
    int64_t				actualRealByteTransfered;
    FolderExistsAction	alwaysDoThisActionForFolderExists;
    bool				checkDestinationFolderExists;
    unsigned int        parallelizeIfSmallerThan;
    bool                moveTheWholeFolder;
    bool                deletePartiallyTransferredFiles;
    int                 inodeThreads;
    bool                renameTheOriginalDestination;
    bool                checkDiskSpace;
    bool                buffer;
    bool                followTheStrictOrder;
    bool                ignoreBlackList;
    std::unordered_set<TransferThreadAsync *> putAtBottomAfterError;
    std::unordered_map<std::string,uint64_t> requiredSpace;
    std::vector<std::pair<uint64_t,uint32_t> > timeToTransfer;
    //unsigned int        putAtBottom;//why here? more correct into CopyEngine(), then translated to CopyEngine
    std::vector<Filters_rules>		include,exclude;
    Ultracopier::CopyMode		mode;
    bool				forcedMode;
    std::string				firstRenamingRule;
    std::string				otherRenamingRule;
    /* here to prevent:
    QObject::killTimer: timers cannot be stopped from another thread
    QObject::startTimer: timers cannot be started from another thread */

    static Ultracopier::ItemOfCopyList actionToDoTransferToItemOfCopyList(const ActionToDoTransfer &actionToDoTransfer);
    //add file transfer to do
    uint64_t addToTransfer(const INTERNALTYPEPATH& source, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode& mode, const int64_t sendedsize=-1);
    //generate id number
    uint64_t generateIdNumber();
    //warning the first entry is accessible will copy
    bool removeSingleItem(const uint64_t &id);
    //put on top
    bool moveOnTopItem(const uint64_t &id);
    //move up
    bool moveUpItem(const uint64_t &id);
    //move down
    bool moveDownItem(const uint64_t &id);
    //put on bottom
    bool moveOnBottomItem(const uint64_t &id);
    //general transfer
    void startGeneralTransfer();
    //debug windows if needed
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    QTimer timerUpdateDebugDialog;
    #endif
    void detectDrivesOfCurrentTransfer(const std::vector<INTERNALTYPEPATH> &sources, const INTERNALTYPEPATH &destination);
    FacilityInterface * facilityInterface;
    QSemaphore waitConstructor,waitCancel;
    int actionToDoListTransfer_count,actionToDoListInode_count;
    bool doTransfer,doInode;
    int64_t oversize;//used as temp variable
    int64_t currentProgression;
    int64_t copiedSize,totalSize,localOverSize;
    std::vector<Ultracopier::ProgressionItem> progressionList;
    //memory variable for transfer thread creation
    bool doRightTransfer;
    bool keepDate;
    bool os_spec_flags;
    bool native_copy;
    bool mkFullPath;
    std::vector<std::string> drives;
    FileExistsAction alwaysDoThisActionForFileExists;
    int speedLimitation;
    //to return value to the copyEngine
    bool returnBoolToCopyEngine;
    std::pair<quint64,quint64> returnPairQuint64ToCopyEngine;
    std::vector<Ultracopier::ItemOfCopyList> returnListItemOfCopyListToCopyEngine;
    Ultracopier::ItemOfCopyList returnItemOfCopyListToCopyEngine;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QTimer *clockForTheCopySpeed;
    int blockSizeAfterSpeedLimitation;//in Bytes
    int                 multiForBigSpeed;
    int blockSize;
    #endif
    bool putInPause;

    void realByteTransfered();
    int getNumberOfTranferRuning() const;
    bool needMoreSpace() const;
private slots:
    void exportTransferListInternal(const std::string &fileName);
    void importTransferListInternal(const std::string &fileName);

    void scanThreadHaveFinishSlot();
    void scanThreadHaveFinish(bool skipFirstRemove=false);
    void autoStartAndCheckSpace();
    void updateTheStatus();
    void fileTransfer(const INTERNALTYPEPATH &sourceFileInfo, const INTERNALTYPEPATH &destinationFileInfo,
                      const Ultracopier::CopyMode &mode);
    void fileTransferWithInode(const INTERNALTYPEPATH &sourceFileInfo, const INTERNALTYPEPATH &destinationFileInfo,
                               const Ultracopier::CopyMode &mode, const TransferThread::dirent_uc &inode);
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
    void fileAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFile(const INTERNALTYPEPATH &fileInfo,const std::string &errorString, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(const INTERNALTYPEPATH &fileInfo, const std::string &errorString, const ErrorType &errorType);
    //to run the thread
    void run();
    /// \to create transfer thread
    void createTransferThread();
    void deleteTransferThread();
    //mk path to do
    uint64_t addToMkPath(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination, const int &inode);
    //add path to do
    void addToMovePath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inodeToRemove);
    void addToKeepAttributePath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inodeToRemove);
    //add to real move
    void addToRealMove(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    //rsync rm
    void addToRmForRsync(const std::string& destination);
    #endif
    //send the progression, after full reset of the interface (then all is empty)
    void syncTransferList_internal();

    void checkIfReadyToCancel();
signals:
    //send information about the copy
    void actionInProgess(const Ultracopier::EngineActionInProgress &) const;	//should update interface information on this event

    void newActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList> &) const;///very important, need be temporized to group the modification to do and not flood the interface
    void syncReady() const;
    void doneTime(const std::vector<std::pair<uint64_t,uint32_t> >&) const;

    /** \brief to get the progression for a specific file
     * \param id the id of the transfer, id send during population the transfer list
     * first = current transfered byte, second = byte to transfer */
    void pushFileProgression(const std::vector<Ultracopier::ProgressionItem> &progressionList) const;
    //get information about the copy
    /** \brief to get the general progression
     * first = current transfered byte, second = byte to transfer */
    void pushGeneralProgression(const uint64_t &,const uint64_t &) const;

    void newFolderListing(const std::string &path) const;

    //when can be deleted
    void canBeDeleted() const;
    void haveNeedPutAtBottom(bool needPutAtBottom,const INTERNALTYPEPATH &fileInfo,const std::string &errorString,TransferThreadAsync * thread,const ErrorType &errorType) const;

    //send error occurred
    void error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error) const;
    void errorToRetry(const std::string &source,const std::string &destination,const std::string &error) const;
    //for the extra logging
    void rmPath(const std::string &path) const;
    void mkPath(const std::string &path) const;
    /// \brief To debug source
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
    #endif
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    void updateTheDebugInfo(const std::vector<std::string> &,const std::vector<std::string>&,const int &) const;
    #endif

    //other signal
    /// \note Can be call without queue because all call will be serialized
    void send_fileAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame,TransferThreadAsync * thread) const;
    /// \note Can be call without queue because all call will be serialized
    void send_errorOnFile(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,TransferThreadAsync * thread, const ErrorType &errorType) const;
    /// \note Can be call without queue because all call will be serialized
    void send_folderAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame,ScanFileOrFolder * thread) const;
    /// \note Can be call without queue because all call will be serialized
    void send_errorOnFolder(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,ScanFileOrFolder * thread, const ErrorType &errorType) const;
    //send the progression
    void send_syncTransferList() const;
    //mkpath error event
    void mkPathErrorOnFolder(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,const ErrorType &errorType) const;
    //to close
    void tryCancel() const;
    //to ask new transfer thread
    void askNewTransferThread() const;

    void warningTransferList(const std::string &warning) const;
    void errorTransferList(const std::string &error) const;
    void send_sendNewRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule) const;
    void send_realBytesTransfered(const uint64_t &) const;

    void send_parallelizeIfSmallerThan(const int &parallelizeIfSmallerThan) const;
    void send_updateMount();
    void missingDiskSpace(std::vector<Diskspace> list) const;
    void isInPause(const bool &) const;

    void exportTransferListSend(const std::string &fileName);
    void importTransferListSend(const std::string &fileName);
};

#endif // LISTTHREAD_H
