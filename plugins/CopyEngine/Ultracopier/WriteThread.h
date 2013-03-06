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
    void open(const QFileInfo &file,const quint64 &startSize,const bool &buffer);
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
    QString			errorString_internal;
    AvancedQFile		file;
    volatile bool		stopIt;
    volatile bool       postOperationRequested;
    volatile int		blockSize;//in Bytes
    QMutex			accessList;		///< For use the list
    QSemaphore		waitNewClockForSpeed;
    volatile int		numberOfBlockCopied;		///< Multiple for count the number of block copied
    volatile int		multiplicatorForBigSpeed;	///< Multiple for count the number of block needed
    volatile int		MultiForBigSpeed;
    QSemaphore		freeBlock;
    QSemaphore		isOpen;
    volatile bool		putInPause;
    QList<QByteArray>	theBlockList;		///< Store the block list
    quint64			CurentCopiedSize;
    QByteArray		blockArray;		///< temp data for block writing, the data
    qint64			bytesWriten;		///< temp data for block writing, the bytes writen
    qint64			lastGoodPosition;
    int			id;
    bool			endDetected;
    quint64			startSize;
    QSemaphore		*mkpathTransfer;
    bool			fakeMode;
    bool			buffer;
    bool			needRemoveTheFile;
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
