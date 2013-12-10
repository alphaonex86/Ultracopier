#ifndef DRIVEMANAGEMENT_H
#define DRIVEMANAGEMENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>

#include "qstorageinfo.h"
#include "Environment.h"

class DriveManagement : public QObject
{
    Q_OBJECT
public:
    explicit DriveManagement();
    virtual void setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType);
    bool isSameDrive(const QString &file1,const QString &file2) const;
    /// \brief get drive of an file or folder
    QString getDrive(const QString &fileOrFolder) const;
    QStorageInfo::DriveType getDriveType(const QString &drive) const;
protected:
    QStringList		mountSysPoint;
    QList<QStorageInfo::DriveType> driveType;
    #ifdef Q_OS_WIN32
    QRegularExpression reg1,reg2,reg3,reg4;
    #endif
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne) const;
};

#endif // DRIVEMANAGEMENT_H
