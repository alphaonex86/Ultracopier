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
