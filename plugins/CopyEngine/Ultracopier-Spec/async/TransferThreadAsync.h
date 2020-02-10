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

#include "ReadThread.h"
#include "WriteThread.h"

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
    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    //set block size in Bytes for speed limitation
    bool setBlockSize(const unsigned int blockSize);
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    void timeOfTheBlockCopyFinished();
    #endif
    /// \brief pause the copy
    void pause();
    /// \brief resume the copy
    void resume();

    bool haveStartTime;
    ReadThread readThread;
    WriteThread writeThread;
    void setBuffer(const bool buffer);

    #ifdef Q_OS_WIN32
    static bool mkJunction(LPCWSTR szJunction, LPCWSTR szPath);
    #endif
protected:
    void run();
private slots:
    void preOperation();
    void postOperation();
    //force into the right thread
    void internalStartTheTransfer();

    //implemente to connect async
    void read_error();
    void read_readIsStopped();
    void read_closed();
    void write_error();
    void write_closed();
    void read_opened();
    void write_opened();
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
    bool setFiles(const INTERNALTYPEPATH &source, const int64_t &size, const INTERNALTYPEPATH &destination, const Ultracopier::CopyMode &mode);
    /// \brief to set file exists action to do
    void setFileExistsAction(const FileExistsAction &action);
    #ifdef Q_OS_WIN32
    void setProgression(const uint64_t &pos,const uint64_t &size);
    #endif
private:
    //ready = open + ready to operation (no error to resolv)
    bool			transferIsReadyVariable;
    uint64_t transferProgression;
    bool sended_state_readStopped;
    bool readIsClosedVariable;
    bool writeIsClosedVariable;
    bool readIsOpenVariable;
    bool writeIsOpenVariable;
    bool realMove;
    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
    void resetExtraVariable();
    void ifCanStartTransfer();
    void checkIfAllIsClosedAndDoOperations();
};

#endif // TransferThreadAsync_H
