/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86 */

#include "CopyEngine.h"
#include "../../../interface/PluginInterface_CopyEngine.h"
#include <QCoreApplication>

CopyEngine::CopyEngine()
{
    timer=new QTimer();
    connect(timer,&QTimer::timeout,this,&CopyEngine::timerSlot);
    timer->start(1000);
    send=false;
}

CopyEngine::~CopyEngine()
{
}

//to send the options panel
bool CopyEngine::getOptionsEngine(QWidget * tempWidget)
{
    (void)tempWidget;
    return true;
}

//to have interface widget to do modal dialog
void CopyEngine::setInterfacePointer(QWidget * uiinterface)
{
    (void)uiinterface;
    syncTransferList();
}

bool CopyEngine::haveSameSource(const std::vector<std::string> &)
{
    return false;
}

bool CopyEngine::haveSameDestination(const std::string &)
{
    return false;
}

bool CopyEngine::newCopy(const std::vector<std::string> &sources)
{
    (void)sources;
    return true;
}

bool CopyEngine::newCopy(const std::vector<std::string> &sources,const std::string &destination)
{
    (void)sources;
    (void)destination;
    return true;
}

bool CopyEngine::newMove(const std::vector<std::string> &sources)
{
    (void)sources;
    return true;
}

bool CopyEngine::newMove(const std::vector<std::string> &sources,const std::string &destination)
{
    (void)sources;
    (void)destination;
    return true;
}

void CopyEngine::newTransferList(const std::string &file)
{
    (void)file;
}

//because direct access to list thread into the main thread can't be do
uint64_t CopyEngine::realByteTransfered()
{
    return rand()%200000000;
}

void CopyEngine::timerSlot()
{
    emit pushGeneralProgression(rand()%200000000,200000000);

    {
        std::vector<std::pair<uint64_t,uint32_t> > progressionList;
        progressionList.push_back(std::pair<uint64_t,uint32_t>(100,         2));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(1000,        2));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(10000,       2+       rand()%2));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(100000,      5+      rand()%3));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(1000000,     15+     rand()%3));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(10000000,    50+    rand()%10));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(100000000,   800+   rand()%100));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(1000000000,  6000+  rand()%500));
        progressionList.push_back(std::pair<uint64_t,uint32_t>(10000000000, 50000+ rand()%1000));
        emit doneTime(progressionList);
    }

    {
        std::vector<Ultracopier::ProgressionItem> progressionList;
        Ultracopier::ProgressionItem entry;
        entry.currentRead=60*1024*1024+rand()%20*1024*1024;
        entry.currentWrite=40*1024*1024+rand()%20*1024*1024;
        entry.id=1;
        entry.total=100*1024*1024;
        progressionList.push_back(entry);
        emit pushFileProgression(progressionList);
    }
}

//speed limitation
bool CopyEngine::supportSpeedLimitation() const
{
    return false;
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void CopyEngine::syncTransferList()
{
    emit syncReady();
    if(send)
        return;
    send=true;

    emit actionInProgess(Ultracopier::Copying);

    std::vector<Ultracopier::ReturnActionOnCopyList> actionsList;
    Ultracopier::ReturnActionOnCopyList newAction;
    newAction.type				= Ultracopier::AddingItem;

    newAction.addAction.id			= 1;
    newAction.addAction.sourceFullPath	= "/folder1/file.iso";
    newAction.addAction.sourceFileName	= "file.iso";
    newAction.addAction.destinationFullPath	= "/dest/folder1/file.iso";
    newAction.addAction.destinationFileName	= "file.iso";
    newAction.addAction.size		= 100*1024*1024;
    newAction.addAction.mode		= Ultracopier::CopyMode::Copy;
    actionsList.push_back(newAction);

    newAction.addAction.id			= 2;
    newAction.addAction.sourceFullPath	= "/file.mp3";
    newAction.addAction.sourceFileName	= "file.mp3";
    newAction.addAction.destinationFullPath	= "/dest/file.mp3";
    newAction.addAction.destinationFileName	= "file.mp3";
    newAction.addAction.size		= 10*1024*1024;
    newAction.addAction.mode		= Ultracopier::CopyMode::Copy;
    actionsList.push_back(newAction);

    newAction.addAction.id			= 3;
    newAction.addAction.sourceFullPath	= "/file.mp4";
    newAction.addAction.sourceFileName	= "file.mp4";
    newAction.addAction.destinationFullPath	= "/dest/file.mp4";
    newAction.addAction.destinationFileName	= "file.mp4";
    newAction.addAction.size		= 50*1024*1024;
    newAction.addAction.mode		= Ultracopier::CopyMode::Copy;
    actionsList.push_back(newAction);

    newAction.addAction.id			= 1;
    newAction.addAction.sourceFullPath	= "/folder1/file.iso";
    newAction.addAction.sourceFileName	= "file.iso";
    newAction.addAction.destinationFullPath	= "/dest/folder1/file.iso";
    newAction.addAction.destinationFileName	= "file.iso";
    newAction.addAction.size		= 100*1024*1024;
    newAction.addAction.mode		= Ultracopier::CopyMode::Copy;
    newAction.type                  = Ultracopier::PreOperation;
    actionsList.push_back(newAction);
    newAction.type                  = Ultracopier::Transfer;
    actionsList.push_back(newAction);

    emit newActionOnList(actionsList);
}

bool CopyEngine::userAddFolder(const Ultracopier::CopyMode &mode)
{
    (void)mode;
    return true;
}

bool CopyEngine::userAddFile(const Ultracopier::CopyMode &mode)
{
    (void)mode;
    return true;
}

void CopyEngine::pause()
{
}

void CopyEngine::resume()
{
}

void CopyEngine::skip(const uint64_t &id)
{
    (void)id;
}

void CopyEngine::cancel()
{
    //QCoreApplication::exit(0);->crash, not need for demo
}

void CopyEngine::removeItems(const std::vector<uint64_t> &ids)
{
    (void)ids;
}

void CopyEngine::moveItemsOnTop(const std::vector<uint64_t> &ids)
{
    (void)ids;
}

void CopyEngine::moveItemsUp(const std::vector<uint64_t> &ids)
{
    (void)ids;
}

void CopyEngine::moveItemsDown(const std::vector<uint64_t> &ids)
{
    (void)ids;
}

void CopyEngine::moveItemsOnBottom(const std::vector<uint64_t> &ids)
{
    (void)ids;
}

/** \brief give the forced mode, to export/import transfer list */
void CopyEngine::forceMode(const Ultracopier::CopyMode &mode)
{
    (void)mode;
}

void CopyEngine::exportTransferList()
{
}

void CopyEngine::importTransferList()
{
}

bool CopyEngine::setSpeedLimitation(const int64_t &speedLimitation)
{
    (void)speedLimitation;
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed: "+std::to_string(speedLimitation));
    return false;
}

void CopyEngine::exportErrorIntoTransferList()
{
}
