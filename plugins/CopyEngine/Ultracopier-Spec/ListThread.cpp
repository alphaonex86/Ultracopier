#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"

#if defined(SYNCFILEMANIP)
#include "sync/TransferThreadSync.h"
#else
    #if defined(FSCOPYASYNC)
    #include "async/TransferThreadAsync.h"
    #else
    #error not sync and async set
    #endif
#endif

ListThread::ListThread(FacilityInterface * facilityInterface) :
    numberOfInodeOperation(0),
    sourceDriveMultiple(false),
    destinationDriveMultiple(false),
    destinationFolderMultiple(false),
    stopIt(false),
    numberOfTransferIntoToDoList(0),
    bytesToTransfer(0),
    bytesTransfered(0),
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync(false),
    #endif
    idIncrementNumber(1),
    actualRealByteTransfered(0),
    checkDestinationFolderExists(false),
    parallelizeIfSmallerThan(1024),
    moveTheWholeFolder(false),
    deletePartiallyTransferredFiles(true),
    inodeThreads(16),
    renameTheOriginalDestination(false),
    checkDiskSpace(true),
    buffer(false),
    followTheStrictOrder(true),
    mode(Ultracopier::CopyMode::Copy),
    forcedMode(false),
    actionToDoListTransfer_count(0),
    actionToDoListInode_count(0),
    doTransfer(false),
    doInode(false),
    oversize(0),
    currentProgression(0),
    copiedSize(0),
    totalSize(0),
    localOverSize(0),
    doRightTransfer(false),
    keepDate(false),
    mkFullPath(false),
    alwaysDoThisActionForFileExists(FileExists_NotSet),
    speedLimitation(0),
    returnBoolToCopyEngine(true)
{
    moveToThread(this);
    start(HighPriority);
    this->facilityInterface=facilityInterface;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    clockForTheCopySpeed            = NULL;
    multiForBigSpeed                = 0;
    blockSize                       = 65536;
    blockSizeAfterSpeedLimitation   = blockSize;
    #endif
    putInPause                      = false;
    autoStart=true;

    //can't be static into WriteThread, linked by instance then by ListThread
    writeFileList=new QMultiHash<QString,WriteThread *>();
    writeFileListMutex=new QMutex();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    if(!connect(&timerUpdateDebugDialog,&QTimer::timeout,this,&ListThread::timedUpdateDebugDialog))
        abort();
    timerUpdateDebugDialog.start(ULTRACOPIER_PLUGIN_DEBUG_WINDOW_TIMER);
    #endif
    if(!connect(this,           &ListThread::tryCancel,							this,&ListThread::cancel,                               Qt::QueuedConnection))
        abort();
    if(!connect(this,           &ListThread::askNewTransferThread,				this,&ListThread::createTransferThread,					Qt::QueuedConnection))
        abort();
    if(!connect(&mkPathQueue,	&MkPath::firstFolderFinish,						this,&ListThread::mkPathFirstFolderFinish,				Qt::QueuedConnection))
        abort();
    if(!connect(&mkPathQueue,	&MkPath::errorOnFolder,							this,&ListThread::mkPathErrorOnFolder,                  Qt::QueuedConnection))
        abort();
    if(!connect(this,           &ListThread::send_syncTransferList,				this,&ListThread::syncTransferList_internal,			Qt::QueuedConnection))
        abort();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&mkPathQueue,	&MkPath::debugInformation,						this,&ListThread::debugInformation,	Qt::QueuedConnection))
        abort();
    if(!connect(&driveManagement,&DriveManagement::debugInformation,			this,&ListThread::debugInformation,	Qt::QueuedConnection))
        abort();
    #endif // ULTRACOPIER_PLUGIN_DEBUG

    emit askNewTransferThread();
    mkpathTransfer.release();
}

ListThread::~ListThread()
{
    emit tryCancel();
    waitCancel.acquire();
    quit();
    wait();
}

