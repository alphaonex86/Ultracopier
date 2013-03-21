/** \file ExtraSocket.h
\brief Define the socket of ultracopier
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "ExtraSocket.h"

QString ExtraSocket::pathSocket(const QString &name)
{
#ifdef Q_OS_UNIX
    return name+"-"+QString::number(getuid());
#else
    QString userName;
    DWORD size=255;
    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
        userName=QString(QByteArray((char*)userNameW,size*2-2).toHex());
    delete userNameW;
    return name+"-"+userName;
#endif
}
