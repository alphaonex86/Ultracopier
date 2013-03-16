#include "ListThread.h"

ListThread::ListThread(FacilityInterface * facilityInterface)
{
    moveToThread(this);
    start(HighPriority);
    this->facilityInterface         = facilityInterface;
    putInPause                      = false;
    sourceDrive                     = "";
    sourceDriveMultiple             = false;
    destinationDrive                = "";
    destinationDriveMultiple        = false;
    stopIt                          = false;
    bytesToTransfer                 = 0;
    bytesTransfered                 = 0;
    idIncrementNumber               = 1;
    actualRealByteTransfered        = 0;
    numberOfTransferIntoToDoList    = 0;
    numberOfInodeOperation          = 0;
    putAtBottom                     = 0;
    maxSpeed                        = 0;
    doRightTransfer                 = false;
    keepDate                        = false;
    blockSize                       = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*1024;
    sequentialBuffer                = ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
    parallelBuffer                  = ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    blockSizeAfterSpeedLimitation   = blockSize;
    osBufferLimit                   = 512;
    alwaysDoThisActionForFileExists = FileExists_NotSet;
    doChecksum                      = false;
    checksumIgnoreIfImpossible      = true;
    checksumOnlyOnError             = true;
    osBuffer                        = false;
    osBufferLimited                 = false;
    forcedMode                      = false;
    clockForTheCopySpeed            = NULL;
    multiForBigSpeed                = 0;

    #if ! defined (Q_CC_GNU)
    ui->keepDate->setEnabled(false);
    ui->keepDate->setToolTip("Not supported with this compiler");
    #endif
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
    temp_transfer_thread=qobject_cast<TransferThread *>(QObject::sender());
    if(temp_transfer_thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("transfer thread not located!"));
        return;
    }
    isFound=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    int countLocalParse=0;
    #endif
    if(temp_transfer_thread->getStat()!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("transfer thread not idle!"));
        return;
    }
    int_for_internal_loop=0;
    loop_size=actionToDoListTransfer.size();
    while(int_for_internal_loop<loop_size)
    {
        if(actionToDoListTransfer.at(int_for_internal_loop).id==temp_transfer_thread->transferId)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("[%1] have finish, put at idle; for id: %2").arg(int_for_internal_loop).arg(temp_transfer_thread->transferId));
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::RemoveItem;
            newAction.userAction.moveAt=0;
            newAction.addAction=actionToDoTransferToItemOfCopyList(actionToDoListTransfer.at(int_for_internal_loop));
            newAction.userAction.position=int_for_internal_loop;
            actionDone << newAction;
            /// \todo check if item is at the right thread
            actionToDoListTransfer.removeAt(int_for_internal_loop);
            if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
                updateTheStatus();

            //add the current size of file, to general size because it's finish
            copiedSize=temp_transfer_thread->copiedSize();
            if(copiedSize>(qint64)temp_transfer_thread->transferSize)
            {
                oversize=copiedSize-temp_transfer_thread->transferSize;
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
            if(actionToDoListTransfer.size()==0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionToDoListTransfer==0");
                actionToDoListInode << actionToDoListInode_afterTheTransfer;
                actionToDoListInode_afterTheTransfer.clear();
                doNewActions_inode_manipulation();
            }
            break;
        }
        int_for_internal_loop++;
    }
    if(!isFound)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(temp_transfer_thread->transferId).arg(int_for_loop));
        temp_transfer_thread->transferId=0;
        temp_transfer_thread->transferSize=0;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("countLocalParse: %1, actionToDoList.size(): %2").arg(countLocalParse).arg(actionToDoListTransfer.size()));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(countLocalParse!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("countLocalParse != 1"));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("transfer thread not located!"));
        return;
    }
    bool isFound=false;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    int countLocalParse=0;
    #endif
    int indexAction=0;
    while(indexAction<actionToDoListTransfer.size())
    {
        if(actionToDoListTransfer.at(indexAction).id==transfer->transferId)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Put at the end: %1").arg(transfer->transferId));
            //push for interface at the end
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=transfer->transferId;
            newAction.userAction.position=actionToDoListTransfer.size()-1;
            actionDone << newAction;
            //do the wait stat
            actionToDoListTransfer[indexAction].isRunning=false;
            //move at the end
            actionToDoListTransfer.move(indexAction,actionToDoListTransfer.size()-1);
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("unable to found item into the todo list, id: %1, index: %2").arg(transfer->transferId));
        transfer->transferId=0;
        transfer->transferSize=0;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("countLocalParse: %1").arg(countLocalParse));
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(countLocalParse!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("countLocalParse != 1"));
    #endif
    transfer->skip();
}

//set the copy info and options before runing
void ListThread::setRightTransfer(const bool doRightTransfer)
{
    mkPathQueue.setRightTransfer(doRightTransfer);
    this->doRightTransfer=doRightTransfer;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setBlockSize(this->blockSize);
        index++;
    }
    setSpeedLimitation(maxSpeed);
}

//set auto start
void ListThread::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
}

