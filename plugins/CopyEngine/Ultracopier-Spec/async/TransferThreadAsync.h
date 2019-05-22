/** \file TransferThreadAsync.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef TransferThreadAsync_H
#define TransferThreadAsync_H

#include <QObject>
#include <QTime>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>

//defore the next define
#include "Variable.h"

#ifdef Q_OS_UNIX
    #include <utime.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif
#ifdef Q_OS_WIN32
    #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #include <utime.h>
        #include <time.h>
        #include <unistd.h>
        #include <sys/stat.h>
    #endif
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "Environment.h"
#include "DriveManagement.h"
#include "StructEnumDefinition_CopyEngine.h"
#include "../TransferThread.h"

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThreadAsync : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadAsync();
    ~TransferThreadAsync();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief get the reading letter
    char readingLetter() const;
    /// \brief get the writing letter
    char writingLetter() const;
    #endif

    //not copied size, ...
    uint64_t realByteTransfered() const;
    std::pair<uint64_t, uint64_t> progression() const;
protected:
    void run();
public slots:
    /// \brief to start the transfer of data
    void startTheTransfer();
    /// \brief stop the copy
    void stop();
    /// \brief skip the copy
    void skip();
    /// \brief retry after error
    void retryAfterError();
    /// \brief return info about the copied size
    int64_t copiedSize();
    /// \brief put the current file at bottom
    void putAtBottom();
private slots:
    void preOperation();
    void postOperation();
    //force into the right thread
    void internalStartTheTransfer();
    void ifCanStartTransfer();
};

#endif // TransferThreadAsync_H
