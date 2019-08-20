#include "DriveManagement.h"

#include <QDir>
#include <QFileInfoList>
#include <QStorageInfo>

#include "../../../cpp11addition.h"

DriveManagement::DriveManagement()
{
    tryUpdate();
    #ifdef Q_OS_WIN32
    reg1=std::regex("^(\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+.*");
    reg2=std::regex("^((\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+).*$");
    reg3=std::regex("^[a-zA-Z]:[\\\\/].*");
    reg4=std::regex("^([a-zA-Z]:[\\\\/]).*$");
    #endif
    /// \warn ULTRACOPIER_DEBUGCONSOLE() don't work here because the sinal slot is not connected!
}

//get drive of an file or folder
/// \todo do network drive support for windows
std::string DriveManagement::getDrive(const std::string &fileOrFolder) const
{
    const std::string &inode=fileOrFolder;
    #ifdef Q_OS_WIN32
    //optimized to windows version:
    if(fileOrFolder.size()>=3)
    {
        if(fileOrFolder.at(1)==L':' && (fileOrFolder.at(2)==L'\\' || fileOrFolder.at(2)==L'/'))
        {
            char driveLetter=toupper(fileOrFolder.at(0));
            return driveLetter+std::string(":/");
        }
    }

    if(std::regex_match(fileOrFolder,reg1))
    {
        std::string returnString=fileOrFolder;
        std::regex_replace(returnString,reg2,"$1");
        return returnString;
    }
    //due to lack of WMI support into mingw, the new drive event is never called, this is a workaround
    if(std::regex_match(fileOrFolder,reg3))
    {
        std::string returnString=fileOrFolder;
        std::regex_replace(returnString,reg4,"$1");
        return QDir::toNativeSeparators(QString::fromStdString(returnString)).toUpper().toStdString();
    }
    #else
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(stringStartWith(inode,mountSysPoint.at(i)))
            return mountSysPoint.at(i);
    }
    #endif
    //if unable to locate the right mount point
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unable to locate the right mount point for: "+inode+", mount point: "+stringimplode(mountSysPoint,";"));
    return std::string();
}

std::string DriveManagement::getDriveType(const std::string &drive) const
{
    int index=vectorindexOf(mountSysPoint,drive);
    if(index!=-1)
        return driveType.at(index);
    return std::string();
}

bool DriveManagement::isSameDrive(const std::string &file1,const std::string &file2) const
{
    if(mountSysPoint.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"no mount point found");
        return false;
    }
    const std::string &drive1=getDrive(file1);
    if(drive1.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"drive for the file1 not found: "+file1);
        return false;
    }
    const std::string &drive2=getDrive(file2);
    if(drive2.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"drive for the file2 not found: "+file2);
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,drive1+" is egal to "+drive2);
    if(drive1==drive2)
        return true;
    else
        return false;
}

void DriveManagement::tryUpdate()
{
    mountSysPoint.clear();
    driveType.clear();
    const QList<QStorageInfo> mountedVolumesList=QStorageInfo::mountedVolumes();
    int index=0;
    while(index<mountedVolumesList.size())
    {
        mountSysPoint.push_back(QDir::toNativeSeparators(mountedVolumesList.at(index).rootPath()).toStdString());
        #ifdef Q_OS_WIN32
        if(mountSysPoint.back()!="A:\\" && mountSysPoint.back()!="A:/" && mountSysPoint.back()!="A:" && mountSysPoint.back()!="A" &&
                mountSysPoint.back()!="a:\\" && mountSysPoint.back()!="a:/" && mountSysPoint.back()!="a:" && mountSysPoint.back()!="a")
        {
            const QByteArray &data=mountedVolumesList.at(index).fileSystemType();
            driveType.push_back(std::string(data.constData(),data.size()));
        }
        else
            driveType.push_back(std::string());
        #else
        const QByteArray &data=mountedVolumesList.at(index).fileSystemType();
        driveType.push_back(std::string(data.constData(),data.size()));
        #endif
        index++;
    }
}
