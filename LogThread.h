#ifndef LOGTHREAD_H
#define LOGTHREAD_H

#include <QThread>
#include <QString>
#include <QDateTime>
#include <QVariant>

#include "GlobalClass.h"
#include "Environment.h"
#include "StructEnumDefinition.h"


class LogThread : public QThread, public GlobalClass
{
    Q_OBJECT
public:
	explicit LogThread();
	 ~LogThread();
signals:
	void newData();
public slots:
	void newTransferStart(ItemOfCopyList);
	void newTransferStop(quint64 id);
	void error(QString path,quint64 size,QDateTime mtime,QString error);
	void openLogs();
	void closeLogs();
	void rmPath(QString path);
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
protected:
	void run();
};

#endif // LOGTHREAD_H
