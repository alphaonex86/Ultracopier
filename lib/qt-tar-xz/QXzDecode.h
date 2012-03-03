/** \file QXzDecode.h
\brief To decompress a xz stream
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef QXZDECODE_H
#define QXZDECODE_H

#include <QObject>
#include <QByteArray>
#include <QDataStream>

//comment this to check integrity of compressed file, compressed via: xz --check=crc32 YourFile
//#define XZ_DEC_ANY_CHECK

/// \brief The decode class for the xz stream
class QXzDecode : public QObject
{
	public:
		/** \brief create the object to decode it
		 * \param data the compressed data
		 * \param maxSize the max size
		 * **/
		QXzDecode(QByteArray data,quint64 maxSize=0);
		/// \brief lunch the decode
		bool decode();
		/// \brief the error string
		QString errorString();
		/// \brief the un-compressed data
		QByteArray decodedData();
		/// \brief un-compress the data by stream
		bool decodeStream(QDataStream *stream_xz_decode_in,QDataStream *stream_xz_decode_out);
	private:
		QByteArray data;
		QString error;
		bool isDecoded;
		quint64 maxSize;
};

#endif // QXZDECODE_H
