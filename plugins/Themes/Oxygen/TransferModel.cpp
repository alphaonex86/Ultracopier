#include "TransferModel.h"

#define COLUMN_COUNT 3

// Model

TransferModel::TransferModel()
{
    start=QIcon(":/resources/player_play.png");
    stop=QIcon(":/resources/player_pause.png");
    currentIndexSearch=0;
    haveSearchItem=false;
}

int TransferModel::columnCount( const QModelIndex& parent ) const
{
    return parent == QModelIndex() ? COLUMN_COUNT : 0;
}

QVariant TransferModel::data( const QModelIndex& index, int role ) const
{
    int row,column;
    row=index.row();
    column=index.column();
    if(index.parent()!=QModelIndex() || row < 0 || row >= transfertItemList.count() || column < 0 || column >= COLUMN_COUNT)
        return QVariant();

    const transfertItem& item = transfertItemList[row];
    if(role==Qt::UserRole)
        return item.id;
    else if(role==Qt::DisplayRole)
    {
        switch(column)
        {
            case 0:
                return item.source;
            break;
            case 1:
                return item.size;
            break;
            case 2:
                return item.destination;
            break;
            default:
            return QVariant();
        }
    }
    else if(role==Qt::DecorationRole)
    {
        switch(column)
        {
            case 0:
                if(stopId.contains(item.id))
                    return stop;
                else if(startId.contains(item.id))
                    return start;
                else
                    return QVariant();
            break;
            default:
            return QVariant();
        }
    }
    else if(role==Qt::BackgroundRole)
    {
        if(!search_text.isEmpty() && (item.source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || item.destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1))
        {
            if(haveSearchItem && searchId==item.id)
                return QColor(255,150,150,100);
            else
                return QColor(255,255,0,100);
        }
        else
            return QVariant();
    }
    return QVariant();
}

int TransferModel::rowCount( const QModelIndex& parent ) const
{
    return parent == QModelIndex() ? transfertItemList.count() : 0;
}

quint64 TransferModel::firstId()
{
    if(transfertItemList.count()>0)
        return transfertItemList[0].id;
    else
        return 0;
}

QVariant TransferModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
        switch ( section ) {
            case 0:
            return facilityEngine->translateText("Source");
            case 1:
            return facilityEngine->translateText("Size");
            case 2:
            return facilityEngine->translateText("Destination");
        }
    }

    return QAbstractTableModel::headerData( section, orientation, role );
}

bool TransferModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    row=index.row();
    column=index.column();
    if(index.parent()!=QModelIndex() || row < 0 || row >= transfertItemList.count() || column < 0 || column >= COLUMN_COUNT)
        return false;

    transfertItem& item = transfertItemList[row];
    if(role==Qt::UserRole)
    {
        item.id=value.toULongLong();
        return true;
    }
    else if(role==Qt::DisplayRole)
    {
        switch(column)
        {
            case 0:
                item.source=value.toString();
                emit dataChanged(index,index);
                return true;
            break;
            case 1:
                item.size=value.toString();
                emit dataChanged(index,index);
                return true;
            break;
            case 2:
                item.destination=value.toString();
                emit dataChanged(index,index);
                return true;
            break;
            default:
            return false;
        }
    }
    return false;
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
QList<quint64> TransferModel::synchronizeItems(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions)
{
    loop_size=returnActions.size();
    index_for_loop=0;
    totalFile=0;
    totalSize=0;
    currentFile=0;
    emit layoutAboutToBeChanged();
    while(index_for_loop<loop_size)
    {
        const Ultracopier::ReturnActionOnCopyList& action=returnActions.at(index_for_loop);
        switch(action.type)
        {
            case Ultracopier::AddingItem:
            {
                transfertItem newItem;
                newItem.id=action.addAction.id;
                newItem.source=action.addAction.sourceFullPath;
                newItem.size=facilityEngine->sizeToString(action.addAction.size);
                newItem.destination=action.addAction.destinationFullPath;
                transfertItemList<<newItem;
                totalFile++;
                totalSize+=action.addAction.size;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, size: %2, name: %3").arg(action.addAction.id).arg(action.addAction.size).arg(action.addAction.sourceFullPath));
            }
            break;
            case Ultracopier::MoveItem:
            {
                //bool current_entry=
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.moveAt<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.moveAt>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                transfertItemList.move(action.userAction.position,action.userAction.moveAt);
            }
            break;
            case Ultracopier::RemoveItem:
            {
                if(currentIndexSearch>0 && action.userAction.position<=currentIndexSearch)
                    currentIndexSearch--;
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                transfertItemList.removeAt(action.userAction.position);
                currentFile++;
                startId.remove(action.addAction.id);
                stopId.remove(action.addAction.id);
                internalRunningOperation.remove(action.addAction.id);
            }
            break;
            case Ultracopier::PreOperation:
            {
                ItemOfCopyListWithMoreInformations tempItem;
                tempItem.currentReadProgression=0;
                tempItem.currentWriteProgression=0;
                tempItem.generalData=action.addAction;
                tempItem.actionType=action.type;
                internalRunningOperation[action.addAction.id]=tempItem;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("set for file %1: actionType: PreOperation").arg(action.addAction.id));
            }
            break;
            case Ultracopier::Transfer:
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("found entry for file %1: actionType: Transfer").arg(action.addAction.id));
                if(!startId.contains(action.addAction.id))
                    startId << action.addAction.id;
                stopId.remove(action.addAction.id);
                if(internalRunningOperation.contains(action.addAction.id))
                    internalRunningOperation[action.addAction.id].actionType=action.type;
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unable to found entry for file %1: actionType: Transfer").arg(action.addAction.id));
            }
            break;
            case Ultracopier::PostOperation:
            {
                if(!stopId.contains(action.addAction.id))
                    stopId << action.addAction.id;
                startId.remove(action.addAction.id);
            }
            break;
            case Ultracopier::CustomOperation:
            {
                bool custom_with_progression=(action.addAction.size==1);
                //without progression
                if(custom_with_progression)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("switch the file: %1 to custom operation with progression").arg(action.addAction.id));
                    if(startId.remove(action.addAction.id))
                        if(!stopId.contains(action.addAction.id))
                            stopId << action.addAction.id;
                }
                //with progression
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("switch the file: %1 to custom operation without progression").arg(action.addAction.id));
                    stopId.remove(action.addAction.id);
                    if(!startId.contains(action.addAction.id))
                        startId << action.addAction.id;
                }
                if(internalRunningOperation.contains(action.addAction.id))
                {
                    ItemOfCopyListWithMoreInformations &item=internalRunningOperation[action.addAction.id];
                    item.actionType=action.type;
                    item.custom_with_progression=custom_with_progression;
                    item.currentReadProgression=0;
                    item.currentWriteProgression=0;
                }
            }
            break;
            default:
                //unknow code, ignore it
            break;
        }
        index_for_loop++;
    }
    emit layoutChanged();
    return QList<quint64>() << totalFile << totalSize << currentFile;
}

