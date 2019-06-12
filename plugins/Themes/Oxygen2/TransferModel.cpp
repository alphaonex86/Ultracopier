#include "TransferModel.h"
#include "../../../cpp11addition.h"
#include <iostream>

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

    tree=NULL;
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
    if(index.parent()!=QModelIndex() || row < 0 || (unsigned int)row >= transfertItemList.size() || column < 0 || column >= COLUMN_COUNT)
        return QVariant();

    const TransfertItem& item = transfertItemList.at(row);
    if(role==Qt::UserRole)
        return (quint64)item.id;
    else if(role==Qt::DisplayRole)
    {
        switch(column)
        {
            case 0:
                return QString::fromStdString(item.source);
            break;
            case 1:
                return QString::fromStdString(item.size);
            break;
            case 2:
                return QString::fromStdString(item.destination);
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
                if(stopId.find(item.id)!=stopId.cend())
                    return *stop;
                else if(startId.find(item.id)!=startId.cend())
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
        if(!search_text.empty() && (item.source.find(search_text)!=std::string::npos ||
                                    item.destination.find(search_text)!=std::string::npos))
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
    return parent == QModelIndex() ? transfertItemList.size() : 0;
}

uint64_t TransferModel::firstId() const
{
    if(transfertItemList.size()>0)
        return transfertItemList.front().id;
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
            return QString::fromStdString(facilityEngine->translateText("Source"));
            case 1:
            return QString::fromStdString(facilityEngine->translateText("Size"));
            case 2:
            return QString::fromStdString(facilityEngine->translateText("Destination"));
        }
    }

    return QAbstractTableModel::headerData( section, orientation, role );
}

bool TransferModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    row=index.row();
    column=index.column();
    if(index.parent()!=QModelIndex() || row < 0 || (unsigned int)row >= transfertItemList.size() || column < 0 || column >= COLUMN_COUNT)
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
                item.source=value.toString().toStdString();
                emit dataChanged(index,index);
                return true;
            break;
            case 1:
                item.size=value.toString().toStdString();
                emit dataChanged(index,index);
                return true;
            break;
            case 2:
                item.destination=value.toString().toStdString();
                emit dataChanged(index,index);
                return true;
            break;
            default:
            return false;
        }
    }
    return false;
}

Folder * TransferModel::appendToTreeR(Folder * const tree, const std::string &subPath,Folder * const oldTree)
{
    const std::string::size_type n=subPath.find('/');
    std::string name;
    if(n == std::string::npos)
        name=subPath;
    else
        name=subPath.substr(0,n);
    unsigned int search=0;
    while(search<tree->files.size())
    {
        if(tree->files.at(search)->name()==name)
            break;
        search++;
    }
    Folder * folder=nullptr;
    if(search>=tree->files.size())
    {
        if(n+1==subPath.size())
        {
            oldTree->setName(subPath.substr(0,n).c_str());
            folder=oldTree;
        }
        else
            folder=new Folder(name.c_str());
        if(!folder->isFolder())
            return nullptr;
        tree->append(folder);
    }
    else
    {
        File * file=tree->files.at(search);
        if(!file->isFolder())
            return nullptr;
        folder=static_cast<Folder *>(file);
    }
    if(n+1==subPath.size())
        return folder;
    else
        return appendToTreeR(folder,subPath.substr(n+1));
}

