/** \file LogThread.h
\brief The thread to do the log but not block the main thread
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <QString>
#include <QDateTime>
#include <QVariant>

#include "GlobalClass.h"
#include "Environment.h"
#include "StructEnumDefinition.h"

/** \brief Log all the user oriented activity

It use thread based storage to prevent gui thread freeze on log file writing when is out of the disk buffer. That's allow to async the event.
*/
class LogThread : public QThread, public GlobalClass
{
    Q_OBJECT
public:
	explicit LogThread();
	 ~LogThread();
signals:
	void newData();
public slots:
	/** method called when new transfer is started */
	void newTransferStart(ItemOfCopyList);
	/** method called when transfer is stopped */
	void newTransferStop(quint64 id);
	/** method called when new error is occurred */
	void error(QString path,quint64 size,QDateTime mtime,QString error);
	/** method called when the log file need be created */
	void openLogs();
	/** method called when the log file need be closed */
	void closeLogs();
	/** method called when one folder is removed */
	void rmPath(QString path);
	/** method called when one folder is created */
	void mkPath(QString path);
private slots:
	void realDataWrite();
	void newOptionValue(QString group,QString name,QVariant value);
private:
	QString data;
	QString transfer_format;
	QString error_format;
	QString folder_format;
	QFile log;
	QString replaceBaseVar(QString text);
	QMutex mutex;
	bool sync;
protected:
	void run();
};

#endif // LOGTHREAD_H
