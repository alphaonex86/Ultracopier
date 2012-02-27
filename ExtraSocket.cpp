/** \file ExtraSocket.h
\brief Define the socket of ultracopier
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "ExtraSocket.h"

QString ExtraSocket::pathSocket(QString name)
{
#ifdef Q_OS_UNIX
	return name+"-"+QString::number(getuid());
#else
	QString userName;
	DWORD size=0;
	if(GetUserNameW(NULL,&size) || (GetLastError()!=ERROR_INSUFFICIENT_BUFFER))
	{
	}
	else
	{
		WCHAR * userNameW=new WCHAR[size];
		if(GetUserNameW(userNameW,&size))
		{
			userName.fromWCharArray(userNameW,size*2);
			userName=QString(QByteArray((char*)userNameW,size*2-2).toHex());
		}
		delete userNameW;
	}
	return name+"-"+userName;
#endif
}
