/** \file QXzDecodeThread.cpp
\brief To have thread to decode the data
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "QXzDecodeThread.h"

QXzDecodeThread::QXzDecodeThread()
{
	moveToThread(this);
	DataToDecode=NULL;
	error=false;
}

QXzDecodeThread::~QXzDecodeThread()
{
	if(DataToDecode!=NULL)
		delete DataToDecode;
}

void QXzDecodeThread::setData(QByteArray data,quint64 maxSize)
{
	if(DataToDecode!=NULL)
		delete DataToDecode;
	DataToDecode=new QXzDecode(data,maxSize);
}

bool QXzDecodeThread::errorFound()
{
	return error;
}

QString QXzDecodeThread::errorString()
{
	return DataToDecode->errorString();
}

QByteArray QXzDecodeThread::decodedData()
{
	return DataToDecode->decodedData();
}

void QXzDecodeThread::run()
{
	error=!DataToDecode->decode();
	emit decodedIsFinish();
}

