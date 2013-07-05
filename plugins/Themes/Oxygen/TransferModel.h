#ifndef TRANSFERMODEL_H
#define TRANSFERMODEL_H

#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QSet>
#include <QIcon>
#include <QString>

#include "StructEnumDefinition.h"
#include "Environment.h"

#include "../../../interface/FacilityInterface.h"

/// \brief model to store the transfer list
class TransferModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// \brief the transfer item displayed
    struct TransfertItem
    {
        quint64 id;
        QString source;
        QString size;
        QString destination;
        bool selected;
    };
    /// \brief the transfer item with progression
    struct ItemOfCopyListWithMoreInformations
    {
        quint64 currentReadProgression,currentWriteProgression;
        Ultracopier::ItemOfCopyList generalData;
        Ultracopier::ActionTypeCopyList actionType;
        bool custom_with_progression;
    };
    /// \brief returned first transfer item
    struct currentTransfertItem
    {
        quint64 id;
        bool haveItem;
        QString from;
        QString to;
        QString current_file;
        int progressBar_read,progressBar_write;
    };

    TransferModel();

    virtual bool getSelected(const QModelIndex& index);
    virtual void setSelected(const QModelIndex& index,const bool& selected);
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QList<quint64> synchronizeItems(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions);
    void setFacilityEngine(FacilityInterface * facilityEngine);

    int search(const QString &text,bool searchNext);
    int searchPrev(const QString &text);

    void setFileProgression(QList<Ultracopier::ProgressionItem> &progressionList);

    currentTransfertItem getCurrentTransfertItem();

    quint64 firstId();
protected:
    QList<TransfertItem> transfertItemList;///< To have a transfer list for the user
    QSet<quint64> startId,stopId;///< To show what is started, what is stopped
    QHash<quint64,ItemOfCopyListWithMoreInformations> internalRunningOperation;///< to have progression and stat
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
signals:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,QString fonction,QString text,QString file,int ligne);
    #endif
};

#endif // TRANSFERMODEL_H
