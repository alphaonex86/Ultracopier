#include "TransferModel.h"

#include <QStringList>
#include <QDebug>

#define COLUMN_COUNT 3
#define CHECK_DUPLICATE false

// Model

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
	else
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
	return QVariant();
}

int TransferModel::rowCount( const QModelIndex& parent ) const
{
	return parent == QModelIndex() ? transfertItemList.count() : 0;
}

QVariant TransferModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
		switch ( section ) {
			case 0:
			return tr( "Source" );
			case 1:
			return tr( "Size" );
			case 2:
			return tr( "Target" );
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
	else
	{
		switch(column)
		{
			case 0:
				item.source=value.toString();
				return true;
			break;
			case 1:
				item.size=value.toString();
				return true;
			break;
			case 2:
				item.destination=value.toString();
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
QList<quint64> TransferModel::synchronizeItems(const QList<returnActionOnCopyList>& returnActions)
{
	loop_size=returnActions.size();
	index=0;
	totalFile=0;
	totalSize=0;
	currentFile=0;
	emit layoutAboutToBeChanged();
	while(index<loop_size)
	{
		const returnActionOnCopyList& action=returnActions.at(index);
		if(action.type==AddingItem)
		{
			transfertItem newItem;
			newItem.id=action.addAction.id;
			newItem.source=action.addAction.sourceFullPath;
			newItem.size=facilityEngine->sizeToString(action.addAction.size);
			newItem.destination=action.addAction.destinationFullPath;
			transfertItemList<<newItem;
			totalFile++;
			totalSize+=action.addAction.size;
		}
		else if(action.userAction.type==MoveItem)
			transfertItemList.move(action.userAction.current_position,action.userAction.position);
		else if(action.userAction.type==RemoveItem)
		{
			transfertItemList.removeAt(action.userAction.current_position);
			currentFile++;
		}
		index++;
	}
	emit layoutChanged();
	return QList<quint64>() << totalFile << totalSize << currentFile;
}

void TransferModel::setFacilityEngine(FacilityInterface * facilityEngine)
{
	this->facilityEngine=facilityEngine;
}

