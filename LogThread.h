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
#include <QFile>

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
    bool logTransfer() const;
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
    void newData(const QString &text) const;
private:
    QString data;
    QString transfer_format;
    QString error_format;
    QString folder_format;
    QFile log;
    QString lineReturn;
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

    static QString text_header_copy;
    static QString text_header_move;
    static QString text_header_skip;
    static QString text_header_stop;
    static QString text_header_error;
    static QString text_header_MkPath;
    static QString text_header_RmPath;

    static QString text_var_source;
    static QString text_var_size;
    static QString text_var_destination;
    static QString text_var_path;
    static QString text_var_error;
    static QString text_var_mtime;
    static QString text_var_time;
    static QString text_var_timestring;
    #ifdef Q_OS_WIN32
    static QString text_var_computer;
    static QString text_var_user;
    #endif
    static QString text_var_operation;
    static QString text_var_rmPath;
    static QString text_var_mkPath;
protected:
    void run();
};

#endif // LOGTHREAD_H
