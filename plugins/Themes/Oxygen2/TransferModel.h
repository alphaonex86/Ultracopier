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
#include <memory>

#include "StructEnumDefinition.h"
#include "Oxygen2Environment.h"

#include "../../../interface/FacilityInterface.h"
#include "../../../interface/PathTreeStr.h"
#include "fileTree.h"

/// \brief model to store the transfer list
class TransferModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// \brief the transfer item displayed. source/destination full paths are stored as 4-byte node
    /// ids into pathTreeStr (resolved on demand for the viewport), not as strings -- at 50M files
    /// two full path strings per row is tens of GB of RAM. See PathTreeStr.h. (Independent of the
    /// Folder tree built by appendToTree, which is the tree-VIEW; this is the flat list's storage.)
    struct TransfertItem
    {
        uint64_t id;
        uint32_t srcNode;///< node id into pathTreeStr for the source full path
        uint32_t dstNode;///< node id into pathTreeStr for the destination full path
        std::string size;
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
    /* Stored by pointer so that erasing/moving an entry shifts cheap 8-byte
       handles instead of move-assigning each TransfertItem (3 std::string).
       Same list, same order, same behaviour -- only the element representation
       changes. This is what kept the GUI thread from freezing in -O0/debug
       builds where the per-string moves are not inlined. */
    std::vector<std::unique_ptr<TransfertItem> > transfertItemList;///< To have a transfer list for the user
    PathTreeStr pathTreeStr;///< memory-compact storage for the rows' source/destination full paths (see PathTreeStr.h)
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
