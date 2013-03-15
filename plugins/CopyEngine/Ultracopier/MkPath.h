/** \file MkPath.h
\brief Make the path given as queued mkpath
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef MKPATH_H
#define MKPATH_H

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QSemaphore>
#include <QStringList>
#include <QDir>
#include <QDateTime>

#include "Environment.h"

#ifdef Q_OS_UNIX
    #include <utime.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/stat.h>
#else
    #ifdef Q_OS_WIN32
        #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            #include <utime.h>
            #include <time.h>
            #include <unistd.h>
            #include <sys/stat.h>
        #endif
    #endif
#endif

/// \brief Make the path given as queued mkpath
class MkPath : public QThread
{
    Q_OBJECT
public:
    explicit MkPath();
    ~MkPath();
    /// \brief add path to make
    void addPath(const QFileInfo& source,const QFileInfo& destination,const ActionType &actionType);
    void setRightTransfer(const bool doRightTransfer);
    void setKeepDate(const bool keepDate);
signals:
    void errorOnFolder(const QFileInfo &,const QString &);
    void firstFolderFinish();
    void internalStartAddPath(const QFileInfo& source,const QFileInfo& destination, const ActionType &actionType);
    void internalStartDoThisPath();
    void internalStartSkip();
    void internalStartRetry();
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
public slots:
    /// \brief skip after creation error
    void skip();
    /// \brief retry after creation error
    void retry();
private:
    void run();
    bool waitAction;
    bool stopIt;
    bool skipIt;
    QDateTime		maxTime;
    struct Item
    {
        QFileInfo source;
        QFileInfo destination;
        ActionType actionType;
    };
    QList<Item> pathList;
    void checkIfCanDoTheNext();
    QDir dir;
    bool doRightTransfer;
    bool keepDate;
    bool doTheDateTransfer;
    #ifdef Q_OS_UNIX
            utimbuf butime;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                utimbuf butime;
            #else
                quint32 ftCreateL, ftAccessL, ftWriteL;
                quint32 ftCreateH, ftAccessH, ftWriteH;
            #endif
        #endif
    #endif
    //fonction to edit the file date time
    bool readFileDateTime(const QFileInfo &source);
    bool writeFileDateTime(const QFileInfo &destination);
private slots:
    void internalDoThisPath();
    void internalAddPath(const QFileInfo& source, const QFileInfo& destination,const ActionType &actionType);
    void internalSkip();
    void internalRetry();
    bool rmpath(const QDir &dir);
};

#endif // MKPATH_H
