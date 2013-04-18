/** \file ExtraSocket.h
\brief Define the socket for ultracopier
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef EXTRASOCKET_H
#define EXTRASOCKET_H

#include <QString>

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
#else
    #include <windows.h>
#endif

/** \brief class to have general socket options */
class ExtraSocket
{
public:
    /** \brief class to return always the same socket resolution */
    static QString pathSocket(const QString &name);
    static char * toHex(const char *str);
};

#endif // EXTRASOCKET_H
