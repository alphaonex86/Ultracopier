/** \file ReadThread.h
\brief Thread changed to open/close and read the source file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef READTHREAD_H
#define READTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QSemaphore>
#include <QDateTime>
#include <QFileInfo>
#include <QCryptographicHash>

#include "WriteThread.h"
#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"
#include "AvancedQFile.h"

/// \brief Thread changed to open/close and read the source file
class ReadThread : public QThread
{
    Q_OBJECT
public:
    explicit ReadThread();
    ~ReadThread();
protected:
    void run();
public:
    /// \brief open with the name and copy mode
    void open(const QFileInfo &file, const Ultracopier::CopyMode &mode);
    /// \brief return the error string
    std::string errorString() const;
    //QByteArray read(qint64 position,qint64 maxSize);
    /// \brief stop the copy
    void stop();
    /// \brief put the copy in pause
    void pause();
    /// \brief resume the copy
    void resume();
    /// \brief get the size of the source file
    int64_t size() const;
    /// \brief get the last good position
    int64_t getLastGoodPosition() const;
    /// \brief start the reading of the source file
    void startRead();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    /// \brief set the current max speed in KB/s
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    #endif
    /// \brief set block size in KB
    bool setBlockSize(const int blockSize);
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
        WaitWritePipe=3,
        Checksum=4
    };
    ReadStat stat;
    #endif
    /// \brief return if it's reading
    bool isReading() const;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    /// \brief executed at regular interval to do a speed throling
    void timeOfTheBlockCopyFinished();
    #endif
    /// \brief do the fake open
    void fakeOpen();
    /// \brief do the fake readIsStarted
    void fakeReadIsStarted();
    /// \brief do the fake readIsStopped
    void fakeReadIsStopped();
    /// do the checksum
    void startCheckSum();
public slots:
    /// \brief to reset the copy, and put at the same state when it just open
    void seekToZeroAndWait();
    void postOperation();
    /// do the checksum
    void checkSum();
signals:
    void error() const;
    void opened() const;
    void readIsStarted() const;
    void readIsStopped() const;
    void closed() const;
    void isSeekToZeroAndWait() const;
    void checkIfIsWait() const;
    void resumeAfterErrorByRestartAll() const;
    void resumeAfterErrorByRestartAtTheLastPosition() const;
    void checksumFinish(const QByteArray&) const;
    // internal signals
    void internalStartOpen() const;
    void internalStartChecksum() const;
    void internalStartReopen() const;
    void internalStartRead() const;
    void internalStartClose() const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;

private:
    std::string         errorString_internal;
    AvancedQFile	file;
    volatile bool	stopIt;
    Ultracopier::CopyMode	mode;
    int64_t          lastGoodPosition;
    volatile int	blockSize;//in Bytes
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QSemaphore      waitNewClockForSpeed;
    volatile int	numberOfBlockCopied;		///< Multiple for count the number of block copied
    volatile int	multiForBigSpeed;           ///< Multiple for count the number of block needed
    #endif
    WriteThread*	writeThread;
    int		id;
    QSemaphore      isOpen;
    QSemaphore      pauseMutex;
    volatile bool	putInPause;
    volatile bool	isInReadLoop;
    volatile bool	seekToZero;
    volatile bool	tryStartRead;
    int64_t          size_at_open;
    uint64_t       mtime_at_open;
    bool            fakeMode;
    //internal function
    bool seek(const int64_t &position);/// \todo search if is use full
private slots:
    bool internalOpen(bool resetLastGoodPosition=true);
    bool internalOpenSlot();
    bool internalReopen();
    void internalRead();
    void internalClose(bool callByTheDestructor=false);
    void internalCloseSlot();
    void isInWait();
};

#endif // READTHREAD_H
