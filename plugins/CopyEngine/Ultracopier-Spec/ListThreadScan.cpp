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

ScanFileOrFolder * ListThread::newScanThread(Ultracopier::CopyMode mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start with: "+std::to_string(mode));

    //create new thread because is auto-detroyed
    scanFileOrFolderThreadsPool.push_back(new ScanFileOrFolder(mode));
    ScanFileOrFolder * scanFileOrFolderThreads=scanFileOrFolderThreadsPool.back();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::finishedTheListing,				this,&ListThread::scanThreadHaveFinishSlot,	Qt::QueuedConnection))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::fileTransfer,                     this,&ListThread::fileTransfer,             Qt::QueuedConnection))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::fileTransferWithInode,            this,&ListThread::fileTransferWithInode,             Qt::QueuedConnection))
        abort();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::debugInformation,					this,&ListThread::debugInformation,         Qt::QueuedConnection))
        abort();
    #endif
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::newFolderListing,					this,&ListThread::newFolderListing))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::addToMovePath,					this,&ListThread::addToMovePath,			Qt::QueuedConnection))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::addToRealMove,					this,&ListThread::addToRealMove,			Qt::QueuedConnection))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::addToMkPath,                      this,&ListThread::addToMkPath,              Qt::QueuedConnection))
        abort();
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::addToRmForRsync,                  this,&ListThread::addToRmForRsync,          Qt::QueuedConnection))
        abort();
    #endif

    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::errorOnFolder,					this,&ListThread::errorOnFolder,            Qt::QueuedConnection))
        abort();
    if(!connect(scanFileOrFolderThreads,&ScanFileOrFolder::folderAlreadyExists,				this,&ListThread::folderAlreadyExists,		Qt::QueuedConnection))
        abort();

    if(!connect(this,&ListThread::send_updateMount,                 scanFileOrFolderThreads,&ScanFileOrFolder::set_updateMount,          Qt::QueuedConnection))
        abort();

    scanFileOrFolderThreads->setFilters(include,exclude);
    scanFileOrFolderThreads->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
    scanFileOrFolderThreads->setMoveTheWholeFolder(moveTheWholeFolder);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    scanFileOrFolderThreads->setRsync(rsync);
    #endif
    if(scanFileOrFolderThreadsPool.size()==1)
        updateTheStatus();
    scanFileOrFolderThreads->setRenamingRules(firstRenamingRule,otherRenamingRule);
    scanFileOrFolderThreads->setFollowTheStrictOrder(followTheStrictOrder);
    return scanFileOrFolderThreads;
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
