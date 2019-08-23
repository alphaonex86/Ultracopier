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
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>

#include "StructEnumDefinition.h"
#include "Oxygen2Environment.h"

#include "../../../interface/FacilityInterface.h"
#include "fileTree.h"

/// \brief model to store the transfer list
class TransferModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// \brief the transfer item displayed
    struct TransfertItem
    {
        uint64_t id;
        std::string source;
        std::string size;
        std::string destination;
    };
    /// \brief the transfer item with progression
    struct ItemOfCopyListWithMoreInformations
    {
        uint64_t currentReadProgression,currentWriteProgression;
        Ultracopier::ItemOfCopyList generalData;
        Ultracopier::ActionTypeCopyList actionType;
        bool custom_with_progression;
    };
    /// \brief returned first transfer item
    struct currentTransfertItem
    {
        uint64_t id;
        bool haveItem;
        std::string from;
        std::string to;
        std::string current_file;
        int progressBar_read,progressBar_write;
    };

    TransferModel();

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    std::vector<uint64_t> synchronizeItems(const std::vector<Ultracopier::ReturnActionOnCopyList>& returnActions);
    void appendToTree(const std::string &path, const uint64_t &size);
    Folder * appendToTreeR(Folder * const tree, const std::string &subPath,Folder * const oldTree=NULL);
    void setFacilityEngine(FacilityInterface * facilityEngine);

    int search(const std::string &text,bool searchNext);
    int searchPrev(const std::string &text);

    void setFileProgression(std::vector<Ultracopier::ProgressionItem> &progressionList);

    currentTransfertItem getCurrentTransfertItem() const;

    uint64_t firstId() const;
    Folder * tree;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    //check the integrity of tree
    uint64_t checkIntegrityChildren(const Folder * const tree);
    uint64_t checkIntegritySize(const Folder * const tree);
    #endif
protected:
    std::vector<TransfertItem> transfertItemList;///< To have a transfer list for the user
    std::set<uint64_t> startId,stopId;///< To show what is started, what is stopped
    std::unordered_map<uint64_t,ItemOfCopyListWithMoreInformations> internalRunningOperation;///< to have progression and stat
private:
    int loop_size,index_for_loop;
    int sub_loop_size,sub_index_for_loop;
    int row,column;
    FacilityInterface * facilityEngine;
    std::string search_text;
    /// \brief index from start the search, decresed by remove before it
    int currentIndexSearch;
    bool haveSearchItem;
    uint64_t searchId;
    static QIcon *start;
    static QIcon *stop;
    std::string treePath;
signals:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;
    #endif
};

#endif // TRANSFERMODEL_H
