// In-process, FS-free unit test of the THEME side of the transfer-list contract:
// TransferModel::synchronizeItems() must apply a RemoveItem by its ID, whatever the
// userAction.position that comes with it.
//
// Why: the engine removes a finished file with an O(1) TOMBSTONE (ListThread::
// tombstoneActionToDoTransferAt) and only compacts actionToDoListTransfer after 512
// tombstones, so the position it ships is a PHYSICAL SLOT of its sparse list -- it keeps
// growing while the model's dense row list shrinks. A theme that bounds-checks that
// position against its own row count silently DROPS the removal of every file past the
// halfway point: the list never empties, the end-of-copy action never runs and the window
// sits at "almost done" forever (reported on 3.1.0.5 as "blocked at 99%").
//
// The same file is compiled against the Oxygen and the Oxygen2 TransferModel (one .pro each).
#include "TransferModel.h"
#include "../../../../../interface/FacilityInterface.h"
#ifdef UC_THEME_HAS_FILETREE
#include "fileTree.h"
#endif
#include <QApplication>
#include <cstdio>
#include <string>
#include <vector>

class StubFacility : public FacilityInterface {
public:
    void retranslate() override {}
    std::string sizeToString(const double &) const override { return std::string(); }
    std::string sizeUnitToString(const Ultracopier::SizeUnit &) const override { return std::string(); }
    std::string translateText(const std::string &t) const override { return t; }
    std::string speedToString(const double &) const override { return std::string(); }
    Ultracopier::TimeDecomposition secondsToTimeDecomposition(const uint32_t &) const override { return Ultracopier::TimeDecomposition(); }
    bool haveFunctionality(const std::string &) const override { return false; }
    std::string callFunctionality(const std::string &, const std::vector<std::string> &) override { return std::string(); }
    std::string simplifiedRemainingTime(const uint32_t &) const override { return std::string(); }
    std::string ultimateUrl() const override { return std::string(); }
    std::string softwareName() const override { return std::string("test"); }
    bool isUltimate() const override { return false; }
    bool isIllegal() const override { return false; }
    void *prepareOpusAudio(const std::string &, QBuffer &) const override { return nullptr; }
};

// Oxygen2 keeps an extra folder TREE next to the flat list; interface.cpp creates its root
// lazily right before every synchronizeItems() call, so a direct model test must do the same.
static void initTree(TransferModel &model)
{
    (void)model;
#ifdef UC_THEME_HAS_FILETREE
    if(model.tree==NULL)
        model.tree=new Folder(std::string());
#endif
}

static int g_checks = 0, g_fail = 0;
static void check(bool cond, const std::string &msg)
{
    ++g_checks;
    if(!cond) { ++g_fail; std::fprintf(stderr, "  FAIL: %s\n", msg.c_str()); }
}

static Ultracopier::ReturnActionOnCopyList addItem(uint64_t id)
{
    Ultracopier::ReturnActionOnCopyList a;
    a.type=Ultracopier::AddingItem;
    a.addAction.id=id;
    a.addAction.sourceFullPath="/src/f"+std::to_string(id)+".bin";
    a.addAction.sourceFileName="f"+std::to_string(id)+".bin";
    a.addAction.destinationFullPath="/dst/f"+std::to_string(id)+".bin";
    a.addAction.destinationFileName="f"+std::to_string(id)+".bin";
    a.addAction.size=1024;
    a.addAction.mode=Ultracopier::Copy;
    a.userAction.position=0;
    a.userAction.moveAt=0;
    return a;
}

static Ultracopier::ReturnActionOnCopyList removeItem(uint64_t id,int position)
{
    Ultracopier::ReturnActionOnCopyList a=addItem(id);
    a.type=Ultracopier::RemoveItem;
    a.userAction.position=position;
    a.userAction.moveAt=0;
    return a;
}

static void fill(TransferModel &model,uint64_t count)
{
    std::vector<Ultracopier::ReturnActionOnCopyList> batch;
    for(uint64_t id=1;id<=count;id++)
        batch.push_back(addItem(id));
    initTree(model);
    model.synchronizeItems(batch);
}

static void sync(TransferModel &model,const std::vector<Ultracopier::ReturnActionOnCopyList> &batch)
{
    initTree(model);
    model.synchronizeItems(batch);
}

