#ifndef TRANSFERMODEL_H
#define TRANSFERMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QSet>
#include <QIcon>
#include <QString>

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
	TransferModel();

	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	
	QList<quint64> synchronizeItems(const QList<returnActionOnCopyList>& returnActions);
	void setFacilityEngine(FacilityInterface * facilityEngine);

	/// \brief new transfer have started
	void newTransferStart(const quint64 &id);
	/** \brief one transfer have been stopped
	 * is stopped, example: because error have occurred, and try later, don't remove the item! */
	void newTransferStop(const quint64 &id);

	int search(const QString &text,bool searchNext);
	int searchPrev(const QString &text);
protected:
	QList<transfertItem> transfertItemList;
	QList<quint64> startId,stopId;
	QIcon start,stop;
private:
	int loop_size,index_for_loop;
	int row,column;
	quint64 totalFile,totalSize,currentFile;
	FacilityInterface * facilityEngine;
	QString search_text;
	/// \brief index from start the search, decresed by remove before it
	int currentIndexSearch;
	bool haveSearchItem;
	quint64 searchId;
};

#endif // TRANSFERMODEL_H
