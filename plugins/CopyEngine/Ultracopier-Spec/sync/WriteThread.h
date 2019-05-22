/** \file WriteThread.h
\brief Thread changed to open/close and write the destination file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef WRITETHREAD_H
#define WRITETHREAD_H

#define BLOCKDEFAULTINITVAL 0//to test and debug, in production should be at 0

#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"
#include <cstdint>
#include "Variable.h"
#include "CallBackEventLoop.h"

class ReadThread;
/// \brief Thread changed to open/close and write the destination file
class WriteThread : public QObject
        , public CallBackEventLoop
{
    Q_OBJECT
public:
    explicit WriteThread();
    ~WriteThread();
protected:
    void run();
public:
    /// \brief open the destination to open it
    void open(const std::string &file, const uint64_t &startSize);
    /// \brief to return the error string
    std::string errorString() const;
    /// \brief to stop all
    void stop();
    /// \brief to write data
    bool write();
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief get the write stat
    enum WriteStat
    {
        Idle=0,
        InodeOperation=1,
        Write=2,
        Close=3,
        Read=5
    };
    WriteStat status;
    #endif
    /// \brief do the fake open
    void fakeOpen();
    /// \brief do the fake writeIsStarted
    void fakeWriteIsStarted();
    /// \brief do the fake writeIsStopped
    void fakeWriteIsStopped();
    /// \brief set block size in KB
    int64_t getLastGoodPosition() const;
    /// \brief buffer is empty
    bool bufferIsEmpty();
    void reemitStartOpen();

    /* temp data for block writing, type: ring buffer
     * bool to view diff then Max content start/stop==empty */
    char blockArray[/*1024**/1024];
    std::atomic<bool> blockArrayIsFull;
    /* posible start stop pair with blockArray[1024]:
     * 0 1023: all the block, [0], [1], ... [1021], [1022]
     * 0 1024: all the block, [0], [1], ... [1022], [1023]
     * 1 1024: all the block, [1], [2], ... [1022], [1023]
     * 2 1: all the block, [2], [3], ... [1023], [0]
     * 200 199: all the block but start read byte at [200], [201], ... [1023] (or 824 bytes to the end), [0], [1], [2] ... [197], [198] (or 199 bytes to the end) is 824+199=1023 content max
     * 0 0: empty block,
     * 0 1: only the [0] have content
     * 7 9: only the [7][8] have content
     * 328 328: empty block too
    */

    //where start used block, include
    std::atomic<std::uint32_t> blockArrayStart;

    /* where stop used block, exclude */
    std::atomic<std::uint32_t> blockArrayStop;

    void setReadThread(ReadThread * readThread);
    void callBack();
public slots:
    /// \brief start the operation
    void postOperation();
    /// \brief flush buffer
    void flushBuffer();
    /// \brief set the end is detected
    void endIsDetected();
    /// \brief reopen the file
    void reopen();
    /// \brief flush and seek to zero
    void flushAndSeekToZero();
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
signals:
    void error() const;
    void opened() const;
    void reopened() const;
    void writeIsStarted() const;
    void writeIsStopped() const;
    void flushedAndSeekedToZero() const;
    void closed() const;
    //internal signals
    void internalStartOpen() const;
    void internalStartReopen() const;
    void internalStartWrite() const;
    void internalStartClose() const;
    void internalStartEndOfFile() const;
    void internalStartFlushAndSeekToZero() const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
private:
    FILE * file;
    std::string fileName;
    std::string             errorString_internal;
    volatile bool		stopIt;
    volatile bool       postOperationRequested;
    volatile bool       writeFullBlocked;
    uint64_t             lastGoodPosition;
    int                 id;
    volatile bool       endDetected;
    uint64_t             startSize;
    bool                fakeMode;
    bool                needRemoveTheFile;
    bool                deletePartiallyTransferredFiles;
    ReadThread *        readThread;
private slots:
    bool internalOpen();
    void internalWrite();
    void internalCloseSlot();
    void internalClose(bool emitSignal=true);
    void internalReopen();
    void internalEndOfFile();
    void internalFlushAndSeekToZero();
};

#endif // WRITETHREAD_H