//set check destination folder
void ListThread::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    for(int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void ListThread::fileTransfer(const QFileInfo &sourceFileInfo,const QFileInfo &destinationFileInfo,const Ultracopier::CopyMode &mode)
{
    if(stopIt)
        return;
    addToTransfer(sourceFileInfo,destinationFileInfo,mode);
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::haveSameSource(const QStringList &sources)
{
    if(sourceDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDriveMultiple");
        return false;
    }
    if(sourceDrive.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDrive.isEmpty()");
        return true;
    }
    int index=0;
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
bool ListThread::haveSameDestination(const QString &destination)
{
    if(destinationDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDriveMultiple");
        return false;
    }
    if(destinationDrive.isEmpty())
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

ScanFileOrFolder * ListThread::newScanThread(Ultracopier::CopyMode mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start with: "+QString::number(mode));

    //create new thread because is auto-detroyed
    scanFileOrFolderThreadsPool << new ScanFileOrFolder(mode);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::finishedTheListing,				this,&ListThread::scanThreadHaveFinishSlot,	Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::fileTransfer,                     this,&ListThread::fileTransfer,			Qt::QueuedConnection);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::debugInformation,					this,&ListThread::debugInformation,		Qt::QueuedConnection);
    #endif
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::newFolderListing,					this,&ListThread::newFolderListing);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::addToMovePath,					this,&ListThread::addToMovePath,			Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::addToRealMove,					this,&ListThread::addToRealMove,			Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::addToMkPath,                      this,&ListThread::addToMkPath,			Qt::QueuedConnection);

    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::errorOnFolder,					this,&ListThread::errorOnFolder,		Qt::QueuedConnection);
    connect(scanFileOrFolderThreadsPool.last(),&ScanFileOrFolder::folderAlreadyExists,				this,&ListThread::folderAlreadyExists,		Qt::QueuedConnection);

    scanFileOrFolderThreadsPool.last()->setFilters(include,exclude);
    scanFileOrFolderThreadsPool.last()->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
    if(scanFileOrFolderThreadsPool.size()==1)
        updateTheStatus();
    scanFileOrFolderThreadsPool.last()->setRenamingRules(firstRenamingRule,otherRenamingRule);
    return scanFileOrFolderThreadsPool.last();
}

void ListThread::scanThreadHaveFinishSlot()
{
    scanThreadHaveFinish();
}

void ListThread::scanThreadHaveFinish(bool skipFirstRemove)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"listing thread have finish, skipFirstRemove: "+QString::number(skipFirstRemove));
    if(!skipFirstRemove)
    {
        ScanFileOrFolder * senderThread = qobject_cast<ScanFileOrFolder *>(QObject::sender());
        if(senderThread==NULL)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"sender pointer null (plugin copy engine)");
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
            delete senderThread;
            scanFileOrFolderThreadsPool.removeOne(senderThread);
            if(scanFileOrFolderThreadsPool.size()==0)
                updateTheStatus();
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the next thread, scanFileOrFolderThreadsPool.size(): "+QString::number(scanFileOrFolderThreadsPool.size()));
    if(scanFileOrFolderThreadsPool.size()>0)
    {
        //then start the next listing threads
        if(scanFileOrFolderThreadsPool.first()->isFinished())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Start listing thread");
            scanFileOrFolderThreadsPool.first()->start();
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"The listing thread is already running");
    }
    else
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

void ListThread::startGeneralTransfer()
{
    doNewActions_inode_manipulation();
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newCopy(const QStringList &sources,const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+sources.join(";")+", destination: "+destination);
    ScanFileOrFolder * scanFileOrFolderThread = newScanThread(Ultracopier::Copy);
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

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newMove(const QStringList &sources,const QString &destination)
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

void ListThread::detectDrivesOfCurrentTransfer(const QStringList &sources,const QString &destination)
{
    /* code to detect volume/mount point to group by windows */
    if(!sourceDriveMultiple)
    {
        int index=0;
        while(index<sources.size())
        {
            QString tempDrive=driveManagement.getDrive(sources.at(index));
            //if have not already source, set the source
            if(sourceDrive.isEmpty())
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source informations, sourceDrive: %1, sourceDriveMultiple: %2").arg(sourceDrive).arg(sourceDriveMultiple));
    if(!destinationDriveMultiple)
    {
        QString tempDrive=driveManagement.getDrive(destination);
        //if have not already destination, set the destination
        if(destinationDrive.isEmpty())
            destinationDrive=tempDrive;
        //if have previous destination and the news destination is not the same
        if(destinationDrive!=tempDrive)
            destinationDriveMultiple=true;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("destination informations, destinationDrive: %1, destinationDriveMultiple: %2").arg(destinationDrive).arg(destinationDriveMultiple));
}

void ListThread::setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType)
{
    this->mountSysPoint=mountSysPoint;
    this->driveType=driveType;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("drives: %1").arg(drives.join(";")));
    driveManagement.setDrive(mountSysPoint,driveType);
    emit send_setDrive(mountSysPoint,driveType);
}

void ListThread::setCollisionAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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

bool ListThread::getReturnBoolToCopyEngine()
{
    return returnBoolToCopyEngine;
}

QPair<quint64,quint64> ListThread::getReturnPairQuint64ToCopyEngine()
{
    return returnPairQuint64ToCopyEngine;
}

Ultracopier::ItemOfCopyList ListThread::getReturnItemOfCopyListToCopyEngine()
{
    return returnItemOfCopyListToCopyEngine;
}

void ListThread::set_doChecksum(bool doChecksum)
{
    this->doChecksum=doChecksum;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
    if(putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Seam already in pause!");
        return;
    }
    putInPause=true;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->pause();
        index++;
    }
    emit isInPause(true);
}

void ListThread::resume()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!putInPause)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Seam already resumed!");
        return;
    }
    putInPause=false;
    startGeneralTransfer();
    doNewActions_start_transfer();
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->resume();
        index++;
    }
    emit isInPause(false);
}

void ListThread::skip(const quint64 &id)
{
    skipInternal(id);
}

