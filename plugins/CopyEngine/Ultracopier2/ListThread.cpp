#include "ListThread.h"
#include <QStorageInfo>
#include <QMutexLocker>
#include <QtGlobal>
#include "../../../cpp11addition.h"

ListThread::ListThread(FacilityInterface * facilityInterface)
{
    moveToThread(this);
    start(HighPriority);
    this->facilityInterface         = facilityInterface;
    sourceDriveMultiple             = false;
    destinationDriveMultiple        = false;
    destinationFolderMultiple       = false;
    stopIt                          = false;
    bytesToTransfer                 = 0;
    bytesTransfered                 = 0;
    idIncrementNumber               = 1;
    actualRealByteTransfered        = 0;
    numberOfTransferIntoToDoList    = 0;
    numberOfInodeOperation          = 0;
    putAtBottom                     = 0;
    maxSpeed                        = 0;
    inodeThreads                    = 1;
    renameTheOriginalDestination    = false;
    doRightTransfer                 = false;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync                           = false;
    #endif
    keepDate                        = false;
    checkDiskSpace                  = true;
    blockSize                       = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    sequentialBuffer                = ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
    parallelBuffer                  = ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    osBufferLimit                   = 512;
    alwaysDoThisActionForFileExists = FileExists_NotSet;
    doChecksum                      = false;
    checksumIgnoreIfImpossible      = true;
    checksumOnlyOnError             = true;
    osBuffer                        = false;
    osBufferLimited                 = false;
    forcedMode                      = false;

    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    connect(&timerUpdateDebugDialog,&QTimer::timeout,this,&ListThread::timedUpdateDebugDialog);
    timerUpdateDebugDialog.start(ULTRACOPIER_PLUGIN_DEBUG_WINDOW_TIMER);
    #endif
    connect(this,           &ListThread::tryCancel,							this,&ListThread::cancel,                               Qt::QueuedConnection);
    connect(this,           &ListThread::askNewTransferThread,				this,&ListThread::createTransferThread,					Qt::QueuedConnection);
    connect(&mkPathQueue,	&MkPath::firstFolderFinish,						this,&ListThread::mkPathFirstFolderFinish,				Qt::QueuedConnection);
    connect(&mkPathQueue,	&MkPath::errorOnFolder,							this,&ListThread::mkPathErrorOnFolder,                  Qt::QueuedConnection);
    connect(this,           &ListThread::send_syncTransferList,				this,&ListThread::syncTransferList_internal,			Qt::QueuedConnection);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&mkPathQueue,	&MkPath::debugInformation,						this,&ListThread::debugInformation,	Qt::QueuedConnection);
    connect(&driveManagement,&DriveManagement::debugInformation,			this,&ListThread::debugInformation,	Qt::QueuedConnection);
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
    numberOfInodeOperation--;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfInodeOperation: "+std::to_string(numberOfInodeOperation));
    #endif
    TransferThread *temp_transfer_thread=qobject_cast<TransferThread *>(QObject::sender());
    if(temp_transfer_thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"transfer thread not located!");
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
    int int_for_internal_loop=0;
    const int &loop_size=actionToDoListTransfer.size();
    while(int_for_internal_loop<loop_size)
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

            if(temp_transfer_thread->haveStartTime)
            {
                timeToTransfer.push_back(std::pair<uint64_t,uint32_t>(temp_transfer_thread->transferSize,temp_transfer_thread->startTransferTime.elapsed()));
                temp_transfer_thread->haveStartTime=false;
            }
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
}

/** \brief put the current file at bottom in case of error
\note ONLY IN CASE OF ERROR */
void ListThread::transferPutAtBottom()
{
    TransferThread *transfer=qobject_cast<TransferThread *>(QObject::sender());
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
            newAction.userAction.position=actionToDoListTransfer.size()-1;
            actionDone.push_back(newAction);
            //do the wait stat
            actionToDoListTransfer[indexAction].isRunning=false;
            //move at the end
            actionToDoListTransfer.push_back(actionToDoListTransfer.at(indexAction));
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+indexAction);
            //reset the thread list stat
            transfer->transferId=0;
            transfer->transferSize=0;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            countLocalParse++;
            #endif
            isFound=true;
            break;
        }
        indexAction++;
    }
    if(!isFound)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("unable to found item into the todo list, id: %1, index: %2").arg(transfer->transferId).toStdString());
        transfer->transferId=0;
        transfer->transferSize=0;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"countLocalParse: "+std::to_string(countLocalParse));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(countLocalParse!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"countLocalParse != 1");
    #endif
    transfer->skip();
}

//set the copy info and options before runing
void ListThread::setRightTransfer(const bool doRightTransfer)
{
    mkPathQueue.setRightTransfer(doRightTransfer);
    this->doRightTransfer=doRightTransfer;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setRightTransfer(doRightTransfer);
        index++;
    }
}

//set keep date
void ListThread::setKeepDate(const bool keepDate)
{
    mkPathQueue.setKeepDate(keepDate);
    this->keepDate=keepDate;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setKeepDate(keepDate);
        index++;
    }
}

//set block size in KB
void ListThread::setBlockSize(const int blockSize)
{
    this->blockSize=blockSize*1024;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setBlockSize(this->blockSize);
        index++;
    }
}

//set auto start
void ListThread::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void ListThread::setRsync(const bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set rsync: "+std::to_string(rsync));
    this->rsync=rsync;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setRsync(rsync);
        index++;
    }
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setRsync(rsync);
}
#endif

//set check destination folder
void ListThread::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void ListThread::fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo,const Ultracopier::CopyMode &mode)
{
    if(stopIt)
        return;
    addToTransfer(sourceFileInfo,destinationFileInfo,mode);
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::haveSameSource(const std::vector<std::string> &sources)
{
    if(stopIt)
        return false;
    if(sourceDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDriveMultiple");
        return false;
    }
    if(sourceDrive.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDrive.isEmpty()");
        return true;
    }
    unsigned int index=0;
    while(index<sources.size())
    {
        if(driveManagement.getDrive(sources.at(index))!=sourceDrive)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources.at(index))!=sourceDrive");
            return false;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"seam have same source");
    return true;
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::haveSameDestination(const std::string &destination)
{
    if(stopIt)
        return false;
    if(destinationDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDriveMultiple");
        return false;
    }
    if(destinationDrive.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDrive.isEmpty()");
        return true;
    }
    if(driveManagement.getDrive(destination)!=destinationDrive)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination!=destinationDrive");
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"seam have same destination");
    return true;
}

/// \return empty if multiple or no destination
std::string ListThread::getUniqueDestinationFolder() const
{
    if(stopIt)
        return std::string();
    if(destinationFolderMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDriveMultiple");
        return std::string();
    }
    return destinationFolder;
}