//transfer is finished
void ListThread::transferInodeIsClosed()
{
    if(stopIt)
    {
        checkIfReadyToCancel();
        return;
    }
    numberOfInodeOperation--;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
    #endif
    TransferThreadAsync *temp_transfer_thread=qobject_cast<TransferThreadAsync *>(QObject::sender());
    if(temp_transfer_thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not located!");
        return;
    }
    if(overCheckUsedThread.find(temp_transfer_thread)==overCheckUsedThread.cend())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not located in overcheck");
    else
        overCheckUsedThread.erase(temp_transfer_thread);
    if(putAtBottomAfterError.find(temp_transfer_thread)!=putAtBottomAfterError.cend())
    {
        doNewActions_inode_manipulation();
        doNewActions_start_transfer();
        return;
    }
    bool isFound=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    int countLocalParse=0;
    #endif
    if(temp_transfer_thread->getStat()!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not idle!");
        return;
    }
    unsigned int int_for_internal_loop=0;
    while(int_for_internal_loop<actionToDoListTransfer.size())
    {
        if(actionToDoListTransfer.at(int_for_internal_loop).id==temp_transfer_thread->transferId)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[%1] have finish, put at idle; for id: %2").arg(int_for_internal_loop).arg(temp_transfer_thread->transferId).toStdString());
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::RemoveItem;
            newAction.userAction.moveAt=0;
            newAction.addAction=actionToDoTransferToItemOfCopyList(actionToDoListTransfer.at(int_for_internal_loop));
            newAction.userAction.position=int_for_internal_loop;
            actionDone.push_back(newAction);
            /// \todo check if item is at the right thread
            const int64_t transferTime=temp_transfer_thread->transferTime();
            if(transferTime>=0 && temp_transfer_thread->haveStartTime)
            {
                timeToTransfer.push_back(std::pair<uint64_t,uint32_t>(temp_transfer_thread->transferSize,transferTime));
                temp_transfer_thread->haveStartTime=false;
            }
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+int_for_internal_loop);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, actionToDoListInode: %2, actionToDoListInode_afterTheTransfer: %3").arg(actionToDoListTransfer.size()).arg(actionToDoListInode.size()).arg(actionToDoListInode_afterTheTransfer.size()).toStdString());
            if(actionToDoListTransfer.empty() && actionToDoListInode.empty() && actionToDoListInode_afterTheTransfer.empty())
                updateTheStatus();

            //add the current size of file, to general size because it's finish
            copiedSize=temp_transfer_thread->copiedSize();
            if(copiedSize>(qint64)temp_transfer_thread->transferSize)
            {
                oversize=copiedSize-temp_transfer_thread->transferSize;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"add oversize of: "+std::to_string(oversize));
                bytesToTransfer+=oversize;
                bytesTransfered+=oversize;
            }
            bytesTransfered+=temp_transfer_thread->transferSize;

            temp_transfer_thread->transferId=0;
            temp_transfer_thread->transferSize=0;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            countLocalParse++;
            #endif
            isFound=true;
            if(actionToDoListTransfer.empty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionToDoListTransfer==0");
                actionToDoListInode.insert(actionToDoListInode.cbegin(),actionToDoListInode_afterTheTransfer.cbegin(),actionToDoListInode_afterTheTransfer.cend());
                actionToDoListInode_afterTheTransfer.clear();
                doNewActions_inode_manipulation();
                doNewActions_start_transfer();
            }
            break;
        }
        int_for_internal_loop++;
    }
    if(isFound)
        deleteTransferThread();
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("unable to found item into the todo list, id: %1, index: %2").arg(temp_transfer_thread->transferId).arg(int_for_internal_loop).toStdString());
        temp_transfer_thread->transferId=0;
        temp_transfer_thread->transferSize=0;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("countLocalParse: %1, actionToDoList.size(): %2").arg(countLocalParse).arg(actionToDoListTransfer.size()).toStdString());
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(countLocalParse!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"countLocalParse != 1");
    #endif
    doNewActions_inode_manipulation();
    doNewActions_start_transfer();
}

/** \brief put the current file at bottom in case of error
\note ONLY IN CASE OF ERROR

transferInodeIsClosed() will be call after, then will do:

BUT

this function call transfer->skip();
not put at bottom, remove from to do list
*/
void ListThread::transferPutAtBottom()
{
    TransferThreadAsync *transfer=qobject_cast<TransferThreadAsync *>(QObject::sender());
    if(transfer==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not located!");
        return;
    }
    bool isFound=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    int countLocalParse=0;
    #endif
    unsigned int indexAction=0;
    while(indexAction<actionToDoListTransfer.size())
    {
        if(actionToDoListTransfer.at(indexAction).id==transfer->transferId)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Put at the end: "+std::to_string(transfer->transferId));
            //push for interface at the end
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=transfer->transferId;
            newAction.userAction.position=0;
            newAction.userAction.moveAt=actionToDoListTransfer.size()-1;
            actionDone.push_back(newAction);
            //do the wait stat
            actionToDoListTransfer[indexAction].isRunning=false;
            //move at the end
            actionToDoListTransfer.push_back(actionToDoListTransfer.at(indexAction));
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+indexAction);
            transfer->transferId=0;
            transfer->transferSize=0;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            countLocalParse++;
            #endif
            isFound=true;
            putAtBottomAfterError.insert(transfer);
            break;
        }
        indexAction++;
    }
    if(!isFound)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("unable to found item into the todo list, id: %1, index: %2").arg(transfer->transferId).toStdString());
        transfer->transferId=0;
        transfer->transferSize=0;
        putAtBottomAfterError.insert(transfer);
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"countLocalParse: "+std::to_string(countLocalParse));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(countLocalParse!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"countLocalParse != 1");
    #endif
    transfer->skip();
}

void ListThread::fileTransfer(const INTERNALTYPEPATH &sourceFileInfo,const INTERNALTYPEPATH &destinationFileInfo,const Ultracopier::CopyMode &mode)
{
    if(stopIt)
        return;
    addToTransfer(sourceFileInfo,destinationFileInfo,mode);
}

void ListThread::fileTransferWithInode(const INTERNALTYPEPATH &sourceFileInfo,const INTERNALTYPEPATH &destinationFileInfo,
                                       const Ultracopier::CopyMode &mode,const TransferThread::dirent_uc &inode)
{
    if(stopIt)
        return;
    #ifdef Q_OS_WIN32
    addToTransfer(sourceFileInfo,destinationFileInfo,mode,inode.size);
    #else
    (void)inode;
    addToTransfer(sourceFileInfo,destinationFileInfo,mode);
    #endif
}

