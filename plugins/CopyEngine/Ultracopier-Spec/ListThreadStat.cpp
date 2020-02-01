#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"

#include "async/TransferThreadAsync.h"

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
        default:
            return;
        break;
    }
    newAction.addAction.id			= id;
    actionDone.push_back(newAction);
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
        newList2.push_back(
            TransferThread::internalStringTostring(actionToDoListTransfer.at(index).source)+
            " "+std::to_string(actionToDoListTransfer.at(index).size)+" "+
            TransferThread::internalStringTostring(actionToDoListTransfer.at(index).destination)
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
        TransferThreadAsync * temp_transfer_thread=transferThreadList.at(int_for_loop);
        switch(temp_transfer_thread->getStat())
        {
            case TransferStat_Transfer:
            case TransferStat_PostTransfer:
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
                Ultracopier::ProgressionItem tempItem;
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
