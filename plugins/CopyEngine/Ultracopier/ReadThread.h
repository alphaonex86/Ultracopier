/** \file ReadThread.h
\brief Thread changed to open/close and read the source file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef READTHREAD_H
#define READTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QSemaphore>
#include <QTimer>
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
    void open(const QString &name,const Ultracopier::CopyMode &mode);
    /// \brief return the error string
    QString errorString();
    //QByteArray read(qint64 position,qint64 maxSize);
    /// \brief stop the copy
    void stop();
    /// \brief put the copy in pause
    bool pause();
    /// \brief resume the copy
    void resume();
    /// \brief get the size of the source file
    qint64 size();
    /// \brief get the last good position
    qint64 getLastGoodPosition();
    /// \brief start the reading of the source file
    void startRead();
    /// \brief set the current max speed in KB/s
    int setMaxSpeed(int maxSpeed);
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
        bool isReading();
    /// \brief executed at regular interval to do a speed throling
    void timeOfTheBlockCopyFinished();
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
    void error();
    void isInPause();
    void opened();
    void readIsStarted();
    void readIsStopped();
    void closed();
    void isSeekToZeroAndWait();
    void checkIfIsWait();
    void resumeAfterErrorByRestartAll();
    void resumeAfterErrorByRestartAtTheLastPosition();
    void checksumFinish(const QByteArray&);
        // internal signals
    void internalStartOpen();
    void internalStartChecksum();
        void internalStartReopen();
        void internalStartRead();
        void internalStartClose();
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,QString fonction,QString text,QString file,int ligne);

private:
    QString		name;
    QString		errorString_internal;
    AvancedQFile	file;
    volatile bool	stopIt;
    Ultracopier::CopyMode	mode;
    qint64		lastGoodPosition;
    volatile int	blockSize;
    volatile int	maxSpeed;		///< The max speed in KB/s, 0 for no limit
    QSemaphore	waitNewClockForSpeed;
    volatile int	numberOfBlockCopied;		///< Multiple for count the number of block copied
    volatile int	multiplicatorForBigSpeed;	///< Multiple for count the number of block needed
    volatile int	MultiForBigSpeed;
    WriteThread*	writeThread;
    int		id;
    QSemaphore	isOpen;
    volatile bool	putInPause;
    volatile bool	isInReadLoop;
    volatile bool	seekToZero;
        volatile bool	tryStartRead;
        qint64		size_at_open;
    QDateTime	mtime_at_open;
    bool		fakeMode;
        //internal function
        bool seek(qint64 position);/// \todo search if is use full
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
