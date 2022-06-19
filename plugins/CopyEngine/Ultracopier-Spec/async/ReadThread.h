/** \file ReadThread.h
\brief Thread changed to open/close and read the source file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef READTHREAD_H
#define READTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QSemaphore>
#include <QCryptographicHash>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "WriteThread.h"
#include "../Environment.h"
#include "../StructEnumDefinition_CopyEngine.h"
#include "../CopyEngineUltracopier-SpecVariable.h"

#ifdef WIDESTRING
#define INTERNALTYPEPATH std::wstring
#define INTERNALTYPECHAR wchar_t
#else
#define INTERNALTYPEPATH std::string
#define INTERNALTYPECHAR char
#endif

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
    void openRead(const INTERNALTYPEPATH &file, const Ultracopier::CopyMode &mode);
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
    /// \brief set block size in KB mostly for speed
    bool setBlockSize(const int blockSize);

    void reopen();//-> not valid in version 2, beacause it restart from open in case of error
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
    ReadStat status;
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

    void setOsSpecFlags(bool os_spec_flags);
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
    void resumeAfterErrorByRestartAtTheLastPosition() const;
    // internal signals
    void internalStartOpen() const;
    //void internalStartReopen() const;-> not valid in version 2, beacause it restart from open in case of error
    void internalStartRead() const;
    void internalStartClose() const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;

private:
    std::string         errorString_internal;
    volatile bool	stopIt;
    Ultracopier::CopyMode	mode;
    bool os_spec_flags;
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
    #ifdef Q_OS_UNIX
    uint64_t       mtime_at_open;
    #else
    FILETIME       mtime_at_open;
    #endif
    bool            fakeMode;
    //internal function
    bool seek(const int64_t &position);/// \todo search if is use full

    #ifdef Q_OS_UNIX
    int from;
    #else
    HANDLE from;
    #endif
    INTERNALTYPEPATH file;
private slots:
    bool internalOpen(bool resetLastGoodPosition=true);
    bool internalOpenSlot();
    //bool internalReopen();
    void internalRead();
    void internalClose(bool callByTheDestructor=false);
    void internalCloseSlot();
    void isInWait();
};

#endif // READTHREAD_H
