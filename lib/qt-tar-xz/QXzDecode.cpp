/** \file QXzDecode.cpp
\brief To decompress a xz stream
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "QXzDecode.h"
#include <QDebug>
extern "C" {
#include "xz.h"
}

static uint8_t in[BUFSIZ];
static uint8_t out[BUFSIZ];

QXzDecode::QXzDecode(QByteArray data,quint64 maxSize)
{
    error="Unknow error";
    this->data=data;
    this->maxSize=maxSize;
    isDecoded=false;
}

bool QXzDecode::decodeStream(QDataStream *stream_xz_decode_in,QDataStream *stream_xz_decode_out)
{
    isDecoded=false;
    struct xz_buf b;
    struct xz_dec *s;
    enum xz_ret ret;

    xz_crc32_init();

    /*
     * Support up to 64 MiB dictionary. The actually needed memory
     * is allocated once the headers have been parsed.
     */
    s = xz_dec_init(XZ_DYNALLOC, 1 << 26);
    if (s == NULL) {
        error=tr("Memory allocation failed");
        xz_dec_end(s);
        return isDecoded;
    }

    b.in = in;
    b.in_pos = 0;
    b.in_size = 0;
    b.out = out;
    b.out_pos = 0;
    b.out_size = BUFSIZ;

    while (true) {
        //input of data
        if (b.in_pos == b.in_size) {
            b.in_size = stream_xz_decode_in->readRawData((char *)in,sizeof(in));
            b.in_pos = 0;
        }

        ret = xz_dec_run(s, &b);

        //output of data
        if (b.out_pos == sizeof(out))
        {
            if (static_cast<size_t>(stream_xz_decode_out->writeRawData((char *)out,static_cast<int>(b.out_pos))) != b.out_pos)
            {
                error=tr("Write error");
                xz_dec_end(s);
                return isDecoded;
            }
            b.out_pos = 0;
        }

        if (ret == XZ_OK)
            continue;

#ifdef XZ_DEC_ANY_CHECK
        if (ret == XZ_UNSUPPORTED_CHECK) {
            continue;
        }
#endif

        if (static_cast<size_t>(stream_xz_decode_out->writeRawData((char *)out,static_cast<int>(b.out_pos))) != b.out_pos)
        {
            error=tr("Write error");
            xz_dec_end(s);
            return isDecoded;
        }

        switch (ret) {
        case XZ_STREAM_END:
            xz_dec_end(s);
            isDecoded=true;
            return isDecoded;
        case XZ_MEM_ERROR:
            error=tr("Memory allocation failed");
            xz_dec_end(s);
            return isDecoded;
        case XZ_MEMLIMIT_ERROR:
            error=tr("Memory usage limit reached");
            xz_dec_end(s);
            return isDecoded;
        case XZ_FORMAT_ERROR:
            error=tr("Not a .xz file");
            xz_dec_end(s);
            return isDecoded;
        case XZ_OPTIONS_ERROR:
            error=tr("Unsupported options in the .xz headers");
            xz_dec_end(s);
            return isDecoded;
        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            error=tr("The file is corrupted");
            xz_dec_end(s);
            return isDecoded;
        default:
            error=tr("Bug!");
            xz_dec_end(s);
            return isDecoded;
        }
    }
}

bool QXzDecode::decode()
{
    if(data.size() < 32) // check the minimal size
        error=tr("The input data is too short");
    QByteArray outputData;
    QDataStream stream_xz_decode_in(&data,QIODevice::ReadOnly);
    QDataStream stream_xz_decode_out(&outputData,QIODevice::WriteOnly);
    int returnVal=decodeStream(&stream_xz_decode_in,&stream_xz_decode_out);
    data=outputData;
    return returnVal;
}

QByteArray QXzDecode::decodedData()
{
    if(isDecoded)
        return data;
    else
        return QByteArray();
}

QString QXzDecode::errorString()
{
    return error;
}


