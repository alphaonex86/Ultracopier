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
public:
	struct transfertItem
	{
		quint64 id;
		QString source;
		QString size;
		QString destination;
	};
	struct ItemOfCopyListWithMoreInformations
	{
		quint64 currentProgression;
		ItemOfCopyList generalData;
		ActionTypeCopyList actionType;
		bool custom_with_progression;
	};
	struct currentTransfertItem
	{
		quint64 id;
		bool haveItem;
		QString from;
		QString to;
		QString current_file;
		int progressBar_file;
	};

	TransferModel();

	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	
	QList<quint64> synchronizeItems(const QList<returnActionOnCopyList>& returnActions);
	void setFacilityEngine(FacilityInterface * facilityEngine);

	int search(const QString &text,bool searchNext);
	int searchPrev(const QString &text);

	void setFileProgression(const QList<ProgressionItem> &progressionList);

	currentTransfertItem getCurrentTransfertItem();
protected:
	QList<transfertItem> transfertItemList;
	QList<quint64> startId,stopId;
	QList<ItemOfCopyListWithMoreInformations> InternalRunningOperation;
	QIcon start,stop;
private:
	int loop_size,index_for_loop;
	int sub_loop_size,sub_index_for_loop;
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
