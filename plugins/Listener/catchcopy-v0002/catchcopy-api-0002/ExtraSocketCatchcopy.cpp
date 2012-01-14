/** \file ExtraSocketCatchcopy.cpp
\brief Define the socket of catchcopy
\author alpha_one_x86
\version 0002
\date 2010 */

#include "ExtraSocketCatchcopy.h"

const QString ExtraSocketCatchcopy::pathSocket()
{
#ifdef Q_OS_UNIX
	return "advanced-copier-"+QString::number(getuid());
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
	return "advanced-copier-"+userName;
#endif
}