void TransferModel::appendToTree(const std::string &path,const uint64_t &size)
{
    if(size==0)
        return;
    const std::string::size_type n=path.rfind('/');
    if(n == std::string::npos)
        return;
    if(treePath.empty())
    {
        treePath=path.substr(0,n+1);
        tree->append(path.c_str()+n+1,size);
    }
    else
    {
        const std::string &newPath=path.substr(0,n+1);
        unsigned int index=0;
        while(index<newPath.size() && index<treePath.size())
        {
            if(treePath.at(index)!=newPath.at(index))
                break;
            index++;
        }
        //append to current path
        if(index==treePath.size())
        {
            //get the next path, found or create
            const std::string &subPath=path.substr(0,n+1);
            Folder * const finalTree=appendToTreeR(tree,subPath);
            finalTree->append(path.c_str()+n+1,size);
        }
        else //new root is to be created
        {
            //save the old values
            const std::string oldTreePath=treePath;
            Folder * const oldTree=tree;
            tree=new Folder("");
            treePath=path.substr(0,index);

            if(oldTreePath.size()>index)
                appendToTreeR(tree,oldTreePath.substr(index),oldTree);

            Folder * const finalTree=appendToTreeR(tree,path.substr(index));
            finalTree->append(path.c_str()+n+1,size);
        }
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    //check the integrity of tree
    checkIntegrity(tree);
    #endif
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//check the integrity of tree
uint64_t TransferModel::checkIntegrity(const Folder * const tree)
{
    uint64_t size=0;
    unsigned int index=0;
    while(index<tree->files.size())
    {
        File * file=tree->files.at(index);
        if(file->isFolder())
            size+=checkIntegrity(static_cast<Folder *>(file));
        else
            size+=file->size();
        index++;
    }
    if(size!=tree->size())
    {
        std::cerr << "tree corrupted" << std::endl;
        abort();
    }
    return tree->size();
}
#endif

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
std::vector<uint64_t> TransferModel::synchronizeItems(const std::vector<Ultracopier::ReturnActionOnCopyList>& returnActions)
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
                transfertItemList.push_back(newItem);
                totalFile++;
                totalSize+=action.addAction.size;

                appendToTree(action.addAction.sourceFullPath,action.addAction.size);
            }
            break;
            case Ultracopier::MoveItem:
            {
                //bool current_entry=
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                if((unsigned int)action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                if(action.userAction.moveAt<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                if((unsigned int)action.userAction.moveAt>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, position is wrong: %2").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                if(action.userAction.position==action.userAction.moveAt)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("id: %1, move at same position: %2").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                const TransfertItem transfertItem=transfertItemList.at(action.userAction.position);
                transfertItemList.erase(transfertItemList.cbegin()+action.userAction.position);
                transfertItemList.insert(transfertItemList.cbegin()+action.userAction.moveAt,transfertItem);
                //newIndexes.move(action.userAction.position,action.userAction.moveAt);
            }
            break;
            case Ultracopier::RemoveItem:
            {
                if(currentIndexSearch>0 && action.userAction.position<=currentIndexSearch)
                    currentIndexSearch--;
                if(action.userAction.position<0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                if((unsigned int)action.userAction.position>(transfertItemList.size()-1))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("id: %1, position is wrong: %3").arg(action.addAction.id).arg(action.userAction.position).toStdString());
                    break;
                }
                transfertItemList.erase(transfertItemList.cbegin()+action.userAction.position);
                currentFile++;
                startId.erase(action.addAction.id);
                stopId.erase(action.addAction.id);
                internalRunningOperation.erase(action.addAction.id);
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
                if(startId.find(action.addAction.id)==startId.cend())
                    startId.insert(action.addAction.id);
                stopId.erase(action.addAction.id);
                if(internalRunningOperation.find(action.addAction.id)!=internalRunningOperation.cend())
                    internalRunningOperation[action.addAction.id].actionType=action.type;
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to found entry for file %1: actionType: Transfer").arg(action.addAction.id).toStdString());
            }
            break;
            case Ultracopier::PostOperation:
            {
                if(stopId.find(action.addAction.id)==stopId.cend())
                    stopId.insert(action.addAction.id);
                startId.erase(action.addAction.id);
            }
            break;
            case Ultracopier::CustomOperation:
            {
                bool custom_with_progression=(action.addAction.size==1);
                //without progression
                if(custom_with_progression)
                {
                    if(startId.find(action.addAction.id)!=startId.cend())
                    {
                        startId.erase(action.addAction.id);
                        if(stopId.find(action.addAction.id)==stopId.cend())
                            stopId.insert(action.addAction.id);
                    }
                }
                //with progression
                else
                {
                    stopId.erase(action.addAction.id);
                    if(startId.find(action.addAction.id)==startId.cend())
                        startId.insert(action.addAction.id);
                }
                if(internalRunningOperation.find(action.addAction.id)!=internalRunningOperation.cend())
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

        for ( unsigned int i = 0; i < transfertItemList.size(); i++ ) {
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
    {
        std::vector<uint64_t> newList;
        newList.resize(3);
        newList[0]=totalFile;
        newList[1]=totalSize;
        newList[2]=currentFile;
        return newList;
    }
}

void TransferModel::setFacilityEngine(FacilityInterface * facilityEngine)
{
    this->facilityEngine=facilityEngine;
}

int TransferModel::search(const std::string &text, bool searchNext)
{
    emit layoutAboutToBeChanged();
    search_text=text;
    emit layoutChanged();
    if(transfertItemList.size()==0)
        return -1;
    if(text.empty())
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
        const TransfertItem &transfertItem=transfertItemList.at(currentIndexSearch);
        if(transfertItem.source.find(search_text)!=std::string::npos ||
                transfertItem.destination.find(search_text)!=std::string::npos)
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

int TransferModel::searchPrev(const std::string &text)
{
    emit layoutAboutToBeChanged();
    search_text=text;
    emit layoutChanged();
    if(transfertItemList.size()==0)
        return -1;
    if(text.empty())
        return -1;
    if(currentIndexSearch==0)
        currentIndexSearch=loop_size-1;
    else
        currentIndexSearch--;
    index_for_loop=0;
    loop_size=transfertItemList.size();
    while(index_for_loop<loop_size)
    {
        const TransfertItem &transfertItem=transfertItemList.at(currentIndexSearch);
        if(transfertItem.source.find(search_text)!=std::string::npos ||
                transfertItem.destination.find(search_text)!=std::string::npos)
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

void TransferModel::setFileProgression(std::vector<Ultracopier::ProgressionItem> &progressionList)
{
    loop_size=progressionList.size();
    index_for_loop=0;
    while(index_for_loop<loop_size)
    {
        if(internalRunningOperation.find(progressionList.at(index_for_loop).id)!=internalRunningOperation.cend())
        {
            internalRunningOperation[progressionList.at(index_for_loop).id].generalData.size=progressionList.at(index_for_loop).total;
            internalRunningOperation[progressionList.at(index_for_loop).id].currentReadProgression=progressionList.at(index_for_loop).currentRead;
            internalRunningOperation[progressionList.at(index_for_loop).id].currentWriteProgression=progressionList.at(index_for_loop).currentWrite;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            progressionList.erase(progressionList.cbegin()+index_for_loop);
            index_for_loop--;
            loop_size--;
            #endif
        }
        index_for_loop++;
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(progressionList.size()>0)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"progression remaning items");
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
        if(internalRunningOperation.find(*startId.cbegin())==internalRunningOperation.cend())
        {
            returnItem.haveItem=false;
            return returnItem;
        }
        const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation.at(*startId.cbegin());
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
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
                returnItem.progressBar_read=65535;
                returnItem.progressBar_write=65535;
            break;
            //should never pass here
            case Ultracopier::PreOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
            break;
            default:
                returnItem.progressBar_read=0;
                returnItem.progressBar_write=0;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
                break;
        }
    }
    else
    {
        returnItem.haveItem=stopId.size()>0;
        if(returnItem.haveItem)
        {
            if(internalRunningOperation.find(*stopId.cbegin())==internalRunningOperation.cend())
            {
                returnItem.haveItem=false;
                return returnItem;
            }
            const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation.at(*stopId.cbegin());
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
                case Ultracopier::PostOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                break;
                //should never pass here
                case Ultracopier::PreOperation:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
                    returnItem.progressBar_read=0;
                    returnItem.progressBar_write=0;
                break;
                default:
                    returnItem.progressBar_read=65535;
                    returnItem.progressBar_write=65535;
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType).toStdString());
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
