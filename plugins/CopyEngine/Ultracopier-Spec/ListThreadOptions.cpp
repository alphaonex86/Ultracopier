#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"

#if defined(SYNCFILEMANIP)
#include "sync/TransferThreadSync.h"
#else
    #if defined(FSCOPYASYNC)
    #include "async/TransferThreadAsync.h"
    #else
    #error not sync and async set
    #endif
#endif

//set the copy info and options before runing
void ListThread::setRightTransfer(const bool doRightTransfer)
{
    mkPathQueue.setRightTransfer(doRightTransfer);
    this->doRightTransfer=doRightTransfer;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setRightTransfer(doRightTransfer);
        index++;
    }
}

//set keep date
void ListThread::setKeepDate(const bool keepDate)
{
    mkPathQueue.setKeepDate(keepDate);
    this->keepDate=keepDate;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setKeepDate(keepDate);
        index++;
    }
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void ListThread::setRsync(const bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set rsync: "+std::to_string(rsync));
    this->rsync=rsync;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setRsync(rsync);
        index++;
    }
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setRsync(rsync);
}
#endif

//set check destination folder
void ListThread::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setCheckDestinationFolderExists(checkDestinationFolderExists && alwaysDoThisActionForFolderExists!=FolderExists_Merge);
}

void ListThread::setCollisionAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
        index++;
    }
}

//set the folder local collision
void ListThread::setFolderCollision(const FolderExistsAction &alwaysDoThisActionForFolderExists)
{
    this->alwaysDoThisActionForFolderExists=alwaysDoThisActionForFolderExists;
}

//speedLimitation in KB/s
bool ListThread::setSpeedLimitation(const int64_t &speedLimitation)
{
    Q_UNUSED(speedLimitation);
    return false;
}

//set data local to the thread
void ListThread::setAlwaysFileExistsAction(const FileExistsAction &alwaysDoThisActionForFileExists)
{
    this->alwaysDoThisActionForFileExists=alwaysDoThisActionForFileExists;
    unsigned int int_for_loop=0;
    while(int_for_loop<transferThreadList.size())
    {
        transferThreadList.at(int_for_loop)->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
        int_for_loop++;
    }
}

/** \brief give the forced mode, to export/import transfer list */
void ListThread::forceMode(const Ultracopier::CopyMode &mode)
{
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(mode==Ultracopier::Move)
        setRsync(false);
    #endif
    if(mode==Ultracopier::Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to copy");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to move");
    this->mode=mode;
    forcedMode=true;
}

void ListThread::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setMoveTheWholeFolder(moveTheWholeFolder);
    this->moveTheWholeFolder=moveTheWholeFolder;
}

void ListThread::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
        index++;
    }
}

void ListThread::setInodeThreads(const int &inodeThreads)
{
    if(inodeThreads<1 || inodeThreads>32)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"inodeThreads is out of ranges: "+std::to_string(inodeThreads));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"inodeThreads: "+std::to_string(inodeThreads));
    this->inodeThreads=inodeThreads;
    createTransferThread();
    deleteTransferThread();
}

void ListThread::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"renameTheOriginalDestination: "+std::to_string(renameTheOriginalDestination));
    this->renameTheOriginalDestination=renameTheOriginalDestination;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setRenameTheOriginalDestination(renameTheOriginalDestination);
        index++;
    }
}

void ListThread::setCheckDiskSpace(const bool &checkDiskSpace)
{
    this->checkDiskSpace=checkDiskSpace;
}

void ListThread::setBuffer(const bool &buffer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"setBuffer: "+std::to_string(buffer));
    this->buffer=buffer;
    unsigned int index=0;
    while(index<transferThreadList.size())
    {
        transferThreadList.at(index)->setBuffer(buffer);
        index++;
    }
}

void ListThread::setFollowTheStrictOrder(const bool &order)
{
    this->followTheStrictOrder=order;
    for(unsigned int i=0;i<scanFileOrFolderThreadsPool.size();i++)
        scanFileOrFolderThreadsPool.at(i)->setFollowTheStrictOrder(this->followTheStrictOrder);
}