void ListThread::autoStartAndCheckSpace()
{
    if(needMoreSpace())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Need more space");
        return;
    }
    autoStartIfNeeded();
}

void ListThread::autoStartIfNeeded()
{
    if(autoStart)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Auto start the copy");
        startGeneralTransfer();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Put the copy engine in pause");
        putInPause=true;
        emit isInPause(true);
    }
}

//set auto start
void ListThread::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
}

void ListThread::startGeneralTransfer()
{
    doNewActions_inode_manipulation();
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void ListThread::syncTransferList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit send_syncTransferList();
}

bool ListThread::getReturnBoolToCopyEngine() const
{
    return returnBoolToCopyEngine;
}

std::pair<quint64, quint64> ListThread::getReturnPairQuint64ToCopyEngine() const
{
    return returnPairQuint64ToCopyEngine;
}

Ultracopier::ItemOfCopyList ListThread::getReturnItemOfCopyListToCopyEngine() const
{
    return returnItemOfCopyListToCopyEngine;
}

void ListThread::realByteTransfered()
{
    quint64 totalRealByteTransfered=0;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        const TransferThreadAsync * thread=transferThreadList.at(index);
        totalRealByteTransfered+=thread->realByteTransfered();
        index++;
    }
    emit send_realBytesTransfered(totalRealByteTransfered);
}

void ListThread::checkIfReadyToCancel()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!stopIt)
        return;
    int index=0;
    int loop_size=transferThreadList.size();
    while(index<loop_size)
    {
        if(transferThreadList.at(index)!=NULL)
        {
            if(transferThreadList.at(index)->transferId!=0)
                return;
            //need wait a clean stop (and then destination remove)
            if(transferThreadList.at(index)->getStat()!=TransferStat_Idle)
                return;
            delete transferThreadList.at(index);//->deleteLayer();
            transferThreadList[index]=NULL;
            transferThreadList.erase(transferThreadList.cbegin()+index);
            loop_size=transferThreadList.size();
            index--;
        }
        index++;
    }
    actionToDoListTransfer.clear();
    actionToDoListInode.clear();
    actionToDoListInode_afterTheTransfer.clear();
    actionDone.clear();
    progressionList.clear();
    returnListItemOfCopyListToCopyEngine.clear();
    quit();
    waitCancel.release();
    emit canBeDeleted();
}

void ListThread::updateTheStatus()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    sendActionDone();
    bool updateTheStatus_listing=scanFileOrFolderThreadsPool.size()>0;
    bool updateTheStatus_copying=actionToDoListTransfer.size()>0 || actionToDoListInode.size()>0 || actionToDoListInode_afterTheTransfer.size()>0;
    Ultracopier::EngineActionInProgress updateTheStatus_action_in_progress;
    if(updateTheStatus_copying && updateTheStatus_listing)
        updateTheStatus_action_in_progress=Ultracopier::CopyingAndListing;
    else if(updateTheStatus_listing)
        updateTheStatus_action_in_progress=Ultracopier::Listing;
    else if(updateTheStatus_copying)
        updateTheStatus_action_in_progress=Ultracopier::Copying;
    else
        updateTheStatus_action_in_progress=Ultracopier::Idle;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit actionInProgess("+std::to_string(updateTheStatus_action_in_progress)+")");
    emit actionInProgess(updateTheStatus_action_in_progress);
}

//mk path to do
uint64_t ListThread::addToMkPath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inode)
{
    if(stopIt)
        return 0;
    if(inode!=0 && (!keepDate && !doRightTransfer))
        return 0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination));
    ActionToDoInode temp;
    temp.type       = ActionType_MkPath;
    temp.id         = generateIdNumber();
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
    return temp.id;
}

//add path to do
void ListThread::addToMovePath(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination, const int& inodeToRemove)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination)+", inodeToRemove: "+std::to_string(inodeToRemove));
    ActionToDoInode temp;
    temp.type	= ActionType_MovePath;
    temp.id		= generateIdNumber();
    temp.size	= inodeToRemove;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}

void ListThread::addToKeepAttributePath(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination, const int& inodeToRemove)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination)+", inodeToRemove: "+std::to_string(inodeToRemove));
    ActionToDoInode temp;
    temp.type	= ActionType_SyncDate;
    temp.id		= generateIdNumber();
    temp.size	= inodeToRemove;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}

void ListThread::addToRealMove(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination));
    ActionToDoInode temp;
    temp.type	= ActionType_RealMove;
    temp.id		= generateIdNumber();
    temp.size	= 0;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
//rsync rm
void ListThread::addToRmForRsync(const std::string& destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"inode: "+destination);
    ActionToDoInode temp;
    temp.type	= ActionType_RmSync;
    temp.id		= generateIdNumber();
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}
#endif

