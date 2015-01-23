#ifndef DRIVEMANAGEMENT_H
#define DRIVEMANAGEMENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QTimer>

#include "Environment.h"

class DriveManagement : public QObject
{
    Q_OBJECT
public:
    explicit DriveManagement();
    bool isSameDrive(const QString &file1,const QString &file2) const;
    /// \brief get drive of an file or folder
    QString getDrive(const QString &fileOrFolder) const;
    QByteArray getDriveType(const QString &drive) const;
    void tryUpdate();
protected:
    QStringList		mountSysPoint;
    QList<QByteArray> driveType;
    #ifdef Q_OS_WIN32
    QRegularExpression reg1,reg2,reg3,reg4;
    #endif
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne) const;
};

#endif // DRIVEMANAGEMENT_H
