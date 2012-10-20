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
#include "Environment.h"

#include "../../../interface/FacilityInterface.h"

/// \brief model to store the transfer list
class TransferModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// \brief the transfer item displayed
    struct transfertItem
    {
        quint64 id;
        QString source;
        QString size;
        QString destination;
    };
    /// \brief the transfer item with progression
    struct ItemOfCopyListWithMoreInformations
    {
        quint64 currentProgression;
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
        QString size;
        int progressBar_file;
    };

    TransferModel();

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QList<quint64> synchronizeItems(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions);
    void setFacilityEngine(FacilityInterface * facilityEngine);

    void setFileProgression(QList<Ultracopier::ProgressionItem> &progressionList);

    currentTransfertItem getCurrentTransfertItem();

    quint64 currentFile;
    quint64 totalFile;
    quint64 currentSize;
    quint64 totalSize;
protected:
    QSet<quint64> startId,stopId;///< To show what is started, what is stopped
    QHash<quint64,ItemOfCopyListWithMoreInformations> internalRunningOperation;///< to have progression and stat
    QIcon iconStart,iconPause,iconStop;
private:
    int loop_size,index_for_loop;
    int sub_loop_size,sub_index_for_loop;
    int row,column;
    FacilityInterface * facilityEngine;
    QString search_text;
    /// \brief index from start the search, decresed by remove before it
    int currentIndexSearch;
    bool haveSearchItem;
    quint64 searchId;
signals:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
    #endif
};

#endif // TRANSFERMODEL_H