//send the progression, after full reset of the interface (then all is empty)
void ListThread::syncTransferList_internal()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit syncReady();
    actionDone.clear();
    //do list operation
    TransferThreadAsync *transferThread;
    const int &loop_size=actionToDoListTransfer.size();
    int loop_sub_size=transferThreadList.size();
    //this loop to have at max inodeThreads*inodeThreads, not inodeThreads*transferThreadList.size()
    int int_for_internal_loop;
    for(int int_for_loop=0; int_for_loop<loop_size; ++int_for_loop) {
        const ActionToDoTransfer &item=actionToDoListTransfer.at(int_for_loop);
        Ultracopier::ReturnActionOnCopyList newAction;
        newAction.type				= Ultracopier::PreOperation;
        newAction.addAction.id			= item.id;
        const size_t sourceIndex=item.source.rfind('/');
        if(sourceIndex == std::string::npos)
        {
            newAction.addAction.sourceFullPath	= '/';
            newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(item.source);
        }
        else
        {
            newAction.addAction.sourceFullPath	= TransferThread::internalStringTostring(item.source);
            newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(item.source.substr(sourceIndex+1));
        }
        const size_t destinationIndex=item.destination.rfind('/');
        if(destinationIndex == std::string::npos)
        {
            newAction.addAction.destinationFullPath	= '/';
            newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(item.destination);
        }
        else
        {
            newAction.addAction.destinationFullPath	= TransferThread::internalStringTostring(item.destination);
            newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(item.destination.substr(destinationIndex+1));
        }
        newAction.addAction.size		= item.size;
        newAction.addAction.mode		= item.mode;
        actionDone.push_back(newAction);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"id: "+std::to_string(item.id)+", size: "+std::to_string(item.size)+
                                 ", name: "+TransferThread::internalStringTostring(item.source)+", size2: "+std::to_string(newAction.addAction.size));
        if(item.isRunning)
        {
            for(int_for_internal_loop=0; int_for_internal_loop<loop_sub_size; ++int_for_internal_loop) {
                transferThread=transferThreadList.at(int_for_internal_loop);
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type				= Ultracopier::PreOperation;
                newAction.addAction.id			= item.id;
                const size_t sourceIndex=item.source.rfind('/');
                if(sourceIndex == std::string::npos)
                {
                    newAction.addAction.sourceFullPath	= TransferThread::internalStringTostring(item.source);
                    newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(item.source);
                }
                else
                {
                    newAction.addAction.sourceFullPath	= TransferThread::internalStringTostring(item.source);
                    newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(item.source.substr(sourceIndex+1));
                }
                const size_t destinationIndex=item.destination.rfind('/');
                if(destinationIndex == std::string::npos)
                {
                    newAction.addAction.destinationFullPath	= TransferThread::internalStringTostring(item.destination);
                    newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(item.destination);
                }
                else
                {
                    newAction.addAction.destinationFullPath	= TransferThread::internalStringTostring(item.destination);
                    newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(item.destination.substr(destinationIndex+1));
                }
                newAction.addAction.size		= item.size;
                newAction.addAction.mode		= item.mode;
                actionDone.push_back(newAction);
                if(transferThread->getStat()!=TransferStat_PreOperation)
                {
                    Ultracopier::ReturnActionOnCopyList newAction;
                    switch(transferThread->getStat())
                    {
                        case TransferStat_Transfer:
                            newAction.type=Ultracopier::Transfer;
                        break;
                        /*case TransferStat_PostTransfer:
                            newAction.type=Ultracopier::PostOperation;
                        break;*/
                        case TransferStat_PostOperation:
                            newAction.type=Ultracopier::PostOperation;
                        break;
                        default:
                        break;
                    }
                    newAction.addAction.id			= item.id;
                    actionDone.push_back(newAction);
                }
            }
        }
    }
}

//add file transfer to do
uint64_t ListThread::addToTransfer(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination, const Ultracopier::CopyMode& mode, const int64_t sendedsize)
{
    if(stopIt)
        return 0;
    //add to transfer list
    numberOfTransferIntoToDoList++;
    uint64_t size=0;
    if(sendedsize>=0)
        size=sendedsize;
    else
    {
        const int64_t tempSize=TransferThread::file_stat_size(source);
        if(tempSize>=0)
            size=tempSize;
    }
    const std::string &drive=driveManagement.getDrive(TransferThread::internalStringTostring(destination));
    if(!drive.empty())//can be a network drive
        if(mode!=Ultracopier::Move || drive!=driveManagement.getDrive(TransferThread::internalStringTostring(source)))
        {
            if(requiredSpace.find(drive)!=requiredSpace.cend())
            {
                requiredSpace[drive]+=size;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("space needed add: %1, space needed: %2, on: %3").arg(size).arg(requiredSpace.at(drive)).arg(QString::fromStdString(drive)).toStdString());
            }
            else
            {
                requiredSpace[drive]=size;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("set space %1 needed, on: %2").arg(size).arg(QString::fromStdString(drive)).toStdString());
            }
        }
    bytesToTransfer+= size;
    ActionToDoTransfer temp;
    temp.id		= generateIdNumber();
    temp.size	= size;
    temp.source	= source;
    temp.destination= destination;
    temp.mode	= mode;
    temp.isRunning	= false;
    actionToDoListTransfer.push_back(temp);
    //push the new transfer to interface
    Ultracopier::ReturnActionOnCopyList newAction;
    newAction.type				= Ultracopier::AddingItem;
    newAction.addAction=actionToDoTransferToItemOfCopyList(temp);
    actionDone.push_back(newAction);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination)+", add entry: "+std::to_string(temp.id)+", size: "+
                             std::to_string(temp.size)+", size2: "+std::to_string(size)+", isSymLink: "+std::to_string(TransferThread::is_symlink(source)));
    return temp.id;
}