ScanFileOrFolder * ListThread::newScanThread(Ultracopier::CopyMode mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start with: "+std::to_string(mode));

    //create new thread because is auto-detroyed
    scanFileOrFolderThreadsPool.push_back(new ScanFileOrFolder(mode));
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::finishedTheListing,				this,&ListThread::scanThreadHaveFinishSlot,	Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::fileTransfer,                     this,&ListThread::fileTransfer,             Qt::QueuedConnection);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::debugInformation,					this,&ListThread::debugInformation,         Qt::QueuedConnection);
    #endif
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::newFolderListing,					this,&ListThread::newFolderListing);
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::addToMovePath,					this,&ListThread::addToMovePath,			Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::addToRealMove,					this,&ListThread::addToRealMove,			Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::addToMkPath,                      this,&ListThread::addToMkPath,              Qt::QueuedConnection);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::addToRmForRsync,                  this,&ListThread::addToRmForRsync,          Qt::QueuedConnection);
    #endif

    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::errorOnFolder,					this,&ListThread::errorOnFolder,            Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::folderAlreadyExists,				this,&ListThread::folderAlreadyExists,		Qt::QueuedConnection);

    connect(this,&ListThread::send_updateMount,                 scanFileOrFolderThreadsPool.back(),&ScanFileOrFolder::set_updateMount,          Qt::QueuedConnection);

    scanFileOrFolderThreadsPool.back()->setFilters(include,exclude);
    scanFileOrFolderThreadsPool.back()->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
    scanFileOrFolderThreadsPool.back()->setMoveTheWholeFolder(moveTheWholeFolder);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    scanFileOrFolderThreadsPool.back()->setRsync(rsync);
    #endif
    if(scanFileOrFolderThreadsPool.size()==1)
        updateTheStatus();
    scanFileOrFolderThreadsPool.back()->setRenamingRules(firstRenamingRule,otherRenamingRule);
    return scanFileOrFolderThreadsPool.back();
}

void ListThread::scanThreadHaveFinishSlot()
{
    scanThreadHaveFinish();
}

void ListThread::scanThreadHaveFinish(bool skipFirstRemove)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"listing thread have finish, skipFirstRemove: "+std::to_string(skipFirstRemove));
    if(!skipFirstRemove)
    {
        ScanFileOrFolder * senderThread = qobject_cast<ScanFileOrFolder *>(QObject::sender());
        if(senderThread==NULL)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"sender pointer null (plugin copy engine)");
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+std::to_string(scanFileOrFolderThreadsPool.size()));
            delete senderThread;
            vectorremoveOne(scanFileOrFolderThreadsPool,senderThread);
            if(scanFileOrFolderThreadsPool.empty())
                updateTheStatus();
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+std::to_string(scanFileOrFolderThreadsPool.size()));
    if(scanFileOrFolderThreadsPool.size()>0)
    {
        //then start the next listing threads
        if(scanFileOrFolderThreadsPool.front()->isFinished())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Start listing thread");
            scanFileOrFolderThreadsPool.front()->start();
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"The listing thread is already running");
    }
    else
        autoStartAndCheckSpace();
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
    }
}

void ListThread::startGeneralTransfer()
{
    doNewActions_inode_manipulation();
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newCopy(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+stringimplode(sources,";")+", destination: "+destination);
    ScanFileOrFolder * scanFileOrFolderThread = newScanThread(Ultracopier::Copy);
    if(scanFileOrFolderThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to get new thread");
        return false;
    }
    std::regex base_regex("^[a-z][a-z][a-z]*:/.*");
    std::smatch base_match;
    std::vector<std::string> sourcesClean;
    unsigned int index=0;
    while(index<sources.size())
    {
        std::string source=sources.at(index);
        //can be: file://192.168.0.99/share/file.txt
        //can be: file:///C:/file.txt
        //can be: file:///home/user/fileatrootunderunix
        #ifndef Q_OS_WIN
        if(stringStartWith(source,"file:///"))
            source.replace(0,7,"");
        #else
        if(stringStartWith(source,"file:///"))
            source.replace(0,8,"");
        else if(stringStartWith(source,"file://"))
            source.replace(0,5,"");
        else if(stringStartWith(source,"file:/"))
            source.replace(0,6,"");
        #endif
        else if (std::regex_match(source, base_match, base_regex))
            return false;
        if(index<99)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,sources.at(index)+" -> "+source);
        index++;
        sourcesClean.push_back(source);
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourcesClean: "+stringimplode(sourcesClean,";"));
    scanFileOrFolderThread->addToList(sourcesClean,destination);
    scanThreadHaveFinish(true);
    detectDrivesOfCurrentTransfer(sourcesClean,destination);
    return true;
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newMove(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ScanFileOrFolder * scanFileOrFolderThread = newScanThread(Ultracopier::Move);
    if(scanFileOrFolderThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to get new thread");
        return false;
    }
    scanFileOrFolderThread->addToList(sources,destination);
    scanThreadHaveFinish(true);
    detectDrivesOfCurrentTransfer(sources,destination);
    return true;
}

void ListThread::detectDrivesOfCurrentTransfer(const std::vector<std::string> &sources,const std::string &destination)
{
    /* code to detect volume/mount point to group by windows */
    if(!sourceDriveMultiple)
    {
        unsigned int index=0;
        while(index<sources.size())
        {
            const std::string &tempDrive=driveManagement.getDrive(sources.at(index));
            //if have not already source, set the source
            if(sourceDrive.empty())
                sourceDrive=tempDrive;
            //if have previous source and the news source is not the same
            if(sourceDrive!=tempDrive)
            {
                sourceDriveMultiple=true;
                break;
            }
            index++;
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source informations, sourceDrive: %1, sourceDriveMultiple: %2").arg(QString::fromStdString(sourceDrive)).arg(sourceDriveMultiple).toStdString());
    if(!destinationDriveMultiple)
    {
        const std::string &tempDrive=driveManagement.getDrive(destination);
        //if have not already destination, set the destination
        if(destinationDrive.empty())
            destinationDrive=tempDrive;
        //if have previous destination and the news destination is not the same
        if(destinationDrive!=tempDrive)
            destinationDriveMultiple=true;
    }
    if(!destinationFolderMultiple)
    {
        //if have not already destination, set the destination
        if(destinationFolder.empty())
            destinationFolder=destination;
        //if have previous destination and the news destination is not the same
        if(destinationFolder!=destination)
            destinationFolderMultiple=true;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("destination informations, destinationDrive: %1, destinationDriveMultiple: %2").arg(QString::fromStdString(destinationDrive)).arg(destinationDriveMultiple).toStdString());
}

void ListThread::setCollisionAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
        index++;
    }
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void ListThread::syncTransferList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit send_syncTransferList();
}

//set the folder local collision
void ListThread::setFolderCollision(const FolderExistsAction &alwaysDoThisActionForFolderExists)
{
    this->alwaysDoThisActionForFolderExists=alwaysDoThisActionForFolderExists;
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

void ListThread::set_doChecksum(bool doChecksum)
{
    this->doChecksum=doChecksum;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_doChecksum(doChecksum);
        index++;
    }
}

void ListThread::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
        index++;
    }
}

void ListThread::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    this->checksumOnlyOnError=checksumOnlyOnError;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_checksumOnlyOnError(checksumOnlyOnError);
        index++;
    }
}

void ListThread::set_osBuffer(bool osBuffer)
{
    this->osBuffer=osBuffer;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_osBuffer(osBuffer);
        index++;
    }
}

void ListThread::set_osBufferLimited(bool osBufferLimited)
{
    this->osBufferLimited=osBufferLimited;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_osBufferLimited(osBufferLimited);
        index++;
    }
}

void ListThread::realByteTransfered()
{
    quint64 totalRealByteTransfered=0;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        totalRealByteTransfered+=transferThreadList.at(index)->realByteTransfered();
        index++;
    }
    emit send_realBytesTransfered(totalRealByteTransfered);
}

