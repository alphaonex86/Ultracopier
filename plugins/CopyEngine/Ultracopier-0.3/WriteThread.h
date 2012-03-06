/** \file WriteThread.h
\brief Thread changed to open/close and write the destination file
\author alpha_one_x86
\version 0.3
\date 2011 */

#ifndef WRITETHREAD_H
#define WRITETHREAD_H

#include <QThread>
#include <QByteArray>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QSemaphore>

#include "Environment.h"
#include "StructEnumDefinition_CopyEngine.h"

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
	void open(const QString &name,const quint64 &startSize);
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
		Close=3
	};
	WriteStat stat;
	#endif
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
signals:
	void error();
	void opened();
	void reopened();
	void writeIsStarted();
	void writeIsStopped();
        void flushedAndSeekedToZero();
	void closed();
        //internal signals
        void internalStartOpen();
	void internalStartReopen();
	void internalStartWrite();
	void internalStartClose();
	void internalStartEndOfFile();
        void internalStartFlushAndSeekToZero();
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
private:
	QString			name;
	QString			errorString_internal;
	QFile			file;
        volatile bool		stopIt;
	QMutex			accessList;		///< For use the list
	QSemaphore		freeBlock;
	QSemaphore		isOpen;
	QList<QByteArray>	theBlockList;		///< Store the block list
	quint64			CurentCopiedSize;
	QByteArray		blockArray;		///< temp data for block writing, the data
	qint64			bytesWriten;		///< temp data for block writing, the bytes writen
	qint64			lastGoodPosition;
	int			id;
	bool			endDetected;
	quint64			startSize;
	QSemaphore		*mkpathTransfer;
private slots:
	bool internalOpen();
	void internalWrite();
	void internalClose(bool emitSignal=true);
	void internalReopen();
	void internalEndOfFile();
        void internalFlushAndSeekToZero();
};

#endif // WRITETHREAD_H
