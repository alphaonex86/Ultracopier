/** \file TransferThreadSync.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef TransferThreadSync_H
#define TransferThreadSync_H

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

#include "ReadThread.h"
#include "WriteThread.h"
#include "../TransferThread.h"
#include "Environment.h"
#include "DriveManagement.h"
#include "StructEnumDefinition_CopyEngine.h"

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThreadSync : public TransferThread
{
    Q_OBJECT
public:
    explicit TransferThreadSync();
    ~TransferThreadSync();
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
    void readIsReady();
    void writeIsReady();
    void readIsFinish();
    void writeIsFinish();
    void readIsClosed();
    void writeIsClosed();
    void getWriteError();
    void getReadError();
    //void syncAfterErrorAndReadFinish();
    void readThreadIsSeekToZeroAndWait();
    void writeThreadIsReopened();
    void readThreadResumeAfterError();
    //to filter the emition of signal
    void readIsStopped();
    void writeIsStopped();
    /// \brief to set files to transfer
    bool setFiles(const std::string& source,const int64_t &size,const std::string& destination,const Ultracopier::CopyMode &mode);
    bool checkIfAllIsClosedAndDoOperations();
    void resumeTransferAfterWriteError();
private:
    ReadThread		readThread;
    WriteThread		writeThread;
    bool sended_state_readStopped;
    bool sended_state_writeStopped;

    //ready = open + ready to operation (no error to resolv)
    bool			readIsReadyVariable;
    bool			writeIsReadyVariable;
    //can be open but with error
    bool            readIsOpeningVariable;//after call open() and before the end of internalOpen(), mostly to prevent internal error by open() when another is running
    bool            writeIsOpeningVariable;//after call open() and before the end of internalOpen(), mostly to prevent internal error by open() when another is running
    bool			readIsOpenVariable;
    bool			writeIsOpenVariable;
    bool			readIsFinishVariable;
    bool			writeIsFinishVariable;
    bool			readIsClosedVariable;
    bool			writeIsClosedVariable;
    //error management
    bool			writeError_destination_reopened;
    void tryMoveDirectly();
    void tryCopyDirectly();
    void tryOpen();
    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
};

#endif // TransferThreadSync_H