bool ListThread::skipInternal(const quint64 &id)
{
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        if(transferThreadList.at(index)->transferId==id)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("skip one transfer: %1").arg(id));
            transferThreadList.at(index)->skip();
            return true;
        }
        index++;
    }
    int_for_internal_loop=0;
    loop_size=actionToDoListTransfer.size();
    while(int_for_internal_loop<loop_size)
    {
        if(actionToDoListTransfer.at(int_for_internal_loop).id==id)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("[%1] remove at not running, for id: %2").arg(int_for_internal_loop).arg(id));
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::RemoveItem;
            newAction.userAction.moveAt=1;
            newAction.addAction=actionToDoTransferToItemOfCopyList(actionToDoListTransfer.at(int_for_internal_loop));
            newAction.userAction.position=int_for_internal_loop;
            actionDone << newAction;
            actionToDoListTransfer.removeAt(int_for_internal_loop);
            if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
                updateTheStatus();
            return true;
        }
        int_for_internal_loop++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("skip transfer not found: %1").arg(id));
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
    loop_size=transferThreadList.size();
    while(index<loop_size)
    {
        transferThreadList.at(index)->stop();
        delete transferThreadList.at(index);//->deleteLayer();
        transferThreadList[index]=NULL;
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
    if(clockForTheCopySpeed!=NULL)
    {
        clockForTheCopySpeed->stop();
        delete clockForTheCopySpeed;
        clockForTheCopySpeed=NULL;
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
bool ListThread::setSpeedLimitation(const qint64 &speedLimitation)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed in KB/s: "+QString::number(speedLimitation));

    if(speedLimitation>1024*1024)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"speedLimitation out of range");
        return false;
    }
    maxSpeed=speedLimitation;

    /*    if(this->maxSpeed==0 && maxSpeed==0 && waitNewClockForSpeed.available()>0)
            waitNewClockForSpeed.tryAcquire(waitNewClockForSpeed.available());*/
    multiForBigSpeed=0;
    if(maxSpeed>0)
    {
        blockSizeAfterSpeedLimitation=blockSize;

        //try resolv the interval
        int newInterval;//in ms
        do
        {
            multiForBigSpeed++;
            //at max speed, is out of range for int, it's why quint64 is used
            newInterval=(((quint64)blockSize*(quint64)multiForBigSpeed)/((quint64)maxSpeed*(quint64)1024));
            if(newInterval<0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated newInterval wrong");
                return false;
            }
        }
        while(newInterval<ULTRACOPIER_PLUGIN_MINTIMERINTERVAL);

        if(newInterval<=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated newInterval wrong");
            return false;
        }
        //wait time too big, then shrink the block size and set interval to max size
        if(newInterval>ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"wait time too big, then shrink the block size and set interval to max size");
            newInterval=ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL;
            multiForBigSpeed=1;
            blockSizeAfterSpeedLimitation=(this->maxSpeed*1024*newInterval)/1000;

            if(blockSizeAfterSpeedLimitation<10)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated block size wrong");
                return false;
            }

            //set the new block size into the thread
            loop_size=transferThreadList.size();
            int_for_loop=0;
            while(int_for_loop<loop_size)
            {
                if(!transferThreadList.at(int_for_loop)->setBlockSize(blockSizeAfterSpeedLimitation))
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the block size");
                int_for_loop++;
            }
        }

        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("fixed speed with new block size and new interval in Bytes/s: %1, new BlockSize: %2, multiForBigSpeed: %3, newInterval: %4").arg(
                                     (float)blockSizeAfterSpeedLimitation*multiForBigSpeed//block size
                                     /
                                     ((float)newInterval/(float)1000)//interval
                                     )
                                 .arg(blockSizeAfterSpeedLimitation)
                                 .arg(multiForBigSpeed)
                                 .arg(newInterval)
                                 );

        clockForTheCopySpeed->setInterval(newInterval);
        if(clockForTheCopySpeed!=NULL)
            clockForTheCopySpeed->start();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"clockForTheCopySpeed == NULL at this point");
    }
    else
    {
        if(clockForTheCopySpeed!=NULL)
            clockForTheCopySpeed->stop();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"clockForTheCopySpeed == NULL at this point");
        int_for_loop=0;
        loop_size=transferThreadList.size();
        while(int_for_loop<loop_size)
        {
            transferThreadList.at(int_for_loop)->setBlockSize(blockSize);
            int_for_loop++;
        }
    }
    int_for_loop=0;
    loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        transferThreadList.at(int_for_loop)->setMultiForBigSpeed(multiForBigSpeed);
        int_for_loop++;
    }

    return true;
}

void ListThread::updateTheStatus()
{
    /*if(threadOfTheTransfer.haveContent())
        copy=true;*/
    updateTheStatus_listing=scanFileOrFolderThreadsPool.size()>0;
    updateTheStatus_copying=actionToDoListTransfer.size()>0 || actionToDoListInode.size()>0 || actionToDoListInode_afterTheTransfer.size()>0;
    if(updateTheStatus_copying && updateTheStatus_listing)
        updateTheStatus_action_in_progress=Ultracopier::CopyingAndListing;
    else if(updateTheStatus_listing)
        updateTheStatus_action_in_progress=Ultracopier::Listing;
    else if(updateTheStatus_copying)
        updateTheStatus_action_in_progress=Ultracopier::Copying;
    else
        updateTheStatus_action_in_progress=Ultracopier::Idle;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit actionInProgess("+QString::number(updateTheStatus_action_in_progress)+")");
    emit actionInProgess(updateTheStatus_action_in_progress);
    if(updateTheStatus_action_in_progress==Ultracopier::Idle)
        sendActionDone();
}

