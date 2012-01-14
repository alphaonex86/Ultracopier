#ifndef QTARDECODE_H
#define QTARDECODE_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QStringList>

class QTarDecode : public QObject
{
	public:
		QTarDecode();
		QStringList getFileList();
		QList<QByteArray> getDataList();
		bool decodeData(QByteArray data);
		QString errorString();
	private:
		QStringList fileList;
		QList<QByteArray> dataList;
		QString error;
		void setErrorString(QString error);
};

#endif // QTARDECODE_H
