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
                        transferFile.write((TransferThread::internalStringTostring(actionToDoListTransfer.at(index).source)+
                                            ";"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).destination)+"\n").c_str());
                    else
                        transferFile.write(("Copy;"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).source)+
                                            ";"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).destination)+"\n").c_str());
                }
                else
                    haveError=true;
            }
            else if(actionToDoListTransfer.at(index).mode==Ultracopier::Move)
            {
                if(!forcedMode || mode==Ultracopier::Move)
                {
                    if(forcedMode)
                        transferFile.write((TransferThread::internalStringTostring(actionToDoListTransfer.at(index).source)+
                                            ";"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).destination)+"\n").c_str());
                    else
                        transferFile.write(("Move;"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).source)+
                                            ";"+TransferThread::internalStringTostring(actionToDoListTransfer.at(index).destination)+"\n").c_str());
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
                        addToTransfer(TransferThread::stringToInternalString(args.at(0)),
                                      TransferThread::stringToInternalString(args.at(1)),mode);
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
                        addToTransfer(TransferThread::stringToInternalString(args.at(1)),TransferThread::stringToInternalString(args.at(2)),tempMode);
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
            if(forcedMode)
                transferFile.write((errorLog.at(index).source+";"+errorLog.at(index).destination+"\n").c_str());
            else
            {
                if(errorLog.at(index).mode==Ultracopier::Copy)
                    transferFile.write(("Copy;"+errorLog.at(index).source+";"+errorLog.at(index).destination+"\n").c_str());
                else if(errorLog.at(index).mode==Ultracopier::Move)
                    transferFile.write(("Move;"+errorLog.at(index).source+";"+errorLog.at(index).destination+"\n").c_str());
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
