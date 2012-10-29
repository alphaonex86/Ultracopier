#include "DriveManagement.h"

#include <QStorageInfo>

//get drive of an file or folder
QString DriveManagement::getDrive(QString fileOrFolder)
{
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(fileOrFolder.startsWith(mountSysPoint.at(i)))
            return mountSysPoint.at(i);
    }
    //if unable to locate the right mount point
    return "";
}

void DriveManagement::setDrive(QStringList mountSysPoint)
{
    this->mountSysPoint=mountSysPoint;
}

bool DriveManagement::isSameDrive(QString file1,QString file2)
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