//set data local to the thread
void ListThread::setAlwaysFileExistsAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    int_for_loop=0;
    loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        transferThreadList.at(int_for_loop)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
        int_for_loop++;
    }
}

//mk path to do
quint64 ListThread::addToMkPath(const QFileInfo& source,const QFileInfo& destination)
{
    if(stopIt)
        return 0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    ActionToDoInode temp;
    temp.type       = ActionType_MkPath;
    temp.id         = generateIdNumber();
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode << temp;
    return temp.id;
}

//add rm path to do
void ListThread::addToMovePath(const QFileInfo& source, const QFileInfo &destination, const int& inodeToRemove)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2, inodeToRemove: %3").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).arg(inodeToRemove));
    ActionToDoInode temp;
    temp.type	= ActionType_MovePath;
    temp.id		= generateIdNumber();
    temp.size	= inodeToRemove;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode << temp;
}

void ListThread::addToRealMove(const QFileInfo& source,const QFileInfo& destination)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()));
    ActionToDoInode temp;
    temp.type	= ActionType_RealMove;
    temp.id		= generateIdNumber();
    temp.size	= 0;
    temp.source     = source;
    temp.destination= destination;
    temp.isRunning	= false;
    actionToDoListInode << temp;
}

//send action done
void ListThread::sendActionDone()
{
    if(actionDone.size()>0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        emit newActionOnList(actionDone);
        actionDone.clear();
    }
}

//send progression
void ListThread::sendProgression()
{
    if(actionToDoListTransfer.size()==0)
        return;
    oversize=0;
    currentProgression=0;
    int_for_loop=0;
    loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        temp_transfer_thread=transferThreadList.at(int_for_loop);
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
                QPair<quint64,quint64> progression=temp_transfer_thread->progression();
                tempItem.currentRead=progression.first;
                tempItem.currentWrite=progression.second;
                tempItem.id=temp_transfer_thread->transferId;
                tempItem.total=totalSize;
                progressionList << tempItem;

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
    loop_size=actionToDoListTransfer.size();
    loop_sub_size=transferThreadList.size();
    //this loop to have at max ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT*ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT, not ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT*transferThreadList.size()
    for(int_for_loop=0; int_for_loop<loop_size; ++int_for_loop) {
        const ActionToDoTransfer &item=actionToDoListTransfer.at(int_for_loop);
        Ultracopier::ReturnActionOnCopyList newAction;
        newAction.type				= Ultracopier::PreOperation;
        newAction.addAction.id			= item.id;
        newAction.addAction.sourceFullPath	= item.source.absoluteFilePath();
        newAction.addAction.sourceFileName	= item.source.fileName();
        newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath();
        newAction.addAction.destinationFileName	= item.destination.fileName();
        newAction.addAction.size		= item.size;
        newAction.addAction.mode		= item.mode;
        actionDone << newAction;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, size: %2, name: %3, size2: %4").arg(item.id).arg(item.size).arg(item.source.absoluteFilePath()).arg(newAction.addAction.size));
        if(item.isRunning)
        {
            for(int_for_internal_loop=0; int_for_internal_loop<loop_sub_size; ++int_for_internal_loop) {
                transferThread=transferThreadList.at(int_for_internal_loop);
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type				= Ultracopier::PreOperation;
                newAction.addAction.id			= item.id;
                newAction.addAction.sourceFullPath	= item.source.absoluteFilePath();
                newAction.addAction.sourceFileName	= item.source.fileName();
                newAction.addAction.destinationFullPath	= item.destination.absoluteFilePath();
                newAction.addAction.destinationFileName	= item.destination.fileName();
                newAction.addAction.size		= item.size;
                newAction.addAction.mode		= item.mode;
                actionDone << newAction;
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
                    actionDone << newAction;
                }
            }
        }
    }
}

//add file transfer to do
quint64 ListThread::addToTransfer(const QFileInfo& source,const QFileInfo& destination,const Ultracopier::CopyMode& mode)
{
    if(stopIt)
        return 0;
    //add to transfer list
    numberOfTransferIntoToDoList++;
    quint64 size=0;
    if(!source.isSymLink())
        size=source.size();
    bytesToTransfer+= size;
    ActionToDoTransfer temp;
    temp.id		= generateIdNumber();
    temp.size	= size;
    temp.source	= source;
    temp.destination= destination;
    temp.mode	= mode;
    temp.isRunning	= false;
    actionToDoListTransfer << temp;
    //push the new transfer to interface
    Ultracopier::ReturnActionOnCopyList newAction;
    newAction.type				= Ultracopier::AddingItem;
    newAction.addAction=actionToDoTransferToItemOfCopyList(temp);
    actionDone << newAction;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("source: %1, destination: %2, add entry: %3, size: %4, size2: %5").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).arg(temp.id).arg(temp.size).arg(newAction.addAction.size));
    return temp.id;
}

Ultracopier::ItemOfCopyList ListThread::actionToDoTransferToItemOfCopyList(const ListThread::ActionToDoTransfer &actionToDoTransfer)
{
    Ultracopier::ItemOfCopyList itemOfCopyList;
    itemOfCopyList.id			= actionToDoTransfer.id;
    itemOfCopyList.sourceFullPath	= actionToDoTransfer.source.absoluteFilePath();
    itemOfCopyList.sourceFileName	= actionToDoTransfer.source.fileName();
    itemOfCopyList.destinationFullPath	= actionToDoTransfer.destination.absoluteFilePath();
    itemOfCopyList.destinationFileName	= actionToDoTransfer.destination.fileName();
    itemOfCopyList.size		= actionToDoTransfer.size;
    itemOfCopyList.mode		= actionToDoTransfer.mode;
    return itemOfCopyList;
}

