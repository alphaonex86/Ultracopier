#include "TransferModel.h"

#define COLUMN_COUNT 3

// Model

QIcon *TransferModel::start=NULL;
QIcon *TransferModel::stop=NULL;

TransferModel::TransferModel()
{
    /// \warning to prevent Must construct a QGuiApplication before QPixmap IN STATIC WINDOWS VERSION ONLY
    if(TransferModel::start==NULL)
        TransferModel::start=new QIcon(QStringLiteral(":/resources/player_play.png"));
    if(TransferModel::stop==NULL)
        TransferModel::stop=new QIcon(QStringLiteral(":/resources/player_pause.png"));
    currentIndexSearch=0;
    haveSearchItem=false;
    loop_size=0,index_for_loop=0;
    sub_loop_size=0,sub_index_for_loop=0;
    row=0,column=0;
    facilityEngine=NULL;
    currentIndexSearch=0;
    haveSearchItem=false;
    searchId=0;
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

    const TransfertItem& item = transfertItemList.at(row);
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
                    return *stop;
                else if(startId.contains(item.id))
                    return *start;
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

quint64 TransferModel::firstId() const
{
    if(transfertItemList.count()>0)
        return transfertItemList.first().id;
    else
        return 0;
}

QVariant TransferModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if(facilityEngine==NULL)
        abort();
    if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
        switch ( section ) {
            case 0:
            return facilityEngine->translateText(QStringLiteral("Source"));
            case 1:
            return facilityEngine->translateText(QStringLiteral("Size"));
            case 2:
            return facilityEngine->translateText(QStringLiteral("Destination"));
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

    TransfertItem& item = transfertItemList[row];
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
    const QModelIndexList oldIndexes = persistentIndexList();
    QModelIndexList newIndexes=oldIndexes;
    QMap<int, quint64> oldMapping;  // model index row in model before update, item id
    QMap<quint64, int> newMapping;  // item id, model index row in model after update
    for ( int i = 0; i < oldIndexes.count(); i++ ) {
        const QModelIndex& index = oldIndexes.at(i);
        oldMapping[index.row()] = index.data( Qt::UserRole ).value<quint64>();
    }

    loop_size=returnActions.size();
    index_for_loop=0;
    quint64 totalFile=0,totalSize=0,currentFile=0;
    emit layoutAboutToBeChanged();
    while(index_for_loop<loop_size)
    {
        const Ultracopier::ReturnActionOnCopyList& action=returnActions.at(index_for_loop);
        switch(action.type)
        {
            case Ultracopier::AddingItem:
            {
                TransfertItem newItem;
                newItem.id=action.addAction.id;
                newItem.source=action.addAction.sourceFullPath;
                newItem.size=facilityEngine->sizeToString(action.addAction.size);
                newItem.destination=action.addAction.destinationFullPath;
                transfertItemList<<newItem;
                totalFile++;
                totalSize+=action.addAction.size;
            }
            break;
            case Ultracopier::MoveItem:
            {
                //bool current_entry=
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.moveAt<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.moveAt>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.position==action.userAction.moveAt)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, move at same position: %2").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                transfertItemList.move(action.userAction.position,action.userAction.moveAt);
                //newIndexes.move(action.userAction.position,action.userAction.moveAt);
            }
            break;
            case Ultracopier::RemoveItem:
            {
                if(currentIndexSearch>0 && action.userAction.position<=currentIndexSearch)
                    currentIndexSearch--;
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                if(action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position));
                    break;
                }
                transfertItemList.removeAt(action.userAction.position);
                currentFile++;
                startId.remove(action.addAction.id);
                stopId.remove(action.addAction.id);
                internalRunningOperation.remove(action.addAction.id);
                //newIndexes.remove(action.userAction.moveAt);
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
            }
            break;
            case Ultracopier::Transfer:
            {
                if(!startId.contains(action.addAction.id))
                    startId << action.addAction.id;
                stopId.remove(action.addAction.id);
                if(internalRunningOperation.contains(action.addAction.id))
                    internalRunningOperation[action.addAction.id].actionType=action.type;
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to found entry for file %1: actionType: Transfer").arg(action.addAction.id));
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
                    if(startId.remove(action.addAction.id))
                        if(!stopId.contains(action.addAction.id))
                            stopId << action.addAction.id;
                }
                //with progression
                else
                {
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

    if(!oldIndexes.isEmpty())
    {
        const QSet<quint64> ids = oldMapping.values().toSet();

        for ( int i = 0; i < transfertItemList.count(); i++ ) {
            const TransferModel::TransfertItem& item = transfertItemList.at(i);

            if ( ids.contains( item.id ) ) {
                newMapping[ item.id ] = i;
            }
        }

        for ( int i = 0; i < oldIndexes.count(); i++ ) {
            const QModelIndex& index = oldIndexes.at(i);
            const int newRow = newMapping.value( oldMapping.value(index.row()), -1 );
            newIndexes[ i ] = newRow == -1 ? QModelIndex() : QAbstractTableModel::index( newRow, index.column(), index.parent() );
        }
    }

    changePersistentIndexList( oldIndexes, newIndexes );
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("progression remaning items"));
    #endif
}

TransferModel::currentTransfertItem TransferModel::getCurrentTransfertItem() const
{
    currentTransfertItem returnItem;
    returnItem.progressBar_read=-1;
    returnItem.progressBar_write=0;
    returnItem.haveItem=startId.size()>0;
    if(returnItem.haveItem)
    {
        if(!internalRunningOperation.contains(*startId.constBegin()))
        {
            returnItem.haveItem=false;
            return returnItem;
        }
        const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation.value(*startId.constBegin());
        returnItem.from=itemTransfer.generalData.sourceFullPath;
        returnItem.to=itemTransfer.generalData.destinationFullPath;
        returnItem.current_file=itemTransfer.generalData.destinationFileName+QStringLiteral(", ")+facilityEngine->sizeToString(itemTransfer.generalData.size);
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
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_read=65535;
                returnItem.progressBar_write=65535;
            break;
            //should never pass here
            case Ultracopier::PreOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
            break;
            default:
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
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
            const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation.value(*stopId.constBegin());
            returnItem.from=itemTransfer.generalData.sourceFullPath;
            returnItem.to=itemTransfer.generalData.destinationFullPath;
            returnItem.current_file=itemTransfer.generalData.destinationFileName+QStringLiteral(", ")+facilityEngine->sizeToString(itemTransfer.generalData.size);
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
                case Ultracopier::PostOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                break;
                //should never pass here
                case Ultracopier::PreOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    returnItem.progressBar_read=0;
                    returnItem.progressBar_write=0;
                break;
                default:
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                    break;
            }
        }
    }
    if(returnItem.haveItem && returnItem.progressBar_read!=-1 && returnItem.progressBar_write>returnItem.progressBar_read)
    {
        int tempVar=returnItem.progressBar_write;
        returnItem.progressBar_write=returnItem.progressBar_read;
        returnItem.progressBar_read=tempVar;
    }
    return returnItem;
}
