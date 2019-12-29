/** \file WriteThread.h
\brief Thread changed to open/close and write the destination file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef WRITETHREAD_H
#define WRITETHREAD_H

#include <QThread>
#include <QByteArray>
#include <QMutex>
#include <QSemaphore>
#include <QCryptographicHash>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

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

/// \brief Thread changed to open/close and write the destination file
class WriteThread : public QThread
{
    Q_OBJECT
public:
    explicit WriteThread();
    ~WriteThread();
    //internal function
    bool seek(const int64_t &position);/// \todo search if is use full
    /// \brief get the size of the destination file
    int64_t size() const;

    //can't be static into WriteThread, linked by instance then by ListThread
    QMultiHash<QString,WriteThread *> *writeFileList;
    QMutex       *writeFileListMutex;
protected:
    void run();
public:
    /// \brief open the destination to open it
    void open(const INTERNALTYPEPATH &file,const uint64_t &startSize);
    /// \brief to return the error string
    std::string errorString() const;
    /// \brief to stop all
    void stop();
    /// \brief to write data
    bool write(const QByteArray &data);
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
    /// \brief set block size in KB mostly for speed
    bool setBlockSize(const int blockSize);
    /// \brief get the last good position
    int64_t getLastGoodPosition() const;
    /// \brief buffer is empty
    bool bufferIsEmpty();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    /// \brief set the current max speed in KB/s
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    #endif
    void pause();
    void resume();
    void reemitStartOpen();
    //return 0 if sucess
    int destTruncate(const uint64_t &startSize);
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
    /// \brief executed at regular interval to do a speed throling
    void timeOfTheBlockCopyFinished();

    void resumeNotStarted();
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
    std::string             errorString_internal;
    volatile bool		stopIt;
    volatile bool       postOperationRequested;
    int                 numberOfBlock;
    QMutex              accessList;		///< For use the list
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    QSemaphore          waitNewClockForSpeed,waitNewClockForSpeed2;
    volatile int		numberOfBlockCopied,numberOfBlockCopied2;		///< Multiple for count the number of block copied
    volatile int		multiplicatorForBigSpeed;	///< Multiple for count the number of block needed
    volatile int		MultiForBigSpeed;
    #endif
    QSemaphore          writeFull;
    volatile bool       writeFullBlocked;
    QSemaphore          isOpen;
    QSemaphore          pauseMutex;
    volatile bool		putInPause;
    QList<QByteArray>	theBlockList;		///< Store the block list
    uint64_t             lastGoodPosition;
    QByteArray          blockArray;		///< temp data for block writing, the data
    int64_t              bytesWriten;		///< temp data for block writing, the bytes writen
    int                 id;
    volatile bool       endDetected;
    uint64_t             startSize;
    bool                fakeMode;
    bool                needRemoveTheFile;
    bool                deletePartiallyTransferredFiles;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    volatile int        multiForBigSpeed;           ///< Multiple for count the number of block needed
    #endif

    #ifdef Q_OS_UNIX
    int to;
    #else
    HANDLE to;
    #endif
    INTERNALTYPEPATH file;
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