void ListThread::pause()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
}

void ListThread::resume()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    startGeneralTransfer();
    doNewActions_start_transfer();
}

void ListThread::skip(const uint64_t &id)
{
    skipInternal(id);
}

bool ListThread::skipInternal(const uint64_t &id)
{
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        if(transferThreadList.at(index)->transferId==id)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"skip one transfer: "+std::to_string(id));
            transferThreadList.at(index)->skip();
            return true;
        }
        index++;
    }
    int int_for_internal_loop=0;
    const int &loop_size=actionToDoListTransfer.size();
    while(int_for_internal_loop<loop_size)
    {
        if(actionToDoListTransfer.at(int_for_internal_loop).id==id)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[%1] remove at not running, for id: %2").arg(int_for_internal_loop).arg(id).toStdString());
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::RemoveItem;
            newAction.userAction.moveAt=1;
            newAction.addAction=actionToDoTransferToItemOfCopyList(actionToDoListTransfer.at(int_for_internal_loop));
            newAction.userAction.position=int_for_internal_loop;
            actionDone.push_back(newAction);
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+int_for_internal_loop);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, actionToDoListInode: %2, actionToDoListInode_afterTheTransfer: %3").arg(actionToDoListTransfer.size()).arg(actionToDoListInode.size()).arg(actionToDoListInode_afterTheTransfer.size()).toStdString());
            if(actionToDoListTransfer.empty() && actionToDoListInode.empty() && actionToDoListInode_afterTheTransfer.empty())
                updateTheStatus();
            return true;
        }
        int_for_internal_loop++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"skip transfer not found: "+std::to_string(id));
    return false;
}

//executed in this thread
void ListThread::cancel()
{
    if(stopIt)
    {
        waitCancel.release();
        return;
    }
    stopIt=true;
    int index=0;
    int loop_size=transferThreadList.size();
    while(index<loop_size)
    {
        transferThreadList.at(index)->stop();
        index++;
    }
    index=0;
    loop_size=scanFileOrFolderThreadsPool.size();
    while(index<loop_size)
    {
        scanFileOrFolderThreadsPool.at(index)->stop();
        delete scanFileOrFolderThreadsPool.at(index);//->deleteLayer();
        scanFileOrFolderThreadsPool[index]=NULL;
        index++;
    }
    scanFileOrFolderThreadsPool.clear();
    checkIfReadyToCancel();
}

void ListThread::checkIfReadyToCancel()
{
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

//speedLimitation in KB/s
bool ListThread::setSpeedLimitation(const int64_t &speedLimitation)
{
    Q_UNUSED(speedLimitation);
    return false;
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

//set data local to the thread
void ListThread::setAlwaysFileExistsAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    int int_for_loop=0;
    const int &loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        transferThreadList.at(int_for_loop)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
        int_for_loop++;
    }
}

//mk path to do
uint64_t ListThread::addToMkPath(const QFileInfo& source,const QFileInfo& destination, const int& inode)
{
    if(stopIt)
        return 0;
    if(inode!=0 && (!keepDate && !doRightTransfer))
        return 0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());
    ActionToDoInode temp;
    temp.type       = ActionType_MkPath;
    temp.id         = generateIdNumber();
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
    return temp.id;
}

//add rm path to do
void ListThread::addToMovePath(const QFileInfo& source, const QFileInfo &destination, const int& inodeToRemove)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1, destination: %2, inodeToRemove: %3").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).arg(inodeToRemove).toStdString());
    ActionToDoInode temp;
    temp.type	= ActionType_MovePath;
    temp.id		= generateIdNumber();
    temp.size	= inodeToRemove;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}

void ListThread::addToRealMove(const QFileInfo& source,const QFileInfo& destination)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());
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
void ListThread::addToRmForRsync(const QFileInfo& destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"inode: "+destination.absoluteFilePath().toStdString());
    ActionToDoInode temp;
    temp.type	= ActionType_RmSync;
    temp.id		= generateIdNumber();
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode.push_back(temp);
}
#endif

//send action done
void ListThread::sendActionDone()
{
    if(!actionDone.empty())
    {
        emit newActionOnList(actionDone);
        actionDone.clear();
    }
    if(!timeToTransfer.empty())
    {
        emit doneTime(timeToTransfer);
        timeToTransfer.clear();
    }
}

//send progression
void ListThread::sendProgression()
{
    if(actionToDoListTransfer.empty())
        return;
    oversize=0;
    currentProgression=0;
    int int_for_loop=0;
    const int &loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        TransferThread * temp_transfer_thread=transferThreadList.at(int_for_loop);
        switch(temp_transfer_thread->getStat())
        {
            case TransferStat_Transfer:
            case TransferStat_PostTransfer:
            case TransferStat_Checksum:
            case TransferStat_PostOperation:
            {
                copiedSize=temp_transfer_thread->copiedSize();

                //for the general progression
                currentProgression+=copiedSize;

                //the oversize (when the file is bigger after/during the copy then what was during the listing)
                if(copiedSize>(qint64)temp_transfer_thread->transferSize)
                    localOverSize=copiedSize-temp_transfer_thread->transferSize;
                else
                    localOverSize=0;

                //the current size copied
                totalSize=temp_transfer_thread->transferSize+localOverSize;
                std::pair<uint64_t,uint64_t> progression=temp_transfer_thread->progression();
                tempItem.currentRead=progression.first;
                tempItem.currentWrite=progression.second;
                tempItem.id=temp_transfer_thread->transferId;
                tempItem.total=totalSize;
                progressionList.push_back(tempItem);

                //add the oversize to the general progression
                oversize+=localOverSize;
            }
            break;
            default:
            break;
        }
        int_for_loop++;
    }
    emit pushFileProgression(progressionList);
    progressionList.clear();
    emit pushGeneralProgression(bytesTransfered+currentProgression,bytesToTransfer+oversize);
    realByteTransfered();
}

