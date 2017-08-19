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
    void error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error);
    /** method called when the log file need be created */
    void openLogs();
    /** method called when the log file need be closed */
    void closeLogs();
    /** method called when one folder is removed */
    void rmPath(const std::string &path);
    /** method called when one folder is created */
    void mkPath(const std::string &path);
private slots:
    /** \to write the data into the file */
    void realDataWrite(const std::string &text);
    /** \to update the options value */
    void newOptionValue(const std::string &group,const std::string &name,const std::string &value);
signals:
    void newData(const std::string &text) const;
private:
    std::string data;
    std::string transfer_format;
    std::string error_format;
    std::string folder_format;
    QFile log;
    std::string lineReturn;
    std::string replaceBaseVar(std::string text);
    #ifdef Q_OS_WIN32
    QString computer;
    QString user;
    #endif
    bool sync;
    bool enabled;
    bool log_enable_transfer;
    bool log_enable_error;
    bool log_enable_folder;

    static std::string text_header_copy;
    static std::string text_header_move;
    static std::string text_header_skip;
    static std::string text_header_stop;
    static std::string text_header_error;
    static std::string text_header_MkPath;
    static std::string text_header_RmPath;

    static std::string text_var_source;
    static std::string text_var_size;
    static std::string text_var_destination;
    static std::string text_var_path;
    static std::string text_var_error;
    static std::string text_var_mtime;
    static std::string text_var_time;
    static std::string text_var_timestring;
    #ifdef Q_OS_WIN32
    static std::string text_var_computer;
    static std::string text_var_user;
    #endif
    static std::string text_var_operation;
    static std::string text_var_rmPath;
    static std::string text_var_mkPath;
protected:
    void run();
};

#endif // LOGTHREAD_H