Ultracopier::ItemOfCopyList ListThread::actionToDoTransferToItemOfCopyList(const ListThread::ActionToDoTransfer &actionToDoTransfer)
{
    Ultracopier::ItemOfCopyList itemOfCopyList;
    itemOfCopyList.id			= actionToDoTransfer.id;
    const size_t sourceIndex=actionToDoTransfer.source.rfind('/');
    if(sourceIndex == std::string::npos)
    {
        itemOfCopyList.sourceFullPath	= TransferThread::internalStringTostring(actionToDoTransfer.source);
        itemOfCopyList.sourceFileName	= TransferThread::internalStringTostring(actionToDoTransfer.source);
    }
    else
    {
        itemOfCopyList.sourceFullPath	= TransferThread::internalStringTostring(actionToDoTransfer.source);
        itemOfCopyList.sourceFileName	= TransferThread::internalStringTostring(actionToDoTransfer.source.substr(sourceIndex+1));
    }
    const size_t destinationIndex=actionToDoTransfer.destination.rfind('/');
    if(destinationIndex == std::string::npos)
    {
        itemOfCopyList.destinationFullPath	= TransferThread::internalStringTostring(actionToDoTransfer.destination);
        itemOfCopyList.destinationFileName	= TransferThread::internalStringTostring(actionToDoTransfer.destination);
    }
    else
    {
        itemOfCopyList.destinationFullPath	= TransferThread::internalStringTostring(actionToDoTransfer.destination);
        itemOfCopyList.destinationFileName	= TransferThread::internalStringTostring(actionToDoTransfer.destination.substr(destinationIndex+1));
    }
    itemOfCopyList.size		= actionToDoTransfer.size;
    itemOfCopyList.mode		= actionToDoTransfer.mode;
    return itemOfCopyList;
}

//generate id number
uint64_t ListThread::generateIdNumber()
{
    idIncrementNumber++;
    if(idIncrementNumber>(((quint64)1024*1024)*1024*1024*2))
        idIncrementNumber=0;
    return idIncrementNumber;
}

//do new actions
void ListThread::doNewActions_start_transfer()
{
    /*ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, numberOfTranferRuning: %2")
                             .arg(actionToDoListTransfer.size()).arg(getNumberOfTranferRuning()).toStdString());*/
    if(stopIt || putInPause)
        return;
    int numberOfTranferRuning=getNumberOfTranferRuning();
    const int &loop_size=transferThreadList.size();
    //lunch the transfer in WaitForTheTransfer
    //high priority
    int int_for_loop=0;
    while(int_for_loop<loop_size)
    {
        TransferThreadAsync *transferThread=transferThreadList.at(int_for_loop);
        if(transferThread->getStat()==TransferStat_WaitForTheTransfer/*ready to start the transfer*/)
        {
            //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
            if(transferThread->transferSize>=parallelizeIfSmallerThan)
            {
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                    transferThread->startTheTransfer();
                    numberOfTranferRuning++;
                }
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                transferThread->startTheTransfer();
                numberOfTranferRuning++;
            }
        }
        int_for_loop++;
    }
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));

    //low priority
    int_for_loop=0;
    while(int_for_loop<loop_size)
    {
        TransferThreadAsync *transferThread=transferThreadList.at(int_for_loop);
        if(transferThread->getStat()==TransferStat_PreOperation/*wait user dialog action*/)
        {
            //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
            if(transferThread->transferSize>=parallelizeIfSmallerThan)
            {
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                    transferThread->startTheTransfer();
                    numberOfTranferRuning++;
                }
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
                transferThread->startTheTransfer();
                numberOfTranferRuning++;
            }
        }
        int_for_loop++;
    }
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
}

/** \brief lunch the pre-op or inode op
  1) locate the next next item to do into the both list
    1a) optimisation posible on the mkpath/rmpath
  2) determine what need be lunched
  3) lunch it, rerun the 2)
  */