//send the progression, after full reset of the interface (then all is empty)
void ListThread::syncTransferList_internal()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit syncReady();
    actionDone.clear();
    //do list operation
    TransferThread *transferThread;
    const int &loop_size=actionToDoListTransfer.size();
    int loop_sub_size=transferThreadList.size();
    //this loop to have at max inodeThreads*inodeThreads, not inodeThreads*transferThreadList.size()
    int int_for_internal_loop;
    for(int int_for_loop=0; int_for_loop<loop_size; ++int_for_loop) {
        const ActionToDoTransfer &item=actionToDoListTransfer.at(int_for_loop);
        Ultracopier::ReturnActionOnCopyList newAction;
        newAction.type				= Ultracopier::PreOperation;
        newAction.addAction.id			= item.id;
        newAction.addAction.sourceFullPath	= item.source.absoluteFilePath().toStdString();
        newAction.addAction.sourceFileName	= item.source.fileName().toStdString();
        newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath().toStdString();
        newAction.addAction.destinationFileName	= item.destination.fileName().toStdString();
        newAction.addAction.size		= item.size;
        newAction.addAction.mode		= item.mode;
        actionDone.push_back(newAction);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("id: %1, size: %2, name: %3, size2: %4").arg(item.id).arg(item.size).arg(item.source.absoluteFilePath()).arg(newAction.addAction.size).toStdString());
        if(item.isRunning)
        {
            for(int_for_internal_loop=0; int_for_internal_loop<loop_sub_size; ++int_for_internal_loop) {
                transferThread=transferThreadList.at(int_for_internal_loop);
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type				= Ultracopier::PreOperation;
                newAction.addAction.id			= item.id;
                newAction.addAction.sourceFullPath	= item.source.absoluteFilePath().toStdString();
                newAction.addAction.sourceFileName	= item.source.fileName().toStdString();
                newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath().toStdString();
                newAction.addAction.destinationFileName	= item.destination.fileName().toStdString();
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
uint64_t ListThread::addToTransfer(const QFileInfo& source,const QFileInfo& destination,const Ultracopier::CopyMode& mode)
{
    if(stopIt)
        return 0;
    //add to transfer list
    numberOfTransferIntoToDoList++;
    quint64 size=0;
    if(!source.isSymLink())
        size=source.size();
    const std::string &drive=driveManagement.getDrive(destination.absoluteFilePath().toStdString());
    if(!drive.empty())//can be a network drive
        if(mode!=Ultracopier::Move || drive!=driveManagement.getDrive(source.absoluteFilePath().toStdString()))
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1, destination: %2, add entry: %3, size: %4, size2: %5, isSymLink: %6").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).arg(temp.id).arg(temp.size).arg(size).arg(source.isSymLink()).toStdString());
    return temp.id;
}

Ultracopier::ItemOfCopyList ListThread::actionToDoTransferToItemOfCopyList(const ListThread::ActionToDoTransfer &actionToDoTransfer)
{
    Ultracopier::ItemOfCopyList itemOfCopyList;
    itemOfCopyList.id			= actionToDoTransfer.id;
    itemOfCopyList.sourceFullPath	= actionToDoTransfer.source.absoluteFilePath().toStdString();
    itemOfCopyList.sourceFileName	= actionToDoTransfer.source.fileName().toStdString();
    itemOfCopyList.destinationFullPath	= actionToDoTransfer.destination.absoluteFilePath().toStdString();
    itemOfCopyList.destinationFileName	= actionToDoTransfer.destination.fileName().toStdString();
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

//warning the first entry is accessible will copy
void ListThread::removeItems(const std::vector<uint64_t> &ids)
{
    for(unsigned int i=0;i<ids.size();i++)
        skipInternal(ids.at(i));
}

//put on top
void ListThread::moveItemsOnTop(std::vector<uint64_t> ids)
{
    if(actionToDoListTransfer.size()<=1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list size is empty");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //do list operation
    int indexToMove=0;
    for (unsigned int i=0; i<actionToDoListTransfer.size(); ++i) {
        if(vectorcontainsAtLeastOne(ids,actionToDoListTransfer.at(i).id))
        {
            vectorremoveOne(ids,actionToDoListTransfer.at(i).id);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"move item "+std::to_string(i)+" to "+std::to_string(indexToMove));
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=actionToDoListTransfer.at(i).id;
            newAction.userAction.moveAt=indexToMove;
            newAction.userAction.position=i;
            actionDone.push_back(newAction);
            ActionToDoTransfer temp=actionToDoListTransfer.at(i);
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+i);
            actionToDoListTransfer.insert(actionToDoListTransfer.cbegin()+indexToMove,temp);
            indexToMove++;
            if(ids.empty())
                return;
        }
    }
    sendActionDone();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//move up
void ListThread::moveItemsUp(std::vector<uint64_t> ids)
{
    if(actionToDoListTransfer.size()<=1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list size is empty");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //do list operation
    int lastGoodPositionReal=0;
    bool haveGoodPosition=false;
    for (unsigned int i=0; i<actionToDoListTransfer.size(); ++i) {
        if(vectorcontainsAtLeastOne(ids,actionToDoListTransfer.at(i).id))
        {
            if(haveGoodPosition)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"move item "+std::to_string(i)+" to "+std::to_string(i-1));
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type=Ultracopier::MoveItem;
                newAction.addAction.id=actionToDoListTransfer.at(i).id;
                newAction.userAction.moveAt=lastGoodPositionReal;
                newAction.userAction.position=i;
                actionDone.push_back(newAction);
                ActionToDoTransfer temp1=actionToDoListTransfer.at(i);
                ActionToDoTransfer temp2=actionToDoListTransfer.at(lastGoodPositionReal);
                actionToDoListTransfer[i]=temp2;
                actionToDoListTransfer[lastGoodPositionReal]=temp1;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Try move up false, item "+std::to_string(i));
            vectorremoveOne(ids,actionToDoListTransfer.at(i).id);
            if(ids.empty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop with return");
                return;
            }
        }
        else
        {
            lastGoodPositionReal=i;
            haveGoodPosition=true;
        }
    }
    sendActionDone();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//move down
void ListThread::moveItemsDown(std::vector<uint64_t> ids)
{
    if(actionToDoListTransfer.size()<=1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list size is empty");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //do list operation
    int lastGoodPositionReal=0;
    bool haveGoodPosition=false;
    for (int i=actionToDoListTransfer.size()-1; i>=0; --i) {
        if(vectorcontainsAtLeastOne(ids,actionToDoListTransfer.at(i).id))
        {
            if(haveGoodPosition)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"move item "+std::to_string(i)+" to "+std::to_string(i+1));
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type=Ultracopier::MoveItem;
                newAction.addAction.id=actionToDoListTransfer.at(i).id;
                newAction.userAction.moveAt=lastGoodPositionReal;
                newAction.userAction.position=i;
                actionDone.push_back(newAction);
                ActionToDoTransfer temp1=actionToDoListTransfer.at(i);
                ActionToDoTransfer temp2=actionToDoListTransfer.at(lastGoodPositionReal);
                actionToDoListTransfer[i]=temp2;
                actionToDoListTransfer[lastGoodPositionReal]=temp1;
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Try move up false, item "+std::to_string(i));
            }
            vectorremoveOne(ids,actionToDoListTransfer.at(i).id);
            if(ids.empty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop with return");
                return;
            }
        }
        else
        {
            lastGoodPositionReal=i;
            haveGoodPosition=true;
        }
    }
    sendActionDone();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//put on bottom
void ListThread::moveItemsOnBottom(std::vector<uint64_t> ids)
{
    if(actionToDoListTransfer.size()<=1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list size is empty");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //do list operation
    int lastGoodPositionReal=actionToDoListTransfer.size()-1;
    for (int i=lastGoodPositionReal; i>=0; --i) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Check action on item "+std::to_string(i));
        if(vectorcontainsAtLeastOne(ids,actionToDoListTransfer.at(i).id))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"move item "+std::to_string(i)+" to "+std::to_string(lastGoodPositionReal));
            vectorremoveOne(ids,actionToDoListTransfer.at(i).id);
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=actionToDoListTransfer.at(i).id;
            newAction.userAction.moveAt=lastGoodPositionReal;
            newAction.userAction.position=i;
            actionDone.push_back(newAction);
            ActionToDoTransfer temp=actionToDoListTransfer.at(i);
            actionToDoListTransfer.erase(actionToDoListTransfer.cbegin()+i);
            actionToDoListTransfer.insert(actionToDoListTransfer.cbegin()+lastGoodPositionReal,temp);
            lastGoodPositionReal--;
            if(ids.empty())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop with return");
                return;
            }
        }
    }
    sendActionDone();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

/** \brief give the forced mode, to export/import transfer list */
void ListThread::forceMode(const Ultracopier::CopyMode &mode)
{
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(mode==Ultracopier::Move)
        setRsync(false);
    #endif
    if(mode==Ultracopier::Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to copy");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to move");
    this->mode=mode;
    forcedMode=true;
}

void ListThread::exportTransferList(const std::string &fileName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QFile transferFile(QString::fromStdString(fileName));
    if(transferFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        transferFile.write(QStringLiteral("Ultracopier;Transfer-list;").toUtf8());
        if(!forcedMode)
            transferFile.write(QStringLiteral("Transfer;").toUtf8());
        else
        {
            if(mode==Ultracopier::Copy)
                transferFile.write(QStringLiteral("Copy;").toUtf8());
            else
                transferFile.write(QStringLiteral("Move;").toUtf8());
        }
        transferFile.write(QStringLiteral("Ultracopier\n").toUtf8());
        bool haveError=false;
        int size=actionToDoListTransfer.size();
        for (int index=0;index<size;++index) {
            if(actionToDoListTransfer.at(index).mode==Ultracopier::Copy)
            {
                if(!forcedMode || mode==Ultracopier::Copy)
                {
                    if(forcedMode)
                        transferFile.write(QStringLiteral("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QStringLiteral("Copy;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
            else if(actionToDoListTransfer.at(index).mode==Ultracopier::Move)
            {
                if(!forcedMode || mode==Ultracopier::Move)
                {
                    if(forcedMode)
                        transferFile.write(QStringLiteral("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QStringLiteral("Move;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
        }
        if(haveError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()).toStdString());
            emit errorTransferList(tr("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()).toStdString());
        }
        transferFile.close();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Unable to save the transfer list: %1").arg(transferFile.errorString()).toStdString());
        emit errorTransferList(tr("Unable to save the transfer list: %1").arg(transferFile.errorString()).toStdString());
        return;
    }
}

void ListThread::importTransferList(const std::string &fileName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QFile transferFile(QString::fromStdString(fileName));
    if(transferFile.open(QIODevice::ReadOnly))
    {
        std::string content;
        QByteArray data=transferFile.readLine(64);
        if(data.size()<=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Problem reading file, or file-size is 0");
            emit errorTransferList(tr("Problem reading file, or file-size is 0").toStdString());
            return;
        }
        content=QString::fromUtf8(data).toStdString();
        if(content!="Ultracopier;Transfer-list;Transfer;Ultracopier\n" && content!="Ultracopier;Transfer-list;Copy;Ultracopier\n" && content!="Ultracopier;Transfer-list;Move;Ultracopier\n")
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Wrong header: "+content);
            emit errorTransferList(tr("Wrong header: \"%1\"").arg(QString::fromStdString(content)).toStdString());
            return;
        }
        bool transferListMixedMode=false;
        if(content=="Ultracopier;Transfer-list;Transfer;Ultracopier\n")
        {
            if(forcedMode)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The transfer list is in mixed mode, but this instance is not");
                emit errorTransferList(tr("The transfer list is in mixed mode, but this instance is not in this mode").toStdString());
                return;
            }
            else
                transferListMixedMode=true;
        }
        if(content=="Ultracopier;Transfer-list;Copy;Ultracopier\n" && (forcedMode && mode==Ultracopier::Move))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("The transfer list is in copy mode, but this instance is not: forcedMode: %1, mode: %2").arg(forcedMode).arg(mode).toStdString());
            emit errorTransferList(tr("The transfer list is in copy mode, but this instance is not in this mode").toStdString());
            return;
        }
        if(content=="Ultracopier;Transfer-list;Move;Ultracopier\n" && (forcedMode && mode==Ultracopier::Copy))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("The transfer list is in move mode, but this instance is not: forcedMode: %1, mode: %2").arg(forcedMode).arg(mode).toStdString());
            emit errorTransferList(tr("The transfer list is in move mode, but this instance is not in this mode").toStdString());
            return;
        }

        bool updateTheStatus_copying=actionToDoListTransfer.size()>0 || actionToDoListInode.size()>0 || actionToDoListInode_afterTheTransfer.size()>0;
        Ultracopier::EngineActionInProgress updateTheStatus_action_in_progress;
        if(updateTheStatus_copying)
            updateTheStatus_action_in_progress=Ultracopier::CopyingAndListing;
        else
            updateTheStatus_action_in_progress=Ultracopier::Listing;
        emit actionInProgess(updateTheStatus_action_in_progress);

        bool errorFound=false;
        std::regex correctLine;
        if(transferListMixedMode)
            correctLine=std::regex("^(Copy|Move);[^;]+;[^;]+[\n\r]*$");
        else
            correctLine=std::regex("^[^;]+;[^;]+[\n\r]*$");
        std::vector<std::string> args;
        Ultracopier::CopyMode tempMode;
        do
        {
            data=transferFile.readLine(65535*2);
            if(data.size()>0)
            {
                content=std::string(data.constData(),data.size());
                //do the import here
                if(std::regex_match(content,correctLine))
                {
                    stringreplaceAll(content,"\n","");
                    args=stringsplit(content,';');
                    if(forcedMode)
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("New data to import in forced mode: %2,%3")
                                                 .arg(QString::fromStdString(args.at(0)))
                                                      .arg(QString::fromStdString(args.at(1)))
                                                      .toStdString());
                        addToTransfer(QFileInfo(QString::fromStdString(args.at(0))),QFileInfo(QString::fromStdString(args.at(1))),mode);
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("New data to import: %1,%2,%3")
                                                 .arg(QString::fromStdString(args.at(0)))
                                                 .arg(QString::fromStdString(args.at(1)))
                                                 .arg(QString::fromStdString(args.at(2)))
                                                 .toStdString());
                        if(args.at(0)=="Copy")
                            tempMode=Ultracopier::Copy;
                        else
                            tempMode=Ultracopier::Move;
                        addToTransfer(QFileInfo(QString::fromStdString(args.at(1))),QFileInfo(QString::fromStdString(args.at(2))),tempMode);
                    }
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Wrong line syntax: "+content);
                    errorFound=true;
                }
            }
        }
        while(data.size()>0);
        transferFile.close();
        if(errorFound)
            emit warningTransferList(tr("Some errors have been found during the line parsing").toStdString());

        updateTheStatus();//->sendActionDone(); into this
        autoStartAndCheckSpace();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Unable to open the transfer list: %1").arg(transferFile.errorString()).toStdString());
        emit errorTransferList(tr("Unable to open the transfer list: %1").arg(transferFile.errorString()).toStdString());
        return;
    }
}

int ListThread::getNumberOfTranferRuning() const
{
    int numberOfTranferRuning=0;
    const int &loop_size=transferThreadList.size();
    //lunch the transfer in WaitForTheTransfer
    int int_for_loop=0;
    while(int_for_loop<loop_size)
    {
        if(transferThreadList.at(int_for_loop)->getStat()==TransferStat_Transfer && transferThreadList.at(int_for_loop)->transferId!=0 && transferThreadList.at(int_for_loop)->transferSize>=parallelizeIfSmallerThan)
            numberOfTranferRuning++;
        int_for_loop++;
    }
    return numberOfTranferRuning;
}

//return
bool ListThread::needMoreSpace() const
{
    if(!checkDiskSpace)
        return false;
    std::vector<Diskspace> diskspace_list;
    for( auto& spaceDrive : requiredSpace ) {
        const QString &drive=QString::fromStdString(spaceDrive.first);
        #ifdef Q_OS_WIN32
        if(spaceDrive.first!="A:\\" && spaceDrive.first!="A:/" && spaceDrive.first!="A:" && spaceDrive.first!="A" && spaceDrive.first!="a:\\" && spaceDrive.first!="a:/" && spaceDrive.first!="a:" && spaceDrive.first!="a")
        {
        #endif
            QStorageInfo storageInfo(drive);
            storageInfo.refresh();
            const qint64 &availableSpace=storageInfo.bytesAvailable();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            const qint64 &bytesFree=storageInfo.bytesFree();
            #endif

            if(availableSpace<0 ||
                    //workaround for all 0 value in case of bug from Qt
                    (availableSpace==0 && storageInfo.bytesTotal()==0)
                    )
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("availableSpace: %1, space needed: %2, on: %3, bytesFree: %4").arg(availableSpace).arg(spaceDrive.second).arg(drive).arg(bytesFree).toStdString());
            }
            else if(spaceDrive.second>(quint64)availableSpace)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("availableSpace: %1, space needed: %2, on: %3, bytesFree: %4").arg(availableSpace).arg(spaceDrive.second).arg(drive).arg(bytesFree).toStdString());
                #ifdef Q_OS_WIN32
                //if(drive.contains(QRegularExpression("^[a-zA-Z]:[\\\\/]")))
                if(drive.contains(QRegularExpression("^[a-zA-Z]:")))
                #endif
                {
                    Diskspace diskspace;
                    diskspace.drive=spaceDrive.first;
                    diskspace.freeSpace=availableSpace;
                    diskspace.requiredSpace=spaceDrive.second;
                    diskspace_list.push_back(diskspace);
                }
                #ifdef Q_OS_WIN32
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"not local drive");
                #endif
            }
        #ifdef Q_OS_WIN32
        }
        #endif
    }
    if(!diskspace_list.empty())
        emit missingDiskSpace(diskspace_list);
    return ! diskspace_list.empty();
}

