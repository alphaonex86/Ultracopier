/** \file ExtraSocket.h
\brief Define the socket of ultracopier
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "ExtraSocket.h"
#include <QByteArray>
#include <stdio.h>

std::string ExtraSocket::pathSocket(const std::string &name)
{
    std::string socketName;
#ifdef Q_OS_UNIX
    socketName=name+"-"+std::to_string(getuid());
#else
    QString userName;

    /* bad way for catchcopy compatibility
    char uname[1024];
    DWORD len=1023;
    if(GetUserNameA(uname, &len)!=FALSE)
        userName=toHex(uname);*/

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
    socketName=name+"-"+userName.toStdString();
#endif
    // Test-isolation hook (NOT a feature, NOT an #ifdef): when the optional environment
    // variable ULTRACOPIER_SOCKET_SUFFIX is set, append it so an isolated test instance
    // binds its own socket (e.g. "ultracopier-1000-test") and never hijacks the user's real
    // single-instance / CLI-forward socket (their file-manager copy/paste keeps working
    // while the test suite runs). The variable is UNSET for every real user, so the shipping
    // binary stays byte-for-byte identical to today. qgetenv() (not qEnvironmentVariable,
    // which is Qt 5.10+) keeps this compiling on Qt 5.6.3 / mingw 4.9.2 too.
    const QByteArray socketSuffix=qgetenv("ULTRACOPIER_SOCKET_SUFFIX");
    if(!socketSuffix.isEmpty())
        socketName+="-"+socketSuffix.toStdString();
    return socketName;
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
        // unsigned char: a signed char >=0x80 sign-extends so "%.2x" prints 8 hex
        // digits while p advances by 4 -> heap overflow and an inconsistent socket
        // name for non-ASCII user names. (This is Ultracopier's own local socket,
        // not the catchcopy pipe, but it had the identical bug.)
        sprintf(p, "%.2x00", (unsigned char)str[i]);
        p+=4;
    }
    *p=0;
    return sz;
}