//generate id number
quint64 ListThread::generateIdNumber()
{
    idIncrementNumber++;
    if(idIncrementNumber>(((quint64)1024*1024)*1024*1024*2))
        idIncrementNumber=0;
    return idIncrementNumber;
}

//warning the first entry is accessible will copy
void ListThread::removeItems(const QList<int> &ids)
{
    for(int i=0;i<ids.size();i++)
        skipInternal(ids.at(i));
}

//put on top
void ListThread::moveItemsOnTop(QList<int> ids)
{
    if(actionToDoListTransfer.size()<=1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list size is empty");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //do list operation
    int indexToMove=0;
    loop_size=actionToDoListTransfer.size();
    for (int i=0; i<loop_size; ++i) {
        if(ids.contains(actionToDoListTransfer.at(i).id))
        {
            ids.removeOne(actionToDoListTransfer.at(i).id);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(indexToMove));
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=actionToDoListTransfer.at(i).id;
            newAction.userAction.moveAt=indexToMove;
            newAction.userAction.position=i;
            actionDone << newAction;
            actionToDoListTransfer.move(i,indexToMove);
            indexToMove++;
            if(ids.size()==0)
            {
                return;
            }
        }
    }
    sendActionDone();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//move up
void ListThread::moveItemsUp(QList<int> ids)
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
    loop_size=actionToDoListTransfer.size();
    for (int i=0; i<loop_size; ++i) {
        if(ids.contains(actionToDoListTransfer.at(i).id))
        {
            if(haveGoodPosition)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i-1));
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type=Ultracopier::MoveItem;
                newAction.addAction.id=actionToDoListTransfer.at(i).id;
                newAction.userAction.moveAt=lastGoodPositionReal;
                newAction.userAction.position=i;
                actionDone << newAction;
                actionToDoListTransfer.swap(i,lastGoodPositionReal);
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
            ids.removeOne(actionToDoListTransfer.at(i).id);
            if(ids.size()==0)
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
void ListThread::moveItemsDown(QList<int> ids)
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
        if(ids.contains(actionToDoListTransfer.at(i).id))
        {
            if(haveGoodPosition)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(i+1));
                Ultracopier::ReturnActionOnCopyList newAction;
                newAction.type=Ultracopier::MoveItem;
                newAction.addAction.id=actionToDoListTransfer.at(i).id;
                newAction.userAction.moveAt=lastGoodPositionReal;
                newAction.userAction.position=i;
                actionDone << newAction;
                actionToDoListTransfer.swap(i,lastGoodPositionReal);
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Try move up false, item ")+QString::number(i));
            }
            ids.removeOne(actionToDoListTransfer.at(i).id);
            if(ids.size()==0)
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
void ListThread::moveItemsOnBottom(QList<int> ids)
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Check action on item ")+QString::number(i));
        if(ids.contains(actionToDoListTransfer.at(i).id))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("move item ")+QString::number(i)+" to "+QString::number(lastGoodPositionReal));
            ids.removeOne(actionToDoListTransfer.at(i).id);
            Ultracopier::ReturnActionOnCopyList newAction;
            newAction.type=Ultracopier::MoveItem;
            newAction.addAction.id=actionToDoListTransfer.at(i).id;
            newAction.userAction.moveAt=lastGoodPositionReal;
            newAction.userAction.position=i;
            actionDone << newAction;
            actionToDoListTransfer.move(i,lastGoodPositionReal);
            lastGoodPositionReal--;
            if(ids.size()==0)
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
    if(mode==Ultracopier::Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to copy"));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to move"));
    this->mode=mode;
    forcedMode=true;
}