//do new actions
void ListThread::doNewActions_start_transfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, numberOfTranferRuning: %2").arg(actionToDoListTransfer.size()).arg(getNumberOfTranferRuning()).toStdString());
    if(stopIt)
        return;
    int numberOfTranferRuning=getNumberOfTranferRuning();
    const int &loop_size=transferThreadList.size();
    //lunch the transfer in WaitForTheTransfer
    int int_for_loop=0;
    while(int_for_loop<loop_size)
    {
        if(transferThreadList.at(int_for_loop)->getStat()==TransferStat_WaitForTheTransfer)
        {
            if(transferThreadList.at(int_for_loop)->transferSize>=parallelizeIfSmallerThan)
            {
                if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
                {
                    transferThreadList.at(int_for_loop)->startTheTransfer();
                    numberOfTranferRuning++;
                }
            }
            else
                transferThreadList.at(int_for_loop)->startTheTransfer();
        }
        int_for_loop++;
    }
    int_for_loop=0;
    while(int_for_loop<loop_size)
    {
        if(transferThreadList.at(int_for_loop)->getStat()==TransferStat_PreOperation)
        {
            if(transferThreadList.at(int_for_loop)->transferSize>=parallelizeIfSmallerThan)
            {
                if(numberOfTranferRuning<ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)
                {
                    transferThreadList.at(int_for_loop)->startTheTransfer();
                    numberOfTranferRuning++;
                }
            }
            else
                transferThreadList.at(int_for_loop)->startTheTransfer();
        }
        int_for_loop++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+std::to_string(numberOfTranferRuning));
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
    if(stopIt)
        return;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_SCHEDULER
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    #endif
    //lunch the pre-op or inode op
    TransferThread *currentTransferThread;
    int int_for_loop=0;
    int int_for_internal_loop=0;
    int int_for_transfer_thread_search=0;
    actionToDoListTransfer_count=actionToDoListTransfer.size();
    actionToDoListInode_count=actionToDoListInode.size();
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    //search the next transfer action to do
    while(int_for_loop<actionToDoListTransfer_count)
    {
        if(!actionToDoListTransfer.at(int_for_loop).isRunning)
        {
            //search the next inode action to do
            while(int_for_internal_loop<actionToDoListInode_count)
            {
                if(!actionToDoListInode.at(int_for_internal_loop).isRunning)
                {
                    if(actionToDoListTransfer.at(int_for_loop).id<actionToDoListInode.at(int_for_internal_loop).id)
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
            ActionToDoTransfer& currentActionToDoTransfer=actionToDoListTransfer[int_for_loop];
            //do the tranfer action
            while(int_for_transfer_thread_search<loop_sub_size_transfer_thread_search)
            {
                /**
                    transferThreadList.at(int_for_transfer_thread_search)->transferId==0) /!\ important!
                    Because the other thread can have call doNewAction before than this thread have the finish event parsed!
                    I this case it lose all data
                    */
                currentTransferThread=transferThreadList.at(int_for_transfer_thread_search);
                if(currentTransferThread->getStat()==TransferStat_Idle && currentTransferThread->transferId==0) // /!\ important!
                {
                    std::string drive=driveManagement.getDrive(actionToDoListTransfer.at(int_for_internal_loop).destination.absoluteFilePath().toStdString());
                    if(requiredSpace.find(drive)!=requiredSpace.cend() && (actionToDoListTransfer.at(int_for_internal_loop).mode!=Ultracopier::Move || drive!=driveManagement.getDrive(actionToDoListTransfer.at(int_for_internal_loop).source.absoluteFilePath().toStdString())))
                    {
                        requiredSpace[drive]-=actionToDoListTransfer.at(int_for_internal_loop).size;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("space needed removed: %1, space needed: %2, on: %3").arg(actionToDoListTransfer.at(int_for_internal_loop).size).arg(requiredSpace.at(drive)).arg(QString::fromStdString(drive)).toStdString());
                    }
                    currentTransferThread->transferId=currentActionToDoTransfer.id;
                    currentTransferThread->transferSize=currentActionToDoTransfer.size;
                    if(!currentTransferThread->setFiles(
                        currentActionToDoTransfer.source,
                        currentActionToDoTransfer.size,
                        currentActionToDoTransfer.destination,
                        currentActionToDoTransfer.mode
                        ))
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[%1] id: %2 is idle, but seam busy at set name: %3").arg(int_for_loop).arg(currentTransferThread->transferId).arg(currentActionToDoTransfer.destination.absoluteFilePath()).toStdString());
                        break;
                    }
                    currentActionToDoTransfer.isRunning=true;

                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[%1] id: %2 is idle, use it for %3").arg(int_for_loop).arg(currentTransferThread->transferId).arg(currentActionToDoTransfer.destination.absoluteFilePath()).toStdString());

                    /// \note wrong position? Else write why it's here
                    Ultracopier::ReturnActionOnCopyList newAction;
                    newAction.type				= Ultracopier::PreOperation;
                    newAction.addAction.id			= currentActionToDoTransfer.id;
                    newAction.addAction.sourceFullPath	= currentActionToDoTransfer.source.absoluteFilePath().toStdString();
                    newAction.addAction.sourceFileName	= currentActionToDoTransfer.source.fileName().toStdString();
                    newAction.addAction.destinationFullPath	= currentActionToDoTransfer.destination.absoluteFilePath().toStdString();
                    newAction.addAction.destinationFileName	= currentActionToDoTransfer.destination.fileName().toStdString();
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
    TransferThread *transfer=qobject_cast<TransferThread *>(QObject::sender());
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

/// \brief update the transfer stat
void ListThread::newTransferStat(const TransferStat &stat,const quint64 &id)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"TransferStat: "+std::to_string(stat));
    Ultracopier::ReturnActionOnCopyList newAction;
    switch(stat)
    {
        case TransferStat_Idle:
            return;
        break;
        case TransferStat_PreOperation:
            return;
        break;
        case TransferStat_WaitForTheTransfer:
            return;
        break;
        case TransferStat_Transfer:
            newAction.type=Ultracopier::Transfer;
        break;
        case TransferStat_PostTransfer:
        case TransferStat_PostOperation:
            newAction.type=Ultracopier::PostOperation;
        break;
        case TransferStat_Checksum:
            newAction.type=Ultracopier::CustomOperation;
        break;
        default:
            return;
        break;
    }
    newAction.addAction.id			= id;
    actionDone.push_back(newAction);
}

void ListThread::set_osBufferLimit(const unsigned int &osBufferLimit)
{
    this->osBufferLimit=osBufferLimit;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_osBufferLimit(osBufferLimit);
        index++;
    }
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
        if(actionToDoListInode.at(int_for_loop).isRunning)
        {
            if(actionToDoListInode.at(int_for_loop).type==ActionType_MkPath)
            {
                //to send to the log
                emit mkPath(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath().toStdString());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("stop mkpath: %1").arg(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath()).toStdString());
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
            if(actionToDoListInode.at(int_for_loop).type==ActionType_MovePath || actionToDoListInode.at(int_for_loop).type==ActionType_RealMove
                    #ifdef ULTRACOPIER_PLUGIN_RSYNC
                     || actionToDoListInode.at(int_for_loop).type==ActionType_RmSync
                    #endif
                    )
            {
                //to send to the log
                #ifdef ULTRACOPIER_PLUGIN_RSYNC
                if(actionToDoListInode.at(int_for_loop).type!=ActionType_RmSync)
                    emit mkPath(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath().toStdString());
                #else
                emit mkPath(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath().toStdString());
                #endif
                emit rmPath(actionToDoListInode.at(int_for_loop).source.absoluteFilePath().toStdString());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("stop mkpath: %1").arg(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath()).toStdString());
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

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW

void ListThread::timedUpdateDebugDialog()
{
    std::vector<std::string> newList;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        QString stat;
        switch(transferThreadList.at(index)->getStat())
        {
        case TransferStat_Idle:
            stat="Idle";
            break;
        case TransferStat_PreOperation:
            stat="PreOperation";
            break;
        case TransferStat_WaitForTheTransfer:
            stat="WaitForTheTransfer";
            break;
        case TransferStat_Transfer:
            stat="Transfer";
            break;
        case TransferStat_PostOperation:
            stat="PostOperation";
            break;
        case TransferStat_PostTransfer:
            stat="PostTransfer";
            break;
        case TransferStat_Checksum:
            stat="Checksum";
            break;
        default:
            stat=QStringLiteral("??? (%1)").arg(transferThreadList.at(index)->getStat());
            break;
        }
        newList.push_back(QStringLiteral("%1) (%3,%4) %2")
            .arg(index)
            .arg(stat)
            .arg(transferThreadList.at(index)->readingLetter())
            .arg(transferThreadList.at(index)->writingLetter())
                          .toStdString()
                          );
        index++;
    }
    std::vector<std::string> newList2;
    index=0;
    const int &loop_size=actionToDoListTransfer.size();
    while(index<loop_size)
    {
        newList2.push_back(QStringLiteral("%1 %2 %3")
            .arg(actionToDoListTransfer.at(index).source.absoluteFilePath())
            .arg(actionToDoListTransfer.at(index).size)
            .arg(actionToDoListTransfer.at(index).destination.absoluteFilePath())
                           .toStdString()
                           );
        if(index>((inodeThreads+ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)*2+1))
        {
            newList2.push_back("...");
            break;
        }
        index++;
    }
    emit updateTheDebugInfo(newList,newList2,numberOfInodeOperation);
}

#endif

/// \note Can be call without queue because all call will be serialized
void ListThread::fileAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame)
{
    emit send_fileAlreadyExists(source,destination,isSame,qobject_cast<TransferThread *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
void ListThread::errorOnFile(const QFileInfo &fileInfo, const std::string &errorString, const ErrorType &errorType)
{
    TransferThread * transferThread=qobject_cast<TransferThread *>(sender());
    if(transferThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Thread locating error");
        return;
    }
    ErrorLogEntry errorLogEntry;
    errorLogEntry.source=transferThread->getSourceInode();
    errorLogEntry.destination=transferThread->getDestinationInode();
    errorLogEntry.mode=transferThread->getMode();
    errorLogEntry.error=errorString;
    errorLog.push_back(errorLogEntry);
    emit errorToRetry(transferThread->getSourcePath(),transferThread->getDestinationPath(),errorString);
    emit send_errorOnFile(fileInfo,errorString,transferThread,errorType);
}

/// \note Can be call without queue because all call will be serialized
void ListThread::folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame)
{
    emit send_folderAlreadyExists(source,destination,isSame,qobject_cast<ScanFileOrFolder *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void ListThread::errorOnFolder(const QFileInfo &fileInfo,const std::string &errorString,const ErrorType &errorType)
{
    emit send_errorOnFolder(fileInfo,errorString,qobject_cast<ScanFileOrFolder *>(sender()),errorType);
}

//to run the thread
void ListThread::run()
{
    exec();
}

void ListThread::getNeedPutAtBottom(const QFileInfo &fileInfo, const std::string &errorString, TransferThread *thread, const ErrorType &errorType)
{
    if(actionToDoListTransfer.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"can't try put at bottom if empty");
        this->alwaysDoThisActionForFileExists=FileExists_NotSet;
        putAtBottom=0;
        emit haveNeedPutAtBottom(false,fileInfo,errorString,thread,errorType);
        return;
    }
    bool needPutAtBottom=(putAtBottom<(quint32)actionToDoListTransfer.size());
    if(!needPutAtBottom)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Reset put at bottom");
        this->alwaysDoThisActionForFileExists=FileExists_NotSet;
        putAtBottom=0;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Put at bottom for later try");
        thread->putAtBottom();
        putAtBottom++;
        return;
    }
    emit haveNeedPutAtBottom(needPutAtBottom,fileInfo,errorString,thread,errorType);
}

/// \to create transfer thread
void ListThread::createTransferThread()
{
    if(stopIt)
        return;
    if(transferThreadList.size()>=(unsigned int)inodeThreads)
        return;
    transferThreadList.push_back(new TransferThread());
    TransferThread * last=transferThreadList.back();
    last->transferId=0;
    last->transferSize=0;
    last->setRightTransfer(doRightTransfer);
    last->setKeepDate(keepDate);
    if(!last->setBlockSize(blockSize))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the block size: "+std::to_string(blockSize));
    if(!last->setSequentialBuffer(sequentialBuffer))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the sequentialBuffer: "+std::to_string(sequentialBuffer));
    if(!last->setBlockSize(parallelBuffer))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the parallelBuffer: "+std::to_string(parallelBuffer));
    last->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
    last->set_doChecksum(doChecksum);
    last->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    last->set_checksumOnlyOnError(checksumOnlyOnError);
    last->set_osBuffer(osBuffer);
    last->set_osBufferLimited(osBufferLimited);
    last->set_osBufferLimit(osBufferLimit);
    last->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    last->setRsync(rsync);
    #endif

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(last,&TransferThread::debugInformation,             this,&ListThread::debugInformation,             Qt::QueuedConnection);
    #endif // ULTRACOPIER_PLUGIN_DEBUG
    connect(last,&TransferThread::errorOnFile,                  this,&ListThread::errorOnFile,                  Qt::QueuedConnection);
    connect(last,&TransferThread::fileAlreadyExists,            this,&ListThread::fileAlreadyExists,            Qt::QueuedConnection);
    connect(last,&TransferThread::tryPutAtBottom,               this,&ListThread::transferPutAtBottom,          Qt::QueuedConnection);
    connect(last,&TransferThread::readStopped,                  this,&ListThread::doNewActions_start_transfer,  Qt::QueuedConnection);
    connect(last,&TransferThread::preOperationStopped,          this,&ListThread::doNewActions_start_transfer,	Qt::QueuedConnection);
    connect(last,&TransferThread::postOperationStopped,         this,&ListThread::transferInodeIsClosed,        Qt::QueuedConnection);
    connect(last,&TransferThread::checkIfItCanBeResumed,		this,&ListThread::restartTransferIfItCan,       Qt::QueuedConnection);
    connect(last,&TransferThread::pushStat,                     this,&ListThread::newTransferStat,              Qt::QueuedConnection);

    connect(this,&ListThread::send_sendNewRenamingRules,		last,&TransferThread::setRenamingRules,         Qt::QueuedConnection);

    connect(this,&ListThread::send_setTransferAlgorithm,		last,&TransferThread::setTransferAlgorithm,		Qt::QueuedConnection);
    connect(this,&ListThread::send_parallelBuffer,              last,&TransferThread::setParallelBuffer,		Qt::QueuedConnection);
    connect(this,&ListThread::send_sequentialBuffer,            last,&TransferThread::setSequentialBuffer,		Qt::QueuedConnection);
    connect(this,&ListThread::send_updateMount,                 last,&TransferThread::set_updateMount,          Qt::QueuedConnection);

    last->setObjectName(QStringLiteral("transfer %1").arg(transferThreadList.size()-1));
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

void ListThread::setTransferAlgorithm(const TransferAlgorithm &transferAlgorithm)
{
    if(transferAlgorithm==TransferAlgorithm_Sequential)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Sequential");
    else if(transferAlgorithm==TransferAlgorithm_Automatic)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Automatic");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Parallel");
    emit send_setTransferAlgorithm(transferAlgorithm);
}

void ListThread::setParallelBuffer(int parallelBuffer)
{
    if(parallelBuffer<1 || parallelBuffer>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"wrong number of block: "+std::to_string(parallelBuffer));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in number of block: "+std::to_string(parallelBuffer));
    this->parallelBuffer=parallelBuffer;
    emit send_parallelBuffer(parallelBuffer);
}

void ListThread::setSequentialBuffer(int sequentialBuffer)
{
    if(sequentialBuffer<1 || sequentialBuffer>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"wrong number of block: "+std::to_string(sequentialBuffer));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in number of block: "+std::to_string(sequentialBuffer));
    this->sequentialBuffer=sequentialBuffer;
    emit send_sequentialBuffer(sequentialBuffer);
}

void ListThread::setParallelizeIfSmallerThan(const unsigned int &parallelizeIfSmallerThan)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"parallelizeIfSmallerThan in Bytes: "+std::to_string(parallelizeIfSmallerThan));
    this->parallelizeIfSmallerThan=parallelizeIfSmallerThan;
}

