#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"

// TransferThreadImpl typedef comes from ListThread.h

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
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->pause();
        index++;
    }
    #ifdef ULTRACOPIER_PLUGIN_KIO
    for(auto &entry : kioJobs)
        entry.job->suspend();
    #endif
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
    int loop_sub_size_transfer_thread_search=transferThreadList.size();
    while(index<loop_sub_size_transfer_thread_search)
    {
        transferThreadList.at(index)->resume();
        index++;
    }
    #ifdef ULTRACOPIER_PLUGIN_KIO
    for(auto &entry : kioJobs)
        entry.job->resume();
    #endif
    emit isInPause(false);
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
            // An interactive skip/remove of a RUNNING transfer is FINAL: no put-to-end retry will
            // ever requeue it. Mark it a pure skip (same as the fileError=Skip policy paths in
            // CopyEngine-collision-and-error.cpp) so WriteThread::stop() completes the close
            // handshake (closed() is emitted) instead of the put-to-end teardown that suppresses
            // it while waiting for a retry that never comes -- without this the write side never
            // "closes", postOperationStopped() never fires, the thread stays counted as running
            // and the whole scheduler wedges (finding-skip-running-hangs).
            transferThreadList.at(index)->finalSkipNoRetry=true;
            transferThreadList.at(index)->skip();
            return true;
        }
        index++;
    }
    // O(1) find by id, O(1) tombstone removal (RemoveItem is id-based in the model)
    const int64_t skipIndex=indexOfActionToDoTransfer(id);
    if(skipIndex>=0)
    {
        const unsigned int int_for_internal_loop=(unsigned int)skipIndex;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("[%1] remove at not running, for id: %2").arg(int_for_internal_loop).arg(id).toStdString());
        Ultracopier::ReturnActionOnCopyList newAction;
        newAction.type=Ultracopier::RemoveItem;
        newAction.userAction.moveAt=1;
        newAction.addAction=actionToDoTransferToItemOfCopyList(*actionToDoListTransfer.at(int_for_internal_loop));
        newAction.userAction.position=int_for_internal_loop;
        actionDone.push_back(newAction);
        tombstoneActionToDoTransferAt(int_for_internal_loop);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("actionToDoListTransfer.size(): %1, actionToDoListInode: %2, actionToDoListInode_afterTheTransfer: %3").arg(actionToDoListTransfer.size()).arg(actionToDoListInode.size()).arg(actionToDoListInode_afterTheTransfer.size()).toStdString());
        if(actionToDoListTransferEmpty() && actionToDoListInode.empty() && actionToDoListInode_afterTheTransfer.empty())
            updateTheStatus();
        return true;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"skip transfer not found: "+std::to_string(id));
    return false;
}

//executed in this thread
void ListThread::cancel()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
        transferThreadList.at(index)->transferId=0;
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
    #ifdef ULTRACOPIER_PLUGIN_KIO
    for(auto &entry : kioJobs)
        entry.job->kill();
    kioJobs.clear();
    #endif
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    if(clockForTheCopySpeed!=NULL)
    {
        clockForTheCopySpeed->stop();
        delete clockForTheCopySpeed;
        clockForTheCopySpeed=NULL;
    }
    #endif
    checkIfReadyToCancel();
}
