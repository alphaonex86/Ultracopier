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
    bool isSameDrive(const std::string &file1, const std::string &file2) const;
    /// \brief get drive of an file or folder
    std::string getDrive(const std::string &fileOrFolder) const;
    std::string getDriveType(const std::string &drive) const;
    void tryUpdate();
protected:
    std::vector<std::string>		mountSysPoint;
    std::vector<QByteArray> driveType;
    #ifdef Q_OS_WIN32
    std::regex reg1,reg2,reg3,reg4;
    #endif
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
};

#endif // DRIVEMANAGEMENT_H
