/** \file TransferThreadAsync.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QTime>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>

//defore the next define
#include "../CopyEngineUltracopier-SpecVariable.h"

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

#include "../TransferThread.h"
#include "../Environment.h"
#include "../DriveManagement.h"
#include "../StructEnumDefinition_CopyEngine.h"

#ifndef TransferThreadAsync_H
#define TransferThreadAsync_H

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThreadAsync : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadAsync();
    ~TransferThreadAsync();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
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
private slots:
    void preOperation();
    void postOperation();
    //force into the right thread
    void internalStartTheTransfer();
signals:
    //internal signal
    void internalStartResumeAfterErrorAndSeek() const;
    void internalStartPostOperation() const;
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
    /// \brief to set files to transfer
    bool setFiles(const std::string& source,const int64_t &size,const std::string& destination,const Ultracopier::CopyMode &mode);
    /// \brief to set file exists action to do
    void setFileExistsAction(const FileExistsAction &action);
    #ifndef Q_OS_WIN32
    //fake copy for no win32
    int copy(const char *from,const char *to);
    #endif
    #ifdef Q_OS_WIN32
    void setProgression(const uint64_t &pos);
    #endif
private:
    //ready = open + ready to operation (no error to resolv)
    bool			transferIsReadyVariable;
    uint64_t transferProgression;
    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
};

#endif // TransferThreadAsync_H
