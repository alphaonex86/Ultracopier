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

class WriteThread : public QThread
{
	Q_OBJECT
public:
	explicit WriteThread();
	~WriteThread();
protected:
	void run();
public:
	void open(const QString &name,const quint64 &startSize);
	QString errorString();
	void stop();
	bool write(const QByteArray &data);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	//to set the id
	void setId(int id);
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
	void postOperation();
	void flushBuffer();
	void endIsDetected();
	void reopen();
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
private slots:
	bool internalOpen();
	void internalWrite();
	void internalClose(bool emitSignal=true);
	void internalReopen();
	void internalEndOfFile();
        void internalFlushAndSeekToZero();
};

#endif // WRITETHREAD_H
