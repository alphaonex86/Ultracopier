#ifndef DRIVEMANAGEMENT_H
#define DRIVEMANAGEMENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QStorageInfo>

#include "Environment.h"

class DriveManagement : public QObject
{
    Q_OBJECT
public:
    virtual void setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType);
    bool isSameDrive(const QString &file1,const QString &file2);
    /// \brief get drive of an file or folder
    QString getDrive(const QString &fileOrFolder);
    QStorageInfo::DriveType getDriveType(const QString &drive);
protected:
    QStringList		mountSysPoint;
    QList<QStorageInfo::DriveType> driveType;
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
};

#endif // DRIVEMANAGEMENT_H
