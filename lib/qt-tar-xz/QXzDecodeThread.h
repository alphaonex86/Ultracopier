#ifndef QXZDECODETHREAD_H
#define QXZDECODETHREAD_H

#include <QThread>
#include "QXzDecode.h"

class QXzDecodeThread : public QThread
{
	Q_OBJECT
	public:
		QXzDecodeThread();
		~QXzDecodeThread();
		bool errorFound();
		QString errorString();
		QByteArray decodedData();
		void setData(QByteArray data,quint64 maxSize=0);
	protected:
		void run();
	private:
		QXzDecode *DataToDecode;
		bool error;
	signals:
		void decodedIsFinish();
};

#endif // QXZDECODETHREAD_H
