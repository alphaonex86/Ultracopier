#include "DriveManagement.h"

#include <QDir>
#include <QFileInfoList>
#include <QStorageInfo>

#include "../../../cpp11addition.h"

DriveManagement::DriveManagement()
{
    tryUpdate();
    #ifdef Q_OS_WIN32
    reg1=QRegularExpression(QStringLiteral("^(\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+"));
    reg2=QRegularExpression(QStringLiteral("^((\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+).*$"));
    reg3=QRegularExpression(QStringLiteral("^[a-zA-Z]:[\\\\/]"));
    reg4=QRegularExpression(QStringLiteral("^([a-zA-Z]:[\\\\/]).*$"));
    #endif
    /// \warn ULTRACOPIER_DEBUGCONSOLE() don't work here because the sinal slot is not connected!
}

//get drive of an file or folder
std::string DriveManagement::getDrive(const std::string &fileOrFolder) const
{
    const std::string &inode=QDir::toNativeSeparators(QString::fromStdString(fileOrFolder)).toStdString();
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(stringStartWith(inode,mountSysPoint.at(i)))
            return QDir::toNativeSeparators(QString::fromStdString(mountSysPoint.at(i))).toStdString();
    }
    #ifdef Q_OS_WIN32
    if(fileOrFolder.contains(reg1))
    {
        QString returnString=fileOrFolder;
        returnString.replace(reg2,QStringLiteral("\\1"));
        return returnString;
    }
    //due to lack of WMI support into mingw, the new drive event is never called, this is a workaround
    if(fileOrFolder.contains(reg3))
    {
        QString returnString=fileOrFolder;
        returnString.replace(reg4,QStringLiteral("\\1"));
        return QDir::toNativeSeparators(returnString).toUpper();
    }
    #endif
    //if unable to locate the right mount point
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unable to locate the right mount point for: "+fileOrFolder+", mount point: "+stringimplode(mountSysPoint,";"));
    return std::string();
}

QByteArray DriveManagement::getDriveType(const std::string &drive) const
{
    int index=vectorindexOf(mountSysPoint,drive);
    if(index!=-1)
        return driveType.at(index);
    return QByteArray();
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
        if(mountSysPoint.last()!="A:\\" && mountSysPoint.last()!="A:/" && mountSysPoint.last()!="A:" && mountSysPoint.last()!="A" &&
                mountSysPoint.last()!="a:\\" && mountSysPoint.last()!="a:/" && mountSysPoint.last()!="a:" && mountSysPoint.last()!="a")
            driveType.push_back(mountedVolumesList.at(index).fileSystemType());
        else
            driveType.push_back(QByteArray());
        #else
        driveType.push_back(mountedVolumesList.at(index).fileSystemType());
        #endif
        index++;
    }
}