void ListThread::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setMoveTheWholeFolder(moveTheWholeFolder);
    this->moveTheWholeFolder=moveTheWholeFolder;
}

void ListThread::setFollowTheStrictOrder(const bool &followTheStrictOrder)
{
    this->followTheStrictOrder=followTheStrictOrder;
}

void ListThread::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
        index++;
    }
}

void ListThread::setInodeThreads(const int &inodeThreads)
{
    if(inodeThreads<1 || inodeThreads>32)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"inodeThreads is out of ranges: "+std::to_string(inodeThreads));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"inodeThreads: "+std::to_string(inodeThreads));
    this->inodeThreads=inodeThreads;
    createTransferThread();
    deleteTransferThread();
}

void ListThread::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"renameTheOriginalDestination: "+std::to_string(renameTheOriginalDestination));
    this->renameTheOriginalDestination=renameTheOriginalDestination;
    int index=0;
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setRenameTheOriginalDestination(renameTheOriginalDestination);
        index++;
    }
}

void ListThread::setCheckDiskSpace(const bool &checkDiskSpace)
{
    this->checkDiskSpace=checkDiskSpace;
}

void ListThread::setCopyListOrder(const bool &order)
{
    this->copyListOrder=order;
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setCopyListOrder(this->copyListOrder);
}