void TransferModel::setFacilityEngine(FacilityInterface * facilityEngine)
{
    this->facilityEngine=facilityEngine;
}

int TransferModel::search(const QString &text,bool searchNext)
{
    emit layoutAboutToBeChanged();
    search_text=text;
    emit layoutChanged();
    if(transfertItemList.size()==0)
        return -1;
    if(text.isEmpty())
        return -1;
    if(searchNext)
    {
        currentIndexSearch++;
        if(currentIndexSearch>=loop_size)
            currentIndexSearch=0;
    }
    index_for_loop=0;
    loop_size=transfertItemList.size();
    while(index_for_loop<loop_size)
    {
        if(transfertItemList.at(currentIndexSearch).source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || transfertItemList.at(currentIndexSearch).destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1)
        {
            haveSearchItem=true;
            searchId=transfertItemList.at(currentIndexSearch).id;
            return currentIndexSearch;
        }
        currentIndexSearch++;
        if(currentIndexSearch>=loop_size)
            currentIndexSearch=0;
        index_for_loop++;
    }
    haveSearchItem=false;
    return -1;
}

int TransferModel::searchPrev(const QString &text)
{
    emit layoutAboutToBeChanged();
    search_text=text;
    emit layoutChanged();
    if(transfertItemList.size()==0)
        return -1;
    if(text.isEmpty())
        return -1;
    if(currentIndexSearch==0)
        currentIndexSearch=loop_size-1;
    else
        currentIndexSearch--;
    index_for_loop=0;
    loop_size=transfertItemList.size();
    while(index_for_loop<loop_size)
    {
        if(transfertItemList.at(currentIndexSearch).source.indexOf(search_text,0,Qt::CaseInsensitive)!=-1 || transfertItemList.at(currentIndexSearch).destination.indexOf(search_text,0,Qt::CaseInsensitive)!=-1)
        {
            haveSearchItem=true;
            searchId=transfertItemList.at(currentIndexSearch).id;
            return currentIndexSearch;
        }
        if(currentIndexSearch==0)
            currentIndexSearch=loop_size-1;
        else
            currentIndexSearch--;
        index_for_loop++;
    }
    haveSearchItem=false;
    return -1;
}

