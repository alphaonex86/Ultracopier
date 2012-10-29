#include "DriveManagement.h"

#include <QDir>
#include <QFileInfoList>

//get drive of an file or folder
QString DriveManagement::getDrive(const QString &fileOrFolder)
{
    QString inode=QDir::toNativeSeparators(fileOrFolder);
    #ifdef Q_OS_WIN32
    QFileInfoList drives=QDir::drives();
    int size=drives.size();
    for (int i = 0; i < size; ++i) {
        if(inode.startsWith(QDir::toNativeSeparators(drives.at(i).absoluteFilePath())))
            return QDir::toNativeSeparators(drives.at(i).absoluteFilePath());
    }
    #else
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(inode.startsWith(mountSysPoint.at(i)))
            return QDir::toNativeSeparators(mountSysPoint.at(i));
    }
    #endif
    //if unable to locate the right mount point
    return "";
}

void DriveManagement::setDrive(const QStringList &mountSysPoint)
{
    this->mountSysPoint=mountSysPoint;
}

bool DriveManagement::isSameDrive(const QString &file1,const QString &file2)
{
    #if defined (Q_OS_LINUX)
    if(mountSysPoint.size()==0)
        return false;
    #endif
    QString drive1=getDrive(file1);
    if(drive1.isEmpty())
        return false;
    QString drive2=getDrive(file2);
    if(drive2.isEmpty())
        return false;
    if(drive1==drive2)
        return true;
    else
        return false;
}
