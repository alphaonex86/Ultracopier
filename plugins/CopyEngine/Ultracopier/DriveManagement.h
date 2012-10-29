#ifndef DRIVEMANAGEMENT_H
#define DRIVEMANAGEMENT_H

#include <QString>
#include <QStringList>

class DriveManagement
{
public:
    virtual void setDrive(const QStringList &mountSysPoint);
protected:
    /// \brief get drive of an file or folder
    QString getDrive(const QString &fileOrFolder);
    bool isSameDrive(const QString &file1,const QString &file2);
    QStringList		mountSysPoint;
};

#endif // DRIVEMANAGEMENT_H