void ListThread::exportTransferList(const QString &fileName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QFile transferFile(fileName);
    if(transferFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        transferFile.write(QString("Ultracopier-0.3;Transfer-list;").toUtf8());
        if(!forcedMode)
            transferFile.write(QString("Transfer;").toUtf8());
        else
        {
            if(mode==Ultracopier::Copy)
                transferFile.write(QString("Copy;").toUtf8());
            else
                transferFile.write(QString("Move;").toUtf8());
        }
        transferFile.write(QString("Ultracopier-0.3\n").toUtf8());
        bool haveError=false;
        int size=actionToDoListTransfer.size();
        for (int index=0;index<size;++index) {
            if(actionToDoListTransfer.at(index).mode==Ultracopier::Copy)
            {
                if(!forcedMode || mode==Ultracopier::Copy)
                {
                    if(forcedMode)
                        transferFile.write(QString("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QString("Copy;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
            else if(actionToDoListTransfer.at(index).mode==Ultracopier::Move)
            {
                if(!forcedMode || mode==Ultracopier::Move)
                {
                    if(forcedMode)
                        transferFile.write(QString("Move;%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                    else
                        transferFile.write(QString("%1;%2\n").arg(actionToDoListTransfer.at(index).source.absoluteFilePath()).arg(actionToDoListTransfer.at(index).destination.absoluteFilePath()).toUtf8());
                }
                else
                    haveError=true;
            }
        }
        if(haveError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()));
            emit errorTransferList(tr("Unable do to move or copy item into wrong forced mode: %1").arg(transferFile.errorString()));
        }
        transferFile.close();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to save the transfer list: %1").arg(transferFile.errorString()));
        emit errorTransferList(tr("Unable to save the transfer list: %1").arg(transferFile.errorString()));
        return;
    }
}

void ListThread::importTransferList(const QString &fileName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QFile transferFile(fileName);
    if(transferFile.open(QIODevice::ReadOnly))
    {
        QString content;
        QByteArray data=transferFile.readLine(64);
        if(data.size()<=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Problem at the reading, or file size is null"));
            emit errorTransferList(tr("Problem at the reading, or file size is null"));
            return;
        }
        content=QString::fromUtf8(data);
        if(content!="Ultracopier-0.3;Transfer-list;Transfer;Ultracopier-0.3\n" && content!="Ultracopier-0.3;Transfer-list;Copy;Ultracopier-0.3\n" && content!="Ultracopier-0.3;Transfer-list;Move;Ultracopier-0.3\n")
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Wrong header: \"%1\"").arg(content));
            emit errorTransferList(tr("Wrong header: \"%1\"").arg(content));
            return;
        }
        bool transferListMixedMode=false;
        if(content=="Ultracopier-0.3;Transfer-list;Transfer;Ultracopier-0.3\n")
        {
            if(forcedMode)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("The transfer list is in mixed mode, but this instance is not in this mode"));
                emit errorTransferList(tr("The transfer list is in mixed mode, but this instance is not in this mode"));
                return;
            }
            else
                transferListMixedMode=true;
        }
        if(content=="Ultracopier-0.3;Transfer-list;Copy;Ultracopier-0.3\n" && (forcedMode && mode==Ultracopier::Move))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("The transfer list is in copy mode, but this instance is not in this mode: forcedMode: %1, mode: %2").arg(forcedMode).arg(mode));
            emit errorTransferList(tr("The transfer list is in copy mode, but this instance is not in this mode"));
            return;
        }
        if(content=="Ultracopier-0.3;Transfer-list;Move;Ultracopier-0.3\n" && (forcedMode && mode==Ultracopier::Copy))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("The transfer list is in move mode, but this instance is not in this mode: forcedMode: %1, mode: %2").arg(forcedMode).arg(mode));
            emit errorTransferList(tr("The transfer list is in move mode, but this instance is not in this mode"));
            return;
        }

        updateTheStatus_copying=actionToDoListTransfer.size()>0 || actionToDoListInode.size()>0 || actionToDoListInode_afterTheTransfer.size()>0;
        if(updateTheStatus_copying)
            updateTheStatus_action_in_progress=Ultracopier::CopyingAndListing;
        else
            updateTheStatus_action_in_progress=Ultracopier::Listing;
        emit actionInProgess(updateTheStatus_action_in_progress);

        bool errorFound=false;
        QRegularExpression correctLine;
        if(transferListMixedMode)
            correctLine=QRegularExpression("^(Copy|Move);[^;]+;[^;]+\n$");
        else
            correctLine=QRegularExpression("^[^;]+;[^;]+\n$");
        QStringList args;
        Ultracopier::CopyMode tempMode;
        do
        {
            data=transferFile.readLine(65535*2);
            if(data.size()>0)
            {
                content=QString::fromUtf8(data);
                //do the import here
                if(content.contains(correctLine))
                {
                    content.remove("\n");
                    args=content.split(";");
                    if(forcedMode)
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("New data to import in forced mode: %2,%3").arg(args.at(0)).arg(args.at(1)));
                        addToTransfer(QFileInfo(args.at(0)),QFileInfo(args.at(1)),mode);
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("New data to import: %1,%2,%3").arg(args.at(0)).arg(args.at(1)).arg(args.at(2)));
                        if(args.at(0)=="Copy")
                            tempMode=Ultracopier::Copy;
                        else
                            tempMode=Ultracopier::Move;
                        addToTransfer(QFileInfo(args.at(1)),QFileInfo(args.at(2)),tempMode);
                    }
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Wrong line syntax: %1").arg(content));
                    errorFound=true;
                }
            }
        }
        while(data.size()>0);
        transferFile.close();
        if(errorFound)
            emit warningTransferList(tr("Some error have been found during the line parsing"));
        sendActionDone();
        updateTheStatus();
        autoStartIfNeeded();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open the transfer list: %1").arg(transferFile.errorString()));
        emit errorTransferList(tr("Unable to open the transfer list: %1").arg(transferFile.errorString()));
        return;
    }
}

int ListThread::getNumberOfTranferRuning() const
{
    int numberOfTranferRuning=0;
    int loop_size=transferThreadList.size();
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

//do new actions
void ListThread::doNewActions_start_transfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("actionToDoListTransfer.size(): %1, numberOfTranferRuning: %2").arg(actionToDoListTransfer.size()).arg(getNumberOfTranferRuning()));
    if(stopIt || putInPause)
        return;
    int numberOfTranferRuning=getNumberOfTranferRuning();
    loop_size=transferThreadList.size();
    //lunch the transfer in WaitForTheTransfer
    int_for_loop=0;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"numberOfTranferRuning: "+QString::number(numberOfTranferRuning));
}

/** \brief lunch the pre-op or inode op
  1) locate the next next item to do into the both list
    1a) optimisation posible on the mkpath/rmpath
  2) determine what need be lunched
  3) lunch it, rerun the 2)
  */
