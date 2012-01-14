#ifndef QXZDECODE_H
#define QXZDECODE_H

#include <QObject>
#include <QByteArray>
#include <QDataStream>

//comment this to check integrity of compressed file, compressed via: xz --check=crc32 YourFile
//#define XZ_DEC_ANY_CHECK

class QXzDecode : public QObject
{
	public:
		QXzDecode(QByteArray data,quint64 maxSize=0);
		bool decode();
		QString errorString();
		QByteArray decodedData();
		bool decodeStream(QDataStream *stream_xz_decode_in,QDataStream *stream_xz_decode_out);
	private:
		QByteArray data;
		QString error;
		bool isDecoded;
		quint64 maxSize;
};

#endif // QXZDECODE_H