void ListThread::doNewActions_inode_manipulation()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionToDoList.size(): "+std::to_string(actionToDoListTransfer.size()));
    #endif
    if(stopIt)
        checkIfReadyToCancel();
    if(stopIt || putInPause)
        return;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    #endif
    //lunch the pre-op or inode op
    TransferThreadAsync *currentTransferThread;
    int int_for_loop=0;
    int int_for_internal_loop=0;
    int int_for_transfer_thread_search=0;
    actionToDoListTransfer_count=actionToDoListTransfer.size();
    actionToDoListInode_count=actionToDoListInode.size();
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    //search the next transfer action to do
    while(int_for_loop<actionToDoListTransfer_count)
    {
        ActionToDoTransfer& currentActionToDoTransfer=actionToDoListTransfer[int_for_loop];
        if(!currentActionToDoTransfer.isRunning)
        {
            //search the next inode action to do
            while(int_for_internal_loop<actionToDoListInode_count)
            {
                if(!actionToDoListInode.at(int_for_internal_loop).isRunning)
                {
                    if(currentActionToDoTransfer.id<actionToDoListInode.at(int_for_internal_loop).id)
                    {
                        //do the tranfer action in the next code
                        break;
                    }
                    else
                    {
                        //do the inode action
                        #include "ListThread_InodeAction.cpp"
                    }
                }
                int_for_internal_loop++;
            }
            //do the tranfer action
            while(int_for_transfer_thread_search<loop_sub_size_transfer_thread_search)
            {
                /**
                    transferThreadList.at(int_for_transfer_thread_search)->transferId==0) /!\ important!
                    Because the other thread can have call doNewAction before than this thread have the finish event parsed!
                    I this case it lose all data
                    */
                currentTransferThread=transferThreadList.at(int_for_transfer_thread_search);
                if(currentTransferThread->getStat()==TransferStat_Idle && currentTransferThread->transferId==0 &&
                        overCheckUsedThread.find(currentTransferThread)==overCheckUsedThread.cend()) // /!\ important!
                {
                    overCheckUsedThread.insert(currentTransferThread);

                    std::string drive=driveManagement.getDrive(TransferThread::internalStringTostring(currentActionToDoTransfer.destination));
                    if(requiredSpace.find(drive)!=requiredSpace.cend() &&
                            (currentActionToDoTransfer.mode!=Ultracopier::Move ||
                             drive!=driveManagement.getDrive(TransferThread::internalStringTostring(currentActionToDoTransfer.source))))
                    {
                        requiredSpace[drive]-=currentActionToDoTransfer.size;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("space needed removed: %1, space needed: %2, on: %3").arg(
                                                     currentActionToDoTransfer.size).arg(requiredSpace.at(drive)).arg(QString::fromStdString(drive)).toStdString());
                    }
                    currentTransferThread->transferId=currentActionToDoTransfer.id;
                    currentTransferThread->transferSize=currentActionToDoTransfer.size;
                    putAtBottomAfterError.erase(currentTransferThread);
                    if(!currentTransferThread->setFiles(
                        currentActionToDoTransfer.source,
                        currentActionToDoTransfer.size,
                        currentActionToDoTransfer.destination,
                        currentActionToDoTransfer.mode
                        ))
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(int_for_loop)+"] id: "+
                                                 std::to_string(currentTransferThread->transferId)+
                                                 " is idle, but seam busy at set name: "+TransferThread::internalStringTostring(currentActionToDoTransfer.destination)
                                                 );
                        break;
                    }
                    currentActionToDoTransfer.isRunning=true;

                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(int_for_loop)+"] id: "+
                                             std::to_string(currentTransferThread->transferId)+
                                             " is idle, use it for "+TransferThread::internalStringTostring(currentActionToDoTransfer.destination)
                                             );

                    /// \note wrong position? Else write why it's here
                    Ultracopier::ReturnActionOnCopyList newAction;
                    newAction.type				= Ultracopier::PreOperation;
                    newAction.addAction.id			= currentActionToDoTransfer.id;
                    const size_t sourceIndex=currentActionToDoTransfer.source.rfind('/');
                    if(sourceIndex == std::string::npos)
                    {
                        newAction.addAction.sourceFullPath	= '/';
                        newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(currentActionToDoTransfer.source);
                    }
                    else
                    {
                        newAction.addAction.sourceFullPath	= TransferThread::internalStringTostring(currentActionToDoTransfer.source);
                        newAction.addAction.sourceFileName	= TransferThread::internalStringTostring(currentActionToDoTransfer.source.substr(sourceIndex+1));
                    }
                    const size_t destinationIndex=currentActionToDoTransfer.destination.rfind('/');
                    if(destinationIndex == std::string::npos)
                    {
                        newAction.addAction.destinationFullPath	= '/';
                        newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(currentActionToDoTransfer.destination);
                    }
                    else
                    {
                        newAction.addAction.destinationFullPath	= TransferThread::internalStringTostring(currentActionToDoTransfer.destination);
                        newAction.addAction.destinationFileName	= TransferThread::internalStringTostring(currentActionToDoTransfer.destination.substr(destinationIndex+1));
                    }
                    newAction.addAction.size		= currentActionToDoTransfer.size;
                    newAction.addAction.mode		= currentActionToDoTransfer.mode;
                    actionDone.push_back(newAction);
                    int_for_transfer_thread_search++;
                    numberOfInodeOperation++;
                    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
                    #endif
                    break;
                }
                int_for_transfer_thread_search++;
            }
            if(int_for_internal_loop==loop_sub_size_transfer_thread_search)
            {
                /// \note Can be normal when all thread is not initialized
                #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unable to found free thread to do the transfer");
                #endif
                break;
            }
            #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
            #endif
            if(numberOfInodeOperation>=inodeThreads)
                break;
            if(followTheStrictOrder)
                break;
        }
        int_for_loop++;
    }
    //search the next inode action to do
    int_for_internal_loop=0;
    while(int_for_internal_loop<actionToDoListInode_count)
    {
        if(!actionToDoListInode.at(int_for_internal_loop).isRunning)
        {
            //do the inode action
            #include "ListThread_InodeAction.cpp"
        }
        int_for_internal_loop++;
    }
    //error  checking
    if(actionToDoListInode_count>inodeThreads)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("The index have been detected as out of max range: %1>%2").arg(actionToDoListInode_count).arg(inodeThreads).toStdString());
        return;
    }
}