void ListThread::doNewActions_inode_manipulation()
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("actionToDoList.size(): %1").arg(actionToDoList.size()));
    if(stopIt || putInPause)
        return;
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //lunch the pre-op or inode op
    int_for_loop=0;
    int_for_internal_loop=0;
    int_for_transfer_thread_search=0;
    actionToDoListTransfer_count=actionToDoListTransfer.count();
    actionToDoListInode_count=actionToDoListInode.count();
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    //search the next transfer action to do
    while(int_for_loop<actionToDoListTransfer_count)
    {
        if(!actionToDoListTransfer[int_for_loop].isRunning)
        {
            //search the next inode action to do
            while(int_for_internal_loop<actionToDoListInode_count)
            {
                if(!actionToDoListInode[int_for_internal_loop].isRunning)
                {
                    if(actionToDoListTransfer[int_for_loop].id<actionToDoListInode[int_for_internal_loop].id)
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
                currentTransferThread=transferThreadList[int_for_transfer_thread_search];
                if(currentTransferThread->getStat()==TransferStat_Idle && currentTransferThread->transferId==0) // /!\ important!
                {
                    currentTransferThread->transferId=currentActionToDoTransfer.id;
                    currentTransferThread->transferSize=currentActionToDoTransfer.size;
                    if(!currentTransferThread->setFiles(
                        currentActionToDoTransfer.source,
                        currentActionToDoTransfer.size,
                        currentActionToDoTransfer.destination,
                        currentActionToDoTransfer.mode
                        ))
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("[%1] id: %2 is idle, but seam busy at set name: %3").arg(int_for_loop).arg(currentTransferThread->transferId).arg(currentActionToDoTransfer.destination.absoluteFilePath()));
                        break;
                    }
                    currentActionToDoTransfer.isRunning=true;

                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("[%1] id: %2 is idle, use it for %3").arg(int_for_loop).arg(currentTransferThread->transferId).arg(currentActionToDoTransfer.destination.absoluteFilePath()));

                    /// \note wrong position? Else write why it's here
                    Ultracopier::ReturnActionOnCopyList newAction;
                    newAction.type				= Ultracopier::PreOperation;
                    newAction.addAction.id			= currentActionToDoTransfer.id;
                    newAction.addAction.sourceFullPath	= currentActionToDoTransfer.source.absoluteFilePath();
                    newAction.addAction.sourceFileName	= currentActionToDoTransfer.source.fileName();
                    newAction.addAction.destinationFullPath	= currentActionToDoTransfer.destination.absoluteFilePath();
                    newAction.addAction.destinationFileName	= currentActionToDoTransfer.destination.fileName();
                    newAction.addAction.size		= currentActionToDoTransfer.size;
                    newAction.addAction.mode		= currentActionToDoTransfer.mode;
                    actionDone << newAction;
                    int_for_transfer_thread_search++;
                    break;
                }
                int_for_transfer_thread_search++;
            }
            if(int_for_internal_loop==loop_sub_size_transfer_thread_search)
            {
                /// \note Can be normal when all thread is not initialized
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to found free thread to do the transfer");
                break;
            }
            numberOfInodeOperation++;
            if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
                return;
            if(followTheStrictOrder)
                break;
        }
        int_for_loop++;
    }
    //search the next inode action to do
    while(int_for_internal_loop<actionToDoListInode_count)
    {
        if(!actionToDoListInode[int_for_internal_loop].isRunning)
        {
            //do the inode action
            #include "ListThread_InodeAction.cpp"
        }
        int_for_internal_loop++;
    }
    //error  checking
    if((actionToDoListTransfer_count+actionToDoListInode_count)>ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("The index have been detected as out of max range: %1>%2").arg(actionToDoListTransfer_count+actionToDoListInode_count).arg(ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("transfer thread not located!"));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("TransferStat: %1").arg(stat));
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
    actionDone << newAction;
}

void ListThread::set_osBufferLimit(const unsigned int &osBufferLimit)
{
    this->osBufferLimit=osBufferLimit;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->set_osBufferLimit(osBufferLimit);
        index++;
    }
}

void ListThread::set_setFilters(const QList<Filters_rules> &include,const QList<Filters_rules> &exclude)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("include.size(): %1, exclude.size(): %2").arg(include.size()).arg(exclude.size()));
    this->include=include;
    this->exclude=exclude;
    int index=0;
    while(index<scanFileOrFolderThreadsPool.size())
    {
        scanFileOrFolderThreadsPool.at(index)->setFilters(include,exclude);
        index++;
    }
}

