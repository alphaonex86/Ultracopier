#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"
#include "async/TransferThreadAsync.h"

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::haveSameSource(const std::vector<std::string> &sources)
{
    if(stopIt)
        return false;
    if(sourceDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDriveMultiple");
        return false;
    }
    if(sourceDrive.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourceDrive.isEmpty()");
        return true;
    }
    unsigned int index=0;
    while(index<sources.size())
    {
        if(driveManagement.getDrive(sources.at(index))!=sourceDrive)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources.at(index))!=sourceDrive");
            return false;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"seam have same source");
    return true;
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::haveSameDestination(const std::string &destination)
{
    if(stopIt)
        return false;
    if(destinationDriveMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDriveMultiple");
        return false;
    }
    if(destinationDrive.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDrive.isEmpty()");
        return true;
    }
    if(driveManagement.getDrive(destination)!=destinationDrive)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination!=destinationDrive");
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"seam have same destination");
    return true;
}

/// \return empty if multiple or no destination
std::string ListThread::getUniqueDestinationFolder() const
{
    if(stopIt)
        return std::string();
    if(destinationFolderMultiple)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destinationDriveMultiple");
        return std::string();
    }
    return TransferThread::internalStringTostring(destinationFolder);
}

void ListThread::detectDrivesOfCurrentTransfer(const std::vector<INTERNALTYPEPATH> &sources,const INTERNALTYPEPATH &destination)
{
    /* code to detect volume/mount point to group by windows */
    if(!sourceDriveMultiple)
    {
        unsigned int index=0;
        while(index<sources.size())
        {
            const std::string &tempDrive=driveManagement.getDrive(TransferThread::internalStringTostring(sources.at(index)));
            //if have not already source, set the source
            if(sourceDrive.empty())
                sourceDrive=tempDrive;
            //if have previous source and the news source is not the same
            if(sourceDrive!=tempDrive)
            {
                sourceDriveMultiple=true;
                break;
            }
            index++;
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source informations, sourceDrive: %1, sourceDriveMultiple: %2").arg(QString::fromStdString(sourceDrive)).arg(sourceDriveMultiple).toStdString());
    if(!destinationDriveMultiple)
    {
        const std::string &tempDrive=driveManagement.getDrive(TransferThread::internalStringTostring(destination));
        //if have not already destination, set the destination
        if(destinationDrive.empty())
            destinationDrive=tempDrive;
        //if have previous destination and the news destination is not the same
        if(destinationDrive!=tempDrive)
            destinationDriveMultiple=true;
    }
    if(!destinationFolderMultiple)
    {
        //if have not already destination, set the destination
        if(destinationFolder.empty())
            destinationFolder=destination;
        //if have previous destination and the news destination is not the same
        if(destinationFolder!=destination)
            destinationFolderMultiple=true;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("destination informations, destinationDrive: %1, destinationDriveMultiple: %2").arg(QString::fromStdString(destinationDrive)).arg(destinationDriveMultiple).toStdString());
}

//return
bool ListThread::needMoreSpace() const
{
    if(!checkDiskSpace)
        return false;
    std::vector<Diskspace> diskspace_list;
    for( auto& spaceDrive : requiredSpace ) {
        const QString &drive=QString::fromStdString(spaceDrive.first);
        #ifdef Q_OS_WIN32
        if(spaceDrive.first!="A:\\" && spaceDrive.first!="A:/" && spaceDrive.first!="A:" && spaceDrive.first!="A" && spaceDrive.first!="a:\\" && spaceDrive.first!="a:/" && spaceDrive.first!="a:" && spaceDrive.first!="a")
        {
        #endif
            QStorageInfo storageInfo(drive);
            storageInfo.refresh();
            const qint64 &availableSpace=storageInfo.bytesAvailable();
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            const qint64 &bytesFree=storageInfo.bytesFree();
            #endif

            if(availableSpace<0 ||
                    //workaround for all 0 value in case of bug from Qt
                    (availableSpace==0 && storageInfo.bytesTotal()==0)
                    )
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("availableSpace: %1, space needed: %2, on: %3, bytesFree: %4").arg(availableSpace).arg(spaceDrive.second).arg(drive).arg(bytesFree).toStdString());
            }
            else if(spaceDrive.second>(quint64)availableSpace)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("availableSpace: %1, space needed: %2, on: %3, bytesFree: %4").arg(availableSpace).arg(spaceDrive.second).arg(drive).arg(bytesFree).toStdString());
                #ifdef Q_OS_WIN32
                //if(drive.contains(QRegularExpression("^[a-zA-Z]:[\\\\/]")))
                if(drive.contains(QRegularExpression("^[a-zA-Z]:")))
                #endif
                {
                    Diskspace diskspace;
                    diskspace.drive=spaceDrive.first;
                    diskspace.freeSpace=availableSpace;
                    diskspace.requiredSpace=spaceDrive.second;
                    diskspace_list.push_back(diskspace);
                }
                #ifdef Q_OS_WIN32
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"not local drive");
                #endif
            }
        #ifdef Q_OS_WIN32
        }
        #endif
    }
    if(!diskspace_list.empty())
        emit missingDiskSpace(diskspace_list);
    return ! diskspace_list.empty();
}
