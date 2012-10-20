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

#include "Environment.h"

/// \brief Make the path given as queued mkpath
class MkPath : public QThread
{
    Q_OBJECT
public:
    explicit MkPath();
    ~MkPath();
    /// \brief add path to make
    void addPath(const QString &path);
signals:
    void errorOnFolder(const QFileInfo &,const QString &);
    void firstFolderFinish();
    void internalStartAddPath(const QString &path);
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
    QStringList pathList;
    void checkIfCanDoTheNext();
    QDir dir;
private slots:
    void internalDoThisPath();
    void internalAddPath(const QString &path);
    void internalSkip();
    void internalRetry();
};

#endif // MKPATH_H