//restart transfer if it can
void ListThread::restartTransferIfItCan()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    TransferThreadAsync *transfer=qobject_cast<TransferThreadAsync *>(QObject::sender());
    if(transfer==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not located!");
        return;
    }
    int numberOfTranferRuning=getNumberOfTranferRuning();
    if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER && transfer->getStat()==TransferStat_WaitForTheTransfer)
        transfer->startTheTransfer();
    doNewActions_start_transfer();
}


void ListThread::set_setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("include.size(): %1, exclude.size(): %2").arg(include.size()).arg(exclude.size()).toStdString());
    this->include=include;
    this->exclude=exclude;
    unsigned int index=0;
    while(index<scanFileOrFolderThreadsPool.size())
    {
        scanFileOrFolderThreadsPool.at(index)->setFilters(include,exclude);
        index++;
    }
}

void ListThread::set_sendNewRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void ListThread::set_updateMount()
{
    driveManagement.tryUpdate();
    emit send_updateMount();
}

void ListThread::mkPathFirstFolderFinish()
{
    int int_for_loop=0;
    const int &loop_size=actionToDoListInode.size();
    while(int_for_loop<loop_size)
    {
        const ActionToDoInode &actionToDoInode=actionToDoListInode.at(int_for_loop);
        if(actionToDoInode.isRunning)
        {
            if(actionToDoInode.type==ActionType_MkPath || actionToDoInode.type==ActionType_SyncDate)
            {
                //to send to the log
                emit mkPath(TransferThread::internalStringTostring(actionToDoInode.destination));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop mkpath: "+TransferThread::internalStringTostring(actionToDoInode.destination));
                actionToDoListInode.erase(actionToDoListInode.cbegin()+int_for_loop);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, actionToDoListInode: %2, actionToDoListInode_afterTheTransfer: %3").arg(actionToDoListTransfer.size()).arg(actionToDoListInode.size()).arg(actionToDoListInode_afterTheTransfer.size()).toStdString());
                if(actionToDoListTransfer.empty() && actionToDoListInode.empty() && actionToDoListInode_afterTheTransfer.empty())
                    updateTheStatus();
                numberOfInodeOperation--;
                #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
                #endif
                doNewActions_inode_manipulation();
                return;
            }
            if(actionToDoInode.type==ActionType_MovePath || actionToDoInode.type==ActionType_RealMove
                    #ifdef ULTRACOPIER_PLUGIN_RSYNC
                     || actionToDoInode.type==ActionType_RmSync
                    #endif
                    )
            {
                //to send to the log
                #ifdef ULTRACOPIER_PLUGIN_RSYNC
                if(actionToDoInode.type!=ActionType_RmSync)
                    emit mkPath(actionToDoInode.destination);
                #else
                emit mkPath(TransferThread::internalStringTostring(actionToDoInode.destination));
                #endif
                emit rmPath(TransferThread::internalStringTostring(actionToDoInode.source));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop mkpath: "+TransferThread::internalStringTostring(actionToDoInode.destination));
                actionToDoListInode.erase(actionToDoListInode.cbegin()+int_for_loop);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, actionToDoListInode: %2, actionToDoListInode_afterTheTransfer: %3").arg(actionToDoListTransfer.size()).arg(actionToDoListInode.size()).arg(actionToDoListInode_afterTheTransfer.size()).toStdString());
                if(actionToDoListTransfer.empty() && actionToDoListInode.empty() && actionToDoListInode_afterTheTransfer.empty())
                    updateTheStatus();
                numberOfInodeOperation--;
                #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
                #endif
                doNewActions_inode_manipulation();
                return;
            }

        }
        int_for_loop++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to found item into the todo list");
}

/// \note Can be call without queue because all call will be serialized
void ListThread::fileAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame)
{
    emit send_fileAlreadyExists(source,destination,isSame,qobject_cast<TransferThreadAsync *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
void ListThread::errorOnFile(const INTERNALTYPEPATH &fileInfo, const std::string &errorString, const ErrorType &errorType)
{
    TransferThreadAsync * transferThread=qobject_cast<TransferThreadAsync *>(sender());
    if(transferThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Thread locating error");
        return;
    }
    ErrorLogEntry errorLogEntry;
    errorLogEntry.source=TransferThread::internalStringTostring(transferThread->getSourcePath());
    errorLogEntry.destination=TransferThread::internalStringTostring(transferThread->getDestinationPath());
    errorLogEntry.mode=transferThread->getMode();
    errorLogEntry.error=errorString;
    errorLog.push_back(errorLogEntry);
    emit errorToRetry(TransferThread::internalStringTostring(transferThread->getSourcePath()),
                      TransferThread::internalStringTostring(transferThread->getDestinationPath()),
                      errorString);
    emit send_errorOnFile(fileInfo,errorString,transferThread,errorType);
}

/// \note Can be call without queue because all call will be serialized
void ListThread::folderAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame)
{
    emit send_folderAlreadyExists(source,destination,isSame,qobject_cast<ScanFileOrFolder *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void ListThread::errorOnFolder(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,const ErrorType &errorType)
{
    emit send_errorOnFolder(fileInfo,errorString,qobject_cast<ScanFileOrFolder *>(sender()),errorType);
}

//to run the thread
void ListThread::run()
{
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    clockForTheCopySpeed=new QTimer();
    #endif
    exec();
}

void ListThread::getNeedPutAtBottom(const INTERNALTYPEPATH &fileInfo, const std::string &errorString, TransferThreadAsync *thread,
                                    const ErrorType &errorType)
{
    (void)fileInfo;
    (void)errorString;
    (void)errorType;
    thread->putAtBottom();
}

/// \to create transfer thread
void ListThread::createTransferThread()
{
    if(stopIt)
        return;
    if(transferThreadList.size()>=(unsigned int)inodeThreads)
        return;
    transferThreadList.push_back(new TransferThreadAsync());
    TransferThreadAsync * last=transferThreadList.back();
    last->transferId=0;
    last->transferSize=0;
    last->setRightTransfer(doRightTransfer);
    last->setKeepDate(keepDate);
    last->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    if(!last->setBlockSize(blockSizeAfterSpeedLimitation))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the block size: "+std::to_string(blockSizeAfterSpeedLimitation));
    #endif
    last->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
    last->setBuffer(buffer);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    last->setRsync(rsync);
    #endif

    last->writeThread.writeFileList=new QMultiHash<QString,WriteThread *>();
    last->writeThread.writeFileListMutex=new QMutex();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(last,&TransferThread::debugInformation,             this,&ListThread::debugInformation,             Qt::QueuedConnection))
        abort();
    #endif // ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(last,&TransferThread::errorOnFile,                  this,&ListThread::errorOnFile,                  Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::fileAlreadyExists,            this,&ListThread::fileAlreadyExists,            Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::tryPutAtBottom,               this,&ListThread::transferPutAtBottom,          Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::readStopped,                  this,&ListThread::doNewActions_start_transfer,  Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::preOperationStopped,          this,&ListThread::doNewActions_start_transfer,	Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::postOperationStopped,         this,&ListThread::transferInodeIsClosed,        Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::checkIfItCanBeResumed,		this,&ListThread::restartTransferIfItCan,       Qt::QueuedConnection))
        abort();
    if(!connect(last,&TransferThread::pushStat,                     this,&ListThread::newTransferStat,              Qt::QueuedConnection))
        abort();

    if(!connect(this,&ListThread::send_sendNewRenamingRules,		last,&TransferThread::setRenamingRules,         Qt::QueuedConnection))
        abort();

    if(!connect(this,&ListThread::send_updateMount,                 last,&TransferThread::set_updateMount,          Qt::QueuedConnection))
        abort();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    last->setMultiForBigSpeed(multiForBigSpeed);
    //speed limitation
    connect(clockForTheCopySpeed,	&QTimer::timeout,			last,	&TransferThreadAsync::timeOfTheBlockCopyFinished,		Qt::QueuedConnection);
    #endif

    last->setObjectName(QStringLiteral("transfer %1").arg(transferThreadList.size()-1));
    last->readThread.setObjectName(QStringLiteral("read %1").arg(transferThreadList.size()-1));
    last->writeThread.setObjectName(QStringLiteral("write %1").arg(transferThreadList.size()-1));
    last->setRenamingRules(firstRenamingRule,otherRenamingRule);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    last->setId(transferThreadList.size()-1);
    #endif
    if(transferThreadList.size()>=(unsigned int)inodeThreads)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"create the last of the "+std::to_string(inodeThreads)+" transferThread");
        return;
    }
    if(stopIt)
        return;
    doNewActions_inode_manipulation();
    emit askNewTransferThread();
}

void ListThread::deleteTransferThread()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int loop_size=transferThreadList.size();
    if(loop_size>inodeThreads)
    {
        int index=0;
        while(index<loop_size && loop_size>inodeThreads)
        {
            if(transferThreadList.at(index)->getStat()==TransferStat_Idle && transferThreadList.at(index)->transferId==0)
            {
                transferThreadList.at(index)->stop();
                delete transferThreadList.at(index);//->deleteLayer();
                transferThreadList[index]=NULL;
                transferThreadList.erase(transferThreadList.cbegin()+index);
                loop_size--;
            }
            else
                index++;
        }
        if(loop_size==inodeThreads)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"inodeThreads is lowered to the right value: "+std::to_string(inodeThreads));
    }
}

void ListThread::setMkFullPath(const bool mkFullPath)
{
    this->mkFullPath=mkFullPath;
    mkPathQueue.setMkFullPath(mkFullPath);
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setMkFullPath(mkFullPath);
        index++;
    }
}
