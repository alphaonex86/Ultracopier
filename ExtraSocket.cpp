/** \file ExtraSocket.h
\brief Define the socket of ultracopier
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "ExtraSocket.h"
#include <QByteArray>
#include <stdio.h>

QString ExtraSocket::pathSocket(const QString &name)
{
#ifdef Q_OS_UNIX
    return name+QStringLiteral("-")+QString::number(getuid());
#else
    QString userName;

    /* bad way for catchcopy compatibility
    char uname[1024];
    DWORD len=1023;
    if(GetUserNameA(uname, &len)!=FALSE)
        userName=QString::fromLatin1(toHex(uname));*/

    QChar charTemp;
    DWORD size=255;
    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
    {
        QByteArray tempArray;
        userName=QString::fromWCharArray(userNameW,size-1);
        int index=0;
        while(index<userName.size())
        {
            tempArray+=userName.at(index).cell();
            tempArray+=userName.at(index).row();
            index++;
        }
        userName=tempArray.toHex();
    }
    delete userNameW;
    return name+QStringLiteral("-")+userName;
#endif
}

// Dump UTF16 (little endian)
char * ExtraSocket::toHex(const char *str)
{
    char *p, *sz;
    size_t len;
    if (str==NULL)
        return NULL;
    len= strlen(str);
    p = sz = (char *) malloc((len+1)*4);
    for (size_t i=0; i<len; i++)
    {
        sprintf(p, "%.2x00", str[i]);
        p+=4;
    }
    *p=0;
    return sz;
}
