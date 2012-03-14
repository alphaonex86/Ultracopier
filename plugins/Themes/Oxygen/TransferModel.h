#ifndef TRANSFERMODEL_H
#define TRANSFERMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QSet>

#include "StructEnumDefinition.h"

#include "../../../interface/FacilityInterface.h"

class TransferModel : public QAbstractTableModel
{
	Q_OBJECT
	struct transfertItem
	{
		quint64 id;
		QString source;
		QString size;
		QString destination;
	};
public:
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	
	QList<quint64> synchronizeItems(const QList<returnActionOnCopyList>& returnActions);
	void setFacilityEngine(FacilityInterface * facilityEngine);
protected:
	QList<transfertItem> transfertItemList;
private:
	int loop_size,index;
	int row,column;
	quint64 totalFile,totalSize,currentFile;
	FacilityInterface * facilityEngine;
};

#endif // TRANSFERMODEL_H
