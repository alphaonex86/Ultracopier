/** \file ReadThread.h
\brief Thread changed to open/close and read the source file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef READTHREAD_H
#define READTHREAD_H

#include "WriteThread.h"
#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"
#include "Variable.h"
#include "CallBackEventLoop.h"

/// \brief Thread changed to open/close and read the source file
class ReadThread : public QObject
        , public CallBackEventLoop
{
    Q_OBJECT
public:
    explicit ReadThread();
    ~ReadThread();
protected:
    void run();
public:
    /// \brief open with the name and copy mode
    void open(const std::string &file, const Ultracopier::CopyMode &mode);
    /// \brief return the error string
    std::string errorString() const;
    /// \brief stop the copy
    void stop();
    /// \brief get the size of the source file
    int64_t size() const;
    /// \brief get the last good position
    int64_t getLastGoodPosition() const;
    /// \brief start the reading of the source file
    void startRead();
    /// \brief reopen after an error
    void reopen();
    /// \brief set the write thread
    void setWriteThread(WriteThread * writeThread);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief stat
    enum ReadStat
    {
        Idle=0,
        InodeOperation=1,
        Read=2,
        WaitWritePipe=3
    };
    ReadStat stat;
    #endif
    /// \brief return if it's reading
    bool isReading() const;
    /// \brief do the fake open
    void fakeOpen();
    /// \brief do the fake readIsStarted
    void fakeReadIsStarted();
    /// \brief do the fake readIsStopped
    void fakeReadIsStopped();
    void callBack();
public slots:
    /// \brief to reset the copy, and put at the same state when it just open
    void seekToZeroAndWait();
    void postOperation();
signals:
    void error() const;
    void opened() const;
    void readIsStarted() const;
    void readIsStopped() const;
    void closed() const;
    void isSeekToZeroAndWait() const;
    void checkIfIsWait() const;
    void resumeAfterErrorByRestartAll() const;
    // internal signals
    void internalStartOpen() const;
    void internalStartReopen() const;
    void internalStartRead() const;
    void internalStartClose() const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;

private:
    FILE * file;
    std::string fileName;
    std::string         errorString_internal;
    volatile bool	stopIt;
    Ultracopier::CopyMode	mode;
    int64_t          lastGoodPosition;//to restaure and reopen after file disconnected (eg NAS)
    volatile int	blockSize;//in Bytes
    WriteThread*	writeThread;
    int		id;
    volatile bool	isInReadLoop;
    volatile bool	tryStartRead;
    bool            fakeMode;
    //internal function
    bool seek(const int64_t &position);/// \todo search if is use full
private slots:
    bool internalOpen();
    bool internalOpenSlot();
    bool internalReopen();
    void internalRead();
    void internalClose(bool callByTheDestructor=false);
    void internalCloseSlot();
    void isInWait();
};

#endif // READTHREAD_H