void ListThread::exportErrorIntoTransferList(const std::string &fileName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QFile transferFile(QString::fromStdString(fileName));
    if(transferFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        transferFile.write(QStringLiteral("Ultracopier;Transfer-list;").toUtf8());
        if(!forcedMode)
            transferFile.write(QStringLiteral("Transfer;").toUtf8());
        else
        {
            if(mode==Ultracopier::Copy)
                transferFile.write(QStringLiteral("Copy;").toUtf8());
            else
                transferFile.write(QStringLiteral("Move;").toUtf8());
        }
        transferFile.write(QStringLiteral("Ultracopier\n").toUtf8());
        bool haveError=false;
        int size=errorLog.size();
        for (int index=0;index<size;++index) {
            if(errorLog.at(index).mode==Ultracopier::Copy)
            {
                if(!forcedMode || mode==Ultracopier::Copy)
                {
                    if(forcedMode)
                        transferFile.write(QStringLiteral("%1;%2\n").arg(errorLog.at(index).source.absoluteFilePath()).arg(errorLog.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QStringLiteral("Copy;%1;%2\n").arg(errorLog.at(index).source.absoluteFilePath()).arg(errorLog.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
            else if(errorLog.at(index).mode==Ultracopier::Move)
            {
                if(!forcedMode || mode==Ultracopier::Move)
                {
                    if(forcedMode)
                        transferFile.write(QStringLiteral("%1;%2\n").arg(errorLog.at(index).source.absoluteFilePath()).arg(errorLog.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QStringLiteral("Move;%1;%2\n").arg(errorLog.at(index).source.absoluteFilePath()).arg(errorLog.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
        }
        if(haveError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable do to move or copy item into wrong forced mode: "+transferFile.errorString().toStdString());
            emit errorTransferList(tr("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()).toStdString());
        }
        transferFile.close();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to save the transfer list: "+transferFile.errorString().toStdString());
        emit errorTransferList(tr("Unable to save the transfer list: %1").arg(transferFile.errorString()).toStdString());
        return;
    }
}
