#ifndef DRIVEMANAGEMENT_H
#define DRIVEMANAGEMENT_H

#include <QString>
#include <QStringList>

class DriveManagement
{
public:
    virtual void setDrive(QStringList mountSysPoint);
protected:
    /// \brief get drive of an file or folder
    QString getDrive(QString fileOrFolder);
    bool isSameDrive(QString file1,QString file2);
    QStringList		mountSysPoint;
};

#endif // DRIVEMANAGEMENT_H
