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
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed in KB/s: "+std::to_string(speedLimitation));

    if(speedLimitation>1024*1024)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"speedLimitation out of range");
        return false;
    }
    this->speedLimitation=speedLimitation;

    multiForBigSpeed=0;
    if(speedLimitation>0)
    {
        blockSizeAfterSpeedLimitation=blockSize;

        //try resolv the interval
        int newInterval;//in ms
        do
        {
            multiForBigSpeed++;
            //at max speed, is out of range for int, it's why quint64 is used
            newInterval=(((quint64)blockSize*(quint64)multiForBigSpeed*1000/* *1000 because interval is into ms, not s*/)/((quint64)speedLimitation*(quint64)1024));
            if(newInterval<0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated newInterval wrong");
                return false;
            }
        }
        while(newInterval<ULTRACOPIER_PLUGIN_MINTIMERINTERVAL);

        if(newInterval<=0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated newInterval wrong");
            return false;
        }
        //wait time too big, then shrink the block size and set interval to max size
        if(newInterval>ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"wait time too big, then shrink the block size and set interval to max size");
            newInterval=ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL;
            multiForBigSpeed=1;
            blockSizeAfterSpeedLimitation=(this->speedLimitation*1024*newInterval)/1000;

            if(blockSizeAfterSpeedLimitation<10)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"calculated block size wrong");
                return false;
            }

            //set the new block size into the thread
            const int &loop_size=transferThreadList.size();
            int int_for_loop=0;
            while(int_for_loop<loop_size)
            {
                if(!transferThreadList.at(int_for_loop)->setBlockSize(blockSizeAfterSpeedLimitation))
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the block size");
                int_for_loop++;
            }
        }

        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("fixed speed with new block size and new interval in BlockSize: %1, multiForBigSpeed: %2, newInterval: %3, maxSpeed: %4")
                                 .arg(blockSizeAfterSpeedLimitation)
                                 .arg(multiForBigSpeed)
                                 .arg(newInterval)
                                 .arg(speedLimitation)
                                 .toStdString()
                                 );

        clockForTheCopySpeed->setInterval(newInterval);
        if(clockForTheCopySpeed!=NULL)
            clockForTheCopySpeed->start();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"clockForTheCopySpeed == NULL at this point");
    }
    else
    {
        if(clockForTheCopySpeed!=NULL)
            clockForTheCopySpeed->stop();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"clockForTheCopySpeed == NULL at this point");
        int int_for_loop=0;
        const int &loop_size=transferThreadList.size();
        while(int_for_loop<loop_size)
        {
            transferThreadList.at(int_for_loop)->setBlockSize(blockSize);
            int_for_loop++;
        }
    }
    int int_for_loop=0;
    const int &loop_size=transferThreadList.size();
    while(int_for_loop<loop_size)
    {
        transferThreadList.at(int_for_loop)->setMultiForBigSpeed(multiForBigSpeed);
        int_for_loop++;
    }

    return true;
    #else
    Q_UNUSED(speedLimitation);
    return false;
    #endif
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

