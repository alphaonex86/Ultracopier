/** \file LogThread.h
\brief The thread to do the log but not block the main thread
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <QString>
#include <QDateTime>
#include <QVariant>

#include "Environment.h"
#include "StructEnumDefinition.h"

/** \brief Log all the user oriented activity

It use thread based storage to prevent gui thread freeze on log file writing when is out of the disk buffer. That's allow to async the event.
*/
class LogThread : public QThread
{
    Q_OBJECT
public:
    explicit LogThread();
     ~LogThread();
    bool logTransfer();
public slots:
    /** method called when new transfer is started */
    void newTransferStart(const Ultracopier::ItemOfCopyList &item);
    /** method called when transfer is stopped */
    void newTransferStop(const Ultracopier::ItemOfCopyList &item);
    /** method called when new transfer is started */
    void transferSkip(const Ultracopier::ItemOfCopyList &item);
    /** method called when new error is occurred */
    void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error);
    /** method called when the log file need be created */
    void openLogs();
    /** method called when the log file need be closed */
    void closeLogs();
    /** method called when one folder is removed */
    void rmPath(const QString &path);
    /** method called when one folder is created */
    void mkPath(const QString &path);
private slots:
    /** \to write the data into the file */
    void realDataWrite(const QString &text);
    /** \to update the options value */
    void newOptionValue(const QString &group,const QString &name,const QVariant &value);
signals:
    void newData(const QString &text);
private:
    QString data;
    QString transfer_format;
    QString error_format;
    QString folder_format;
    QFile log;
    QString replaceBaseVar(QString text);
    #ifdef Q_OS_WIN32
    QString computer;
    QString user;
    #endif
    bool sync;
    bool enabled;
    bool log_enable_transfer;
    bool log_enable_error;
    bool log_enable_folder;
protected:
    void run();
};

#endif // LOGTHREAD_H
