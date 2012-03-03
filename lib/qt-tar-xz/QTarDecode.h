/** \file QTarDecode.h
\brief To read a tar data block
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef QTARDECODE_H
#define QTARDECODE_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QStringList>

/// \brief read the raw tar data, and organize it into data structure
class QTarDecode : public QObject
{
	public:
		QTarDecode();
		/// \brief to get the file list
		QStringList getFileList();
		/// \brief to get the data of the file
		QList<QByteArray> getDataList();
		/// \brief to pass the raw tar data
		bool decodeData(QByteArray data);
		/// \brief to return error string
		QString errorString();
	private:
		QStringList fileList;
		QList<QByteArray> dataList;
		QString error;
		void setErrorString(QString error);
};

#endif // QTARDECODE_H