void ListThread::set_sendNewRenamingRules(const QString &firstRenamingRule,const QString &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void ListThread::mkPathFirstFolderFinish()
{
    int_for_loop=0;
    loop_size=actionToDoListInode.size();
    while(int_for_loop<loop_size)
    {
        if(actionToDoListInode.at(int_for_loop).isRunning)
        {
            if(actionToDoListInode.at(int_for_loop).type==ActionType_MkPath)
            {
                //to send to the log
                emit mkPath(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("stop mkpath: %1").arg(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath()));
                actionToDoListInode.removeAt(int_for_loop);
                if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
                    updateTheStatus();
                numberOfInodeOperation--;
                doNewActions_inode_manipulation();
                return;
            }
            if(actionToDoListInode.at(int_for_loop).type==ActionType_MovePath || actionToDoListInode.at(int_for_loop).type==ActionType_RealMove)
            {
                //to send to the log
                emit mkPath(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath());
                emit rmPath(actionToDoListInode.at(int_for_loop).source.absoluteFilePath());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("stop mkpath: %1").arg(actionToDoListInode.at(int_for_loop).destination.absoluteFilePath()));
                actionToDoListInode.removeAt(int_for_loop);
                if(actionToDoListTransfer.size()==0 && actionToDoListInode.size()==0 && actionToDoListInode_afterTheTransfer.size()==0)
                    updateTheStatus();
                numberOfInodeOperation--;
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
    QStringList newList;
    int index=0;
    loop_sub_size_transfer_thread_search=transferThreadList.size();
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
            stat=QString("??? (%1)").arg(transferThreadList.at(index)->getStat());
            break;
        }
        newList << QString("%1) (%3,%4) %2")
            .arg(index)
            .arg(stat)
            .arg(transferThreadList.at(index)->readingLetter())
            .arg(transferThreadList.at(index)->writingLetter());
        index++;
    }
    QStringList newList2;
    index=0;
    loop_size=actionToDoListTransfer.size();
    while(index<loop_size)
    {
        newList2 << QString("%1 %2 %3")
            .arg(actionToDoListTransfer.at(index).source.absoluteFilePath())
            .arg(actionToDoListTransfer.at(index).size)
            .arg(actionToDoListTransfer.at(index).destination.absoluteFilePath());
        if(index>((ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT+ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER)*2+1))
        {
            newList2 << QString("...");
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
void ListThread::errorOnFile(const QFileInfo &fileInfo,const QString &errorString)
{
    emit send_errorOnFile(fileInfo,errorString,qobject_cast<TransferThread *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
void ListThread::folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame)
{
    emit send_folderAlreadyExists(source,destination,isSame,qobject_cast<ScanFileOrFolder *>(sender()));
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void ListThread::errorOnFolder(const QFileInfo &fileInfo,const QString &errorString)
{
    emit send_errorOnFolder(fileInfo,errorString,qobject_cast<ScanFileOrFolder *>(sender()));
}

//to run the thread
void ListThread::run()
{
    clockForTheCopySpeed=new QTimer();

    exec();
}

void ListThread::getNeedPutAtBottom(const QFileInfo &fileInfo, const QString &errorString,TransferThread *thread)
{
    if(actionToDoListTransfer.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"can't try put at bottom if empty");
        this->alwaysDoThisActionForFileExists=FileExists_NotSet;
        putAtBottom=0;
        emit haveNeedPutAtBottom(false,fileInfo,errorString,thread);
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
    emit haveNeedPutAtBottom(needPutAtBottom,fileInfo,errorString,thread);
}

/// \to create transfer thread
void ListThread::createTransferThread()
{
    if(stopIt)
        return;
    transferThreadList << new TransferThread();
    TransferThread * last=transferThreadList.last();
    last->transferId=0;
    last->transferSize=0;
    last->setRightTransfer(doRightTransfer);
    last->setDrive(mountSysPoint,driveType);
    last->setKeepDate(keepDate);
    if(!last->setBlockSize(blockSizeAfterSpeedLimitation))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the block size: "+QString::number(blockSizeAfterSpeedLimitation));
    if(!last->setSequentialBuffer(sequentialBuffer))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the sequentialBuffer: "+QString::number(sequentialBuffer));
    if(!last->setBlockSize(parallelBuffer))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the parallelBuffer: "+QString::number(parallelBuffer));
    last->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
    last->setMultiForBigSpeed(multiForBigSpeed);
    last->set_doChecksum(doChecksum);
    last->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    last->set_checksumOnlyOnError(checksumOnlyOnError);
    last->set_osBuffer(osBuffer);
    last->set_osBufferLimited(osBufferLimited);
    last->set_osBufferLimit(osBufferLimit);
    last->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);

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

    //speed limitation
    connect(clockForTheCopySpeed,	&QTimer::timeout,			last,	&TransferThread::timeOfTheBlockCopyFinished,		Qt::QueuedConnection);

    connect(this,&ListThread::send_sendNewRenamingRules,		last,&TransferThread::setRenamingRules,		Qt::QueuedConnection);
    connect(this,&ListThread::send_setDrive,                    last,&TransferThread::setDrive,		Qt::QueuedConnection);

    connect(this,&ListThread::send_setTransferAlgorithm,		last,&TransferThread::setTransferAlgorithm,		Qt::QueuedConnection);
    connect(this,&ListThread::send_parallelBuffer,              last,&TransferThread::setParallelBuffer,		Qt::QueuedConnection);
    connect(this,&ListThread::send_sequentialBuffer,            last,&TransferThread::setSequentialBuffer,		Qt::QueuedConnection);

    last->start();
    last->setObjectName(QString("transfer %1").arg(transferThreadList.size()-1));
    last->setMkpathTransfer(&mkpathTransfer);
    last->setRenamingRules(firstRenamingRule,otherRenamingRule);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    last->setId(transferThreadList.size()-1);
    #endif
    if(transferThreadList.size()>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
        return;
    if(stopIt)
        return;
    doNewActions_inode_manipulation();
    emit askNewTransferThread();
}

void ListThread::setTransferAlgorithm(TransferAlgorithm transferAlgorithm)
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"wrong number of block: "+QString::number(parallelBuffer));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in number of block: "+QString::number(parallelBuffer));
    this->parallelBuffer=parallelBuffer;
    emit send_parallelBuffer(parallelBuffer);
}

void ListThread::setSequentialBuffer(int sequentialBuffer)
{
    if(sequentialBuffer<1 || sequentialBuffer>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"wrong number of block: "+QString::number(sequentialBuffer));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in number of block: "+QString::number(sequentialBuffer));
    this->sequentialBuffer=sequentialBuffer;
    emit send_sequentialBuffer(sequentialBuffer);
}

void ListThread::setParallelizeIfSmallerThan(const unsigned int &parallelizeIfSmallerThan)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"parallelizeIfSmallerThan in Bytes: "+QString::number(parallelizeIfSmallerThan));
    this->parallelizeIfSmallerThan=parallelizeIfSmallerThan;
}

void ListThread::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    for(int i=0;i<scanFileOrFolderThreadsPool.size();i++)
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
    loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
        index++;
    }
}
