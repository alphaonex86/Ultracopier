/** \file QXzDecodeThread.h
\brief To have thread to decode the data
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef QXZDECODETHREAD_H
#define QXZDECODETHREAD_H

#include <QThread>
#include "QXzDecode.h"

/// \brief to decode the xz via a thread
class QXzDecodeThread : public QThread
{
	Q_OBJECT
	public:
		QXzDecodeThread();
		~QXzDecodeThread();
		/// \brief to return if the error have been found
		bool errorFound();
		/// \brief to return the error string
		QString errorString();
		/// \brief to get the decoded data
		QByteArray decodedData();
		/// \brief to send the data
		void setData(QByteArray data,quint64 maxSize=0);
	protected:
		void run();
	private:
		/// \brief to have temporary storage
		QXzDecode *DataToDecode;
		bool error;
	signals:
		void decodedIsFinish();
};

#endif // QXZDECODETHREAD_H
