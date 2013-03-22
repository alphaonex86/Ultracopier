/** \file WriteThread.h
\brief Thread changed to open/close and write the destination file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef WRITETHREAD_H
#define WRITETHREAD_H

#include <QThread>
#include <QByteArray>
#include <QString>
#include <QMutex>
#include <QSemaphore>
#include <QCryptographicHash>

#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"
#include "AvancedQFile.h"

/// \brief Thread changed to open/close and write the destination file
class WriteThread : public QThread
{
    Q_OBJECT
public:
    explicit WriteThread();
    ~WriteThread();
    /// \brief to have semaphore to do mkpath one by one
    void setMkpathTransfer(QSemaphore *mkpathTransfer);
protected:
    void run();
public:
    /// \brief open the destination to open it
    void open(const QFileInfo &file,const quint64 &startSize,const bool &buffer,const int &numberOfBlock,const bool &sequential);
    /// \brief to return the error string
    QString errorString();
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
        Read=5,
        Checksum=6
    };
    WriteStat stat;
    #endif
    /// \brief do the fake open
    void fakeOpen();
    /// \brief do the fake writeIsStarted
    void fakeWriteIsStarted();
    /// \brief do the fake writeIsStopped
    void fakeWriteIsStopped();
    /// do the checksum
    void startCheckSum();
    /// \brief set block size in KB
    bool setBlockSize(const int blockSize);
    /// \brief get the last good position
    qint64 getLastGoodPosition();
    /// \brief buffer is empty
    bool bufferIsEmpty();
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    /// \brief set the current max speed in KB/s
    void setMultiForBigSpeed(const int &multiForBigSpeed);
    #endif
    void pause();
    void resume();
    void reemitStartOpen();
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
    /// do the checksum
    void checkSum();
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    /// \brief executed at regular interval to do a speed throling
    void timeOfTheBlockCopyFinished();

    void resumeNotStarted();
signals:
    void error();
    void opened();
    void reopened();
    void writeIsStarted();
    void writeIsStopped();
    void flushedAndSeekedToZero();
    void closed();
    void checksumFinish(const QByteArray&);
    //internal signals
    void internalStartOpen();
    void internalStartChecksum();
    void internalStartReopen();
    void internalStartWrite();
    void internalStartClose();
    void internalStartEndOfFile();
    void internalStartFlushAndSeekToZero();
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
private:
    QString             errorString_internal;
    AvancedQFile		file;
    volatile bool		stopIt;
    volatile bool       postOperationRequested;
    volatile int		blockSize;//only used in checksum
    int                 numberOfBlock;
    QMutex              accessList;		///< For use the list
    static QMultiHash<QString,WriteThread *> writeFileList;
    static QMutex       writeFileListMutex;
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
    quint64             lastGoodPosition;
    QByteArray          blockArray;		///< temp data for block writing, the data
    qint64              bytesWriten;		///< temp data for block writing, the bytes writen
    int                 id;
    volatile bool       endDetected;
    quint64             startSize;
    QSemaphore          *mkpathTransfer;
    bool                fakeMode;
    bool                buffer;
    bool                needRemoveTheFile;
    volatile bool       sequential;
    bool                deletePartiallyTransferredFiles;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    volatile int        multiForBigSpeed;           ///< Multiple for count the number of block needed
    #endif
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