void TransferModel::setFileProgression(QList<Ultracopier::ProgressionItem> &progressionList)
{
    loop_size=progressionList.size();
    index_for_loop=0;
    while(index_for_loop<loop_size)
    {
        if(internalRunningOperation.contains(progressionList.at(index_for_loop).id))
        {
            internalRunningOperation[progressionList.at(index_for_loop).id].generalData.size=progressionList.at(index_for_loop).total;
            internalRunningOperation[progressionList.at(index_for_loop).id].currentReadProgression=progressionList.at(index_for_loop).currentRead;
            internalRunningOperation[progressionList.at(index_for_loop).id].currentWriteProgression=progressionList.at(index_for_loop).currentWrite;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            progressionList.removeAt(index_for_loop);
            index_for_loop--;
            loop_size--;
            #endif
        }
        index_for_loop++;
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(progressionList.size()>0)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"progression remaning items");
    #endif
}

TransferModel::currentTransfertItem TransferModel::getCurrentTransfertItem()
{
    currentTransfertItem returnItem;
    returnItem.haveItem=startId.size()>0;
    if(returnItem.haveItem)
    {
        if(!internalRunningOperation.contains(*startId.constBegin()))
        {
            returnItem.haveItem=false;
            return returnItem;
        }
        const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation[*startId.constBegin()];
        returnItem.from=itemTransfer.generalData.sourceFullPath;
        returnItem.to=itemTransfer.generalData.destinationFullPath;
        returnItem.current_file=itemTransfer.generalData.destinationFileName+", "+facilityEngine->sizeToString(itemTransfer.generalData.size);
        returnItem.id=itemTransfer.generalData.id;
        switch(itemTransfer.actionType)
        {
            case Ultracopier::CustomOperation:
            if(!itemTransfer.custom_with_progression)
                returnItem.progressBar_read=-1;
            else
            {
                if(itemTransfer.generalData.size>0)
                {
                    returnItem.progressBar_read=((double)itemTransfer.currentReadProgression/itemTransfer.generalData.size)*65535;
                    returnItem.progressBar_write=((double)itemTransfer.currentWriteProgression/itemTransfer.generalData.size)*65535;
                }
                else
                    returnItem.progressBar_read=-1;
            }
            break;
            case Ultracopier::Transfer:
            if(itemTransfer.generalData.size>0)
            {
                returnItem.progressBar_read=((double)itemTransfer.currentReadProgression/itemTransfer.generalData.size)*65535;
                returnItem.progressBar_write=((double)itemTransfer.currentWriteProgression/itemTransfer.generalData.size)*65535;
            }
            else
            {
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
            }
            break;
            //should never pass here
            case Ultracopier::PostOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_read=65535;
                returnItem.progressBar_write=65535;
            break;
            //should never pass here
            case Ultracopier::PreOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
            break;
            default:
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                break;
        }
    }
    else
    {
        returnItem.haveItem=stopId.size()>0;
        if(returnItem.haveItem)
        {
            if(!internalRunningOperation.contains(*stopId.constBegin()))
            {
                returnItem.haveItem=false;
                return returnItem;
            }
            const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation[*stopId.constBegin()];
            returnItem.from=itemTransfer.generalData.sourceFullPath;
            returnItem.to=itemTransfer.generalData.destinationFullPath;
            returnItem.current_file=itemTransfer.generalData.destinationFileName+", "+facilityEngine->sizeToString(itemTransfer.generalData.size);
            returnItem.id=itemTransfer.generalData.id;
            switch(itemTransfer.actionType)
            {
                case Ultracopier::CustomOperation:
                if(!itemTransfer.custom_with_progression)
                    returnItem.progressBar_read=-1;
                else
                {
                    if(itemTransfer.generalData.size>0)
                    {
                        returnItem.progressBar_read=((double)itemTransfer.currentReadProgression/itemTransfer.generalData.size)*65535;
                        returnItem.progressBar_write=((double)itemTransfer.currentWriteProgression/itemTransfer.generalData.size)*65535;
                    }
                    else
                        returnItem.progressBar_read=-1;
                }
                break;
                case Ultracopier::Transfer:
                if(itemTransfer.generalData.size>0)
                {
                    returnItem.progressBar_read=((double)itemTransfer.currentReadProgression/itemTransfer.generalData.size)*65535;
                    returnItem.progressBar_write=((double)itemTransfer.currentWriteProgression/itemTransfer.generalData.size)*65535;
                }
                else
                    returnItem.progressBar_read=0;
                    returnItem.progressBar_write=0;
                break;
                case Ultracopier::PostOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                break;
                //should never pass here
                case Ultracopier::PreOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    returnItem.progressBar_read=0;
                    returnItem.progressBar_write=0;
                break;
                default:
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    break;
            }
        }
    }
    if(returnItem.progressBar_read!=-1 && returnItem.progressBar_write>returnItem.progressBar_read)
    {
        int tempVar=returnItem.progressBar_write;
        returnItem.progressBar_write=returnItem.progressBar_read;
        returnItem.progressBar_read=tempVar;
    }
    return returnItem;
}