// The real end-of-copy stream: one file finishes per UI sync tick, so every RemoveItem
// arrives in a batch of its own, carrying the engine's PHYSICAL slot (id-1 here, because
// nothing is compacted below 512 tombstones).
static void oneFilePerBatch(uint64_t count)
{
    TransferModel model;
    StubFacility facility;
    model.setFacilityEngine(&facility);
    fill(model,count);
    check(model.rowCount()==(int)count,"rowCount after adding "+std::to_string(count)+" items");
    for(uint64_t id=1;id<=count;id++)
    {
        std::vector<Ultracopier::ReturnActionOnCopyList> batch;
        batch.push_back(removeItem(id,(int)(id-1)));//engine ships the physical (tombstoned) slot
        sync(model,batch);
        check(model.rowCount()==(int)(count-id),
              "after removing id "+std::to_string(id)+" of "+std::to_string(count)+
              ": rowCount is "+std::to_string(model.rowCount())+", expected "+std::to_string(count-id));
    }
    check(model.rowCount()==0,"list empty at end of copy ("+std::to_string(count)+" files, one per batch)");
}

// Fast small files: several completions are flushed together. Positions are still physical
// slots, so the last ones already exceed the dense row count inside the same batch.
static void wholeListInOneBatch(uint64_t count)
{
    TransferModel model;
    StubFacility facility;
    model.setFacilityEngine(&facility);
    fill(model,count);
    std::vector<Ultracopier::ReturnActionOnCopyList> batch;
    for(uint64_t id=1;id<=count;id++)
        batch.push_back(removeItem(id,(int)(id-1)));
    sync(model,batch);
    check(model.rowCount()==0,"list empty after a single batch removing all "+std::to_string(count)+" files");
}

// A file skipped/finished out of order still leaves by id, not by row.
static void removeOutOfOrder()
{
    TransferModel model;
    StubFacility facility;
    model.setFacilityEngine(&facility);
    fill(model,3);
    std::vector<Ultracopier::ReturnActionOnCopyList> batch;
    batch.push_back(removeItem(3,2));
    sync(model,batch);
    check(model.rowCount()==2,"rowCount after removing the last row first");
    check(model.firstId()==1,"the FIRST row must survive removing the last one");
    batch.clear();
    batch.push_back(removeItem(1,0));
    sync(model,batch);
    check(model.rowCount()==1,"rowCount after removing the first row");
    check(model.firstId()==2,"the remaining row must be id 2");
}

// The scan is still discovering files while earlier ones complete, so a batch carries BOTH an
// AddingItem and RemoveItems. That mix is what the model handles on its general path, and the
// RemoveItem positions there are still the engine's physical slots -- already past the dense
// row count, because the first files left in an earlier batch.
static void addAndRemoveInSameBatch()
{
    TransferModel model;
    StubFacility facility;
    model.setFacilityEngine(&facility);
    fill(model,4);
    std::vector<Ultracopier::ReturnActionOnCopyList> batch;
    batch.push_back(removeItem(1,0));
    batch.push_back(removeItem(2,1));
    sync(model,batch);//ids 3,4 left
    check(model.rowCount()==2,"rowCount after the first two files completed");
    batch.clear();
    batch.push_back(addItem(5));//the scan found one more file
    batch.push_back(removeItem(3,2));//physical slots: the engine tombstones, it does not compact
    batch.push_back(removeItem(4,3));
    sync(model,batch);
    check(model.rowCount()==1,"only the newly added file must remain, rowCount is "+std::to_string(model.rowCount()));
    check(model.firstId()==5,"the remaining row must be the newly added id 5");
    batch.clear();
    batch.push_back(removeItem(5,4));
    sync(model,batch);
    check(model.rowCount()==0,"list empty after the last file completed");
}

int main(int argc,char *argv[])
{
    QApplication app(argc,argv);
    oneFilePerBatch(2);//the reported case: two big files, one flush each
    oneFilePerBatch(8);
    wholeListInOneBatch(8);
    removeOutOfOrder();
    addAndRemoveInSameBatch();
    std::fprintf(stderr,"translist_sync_test: %d checks, %d failed\n",g_checks,g_fail);
    return g_fail==0 ? 0 : 1;
}
