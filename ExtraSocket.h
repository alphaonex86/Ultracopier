/** \file ExtraSocket.h
\brief Define the socket for ultracopier
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef EXTRASOCKET_H
#define EXTRASOCKET_H

#include <QString>

#ifdef Q_OS_UNIX
	#include <unistd.h>
	#include <sys/types.h>
#else
	#include <windows.h>
#endif

class ExtraSocket
{
public:
	/** \brief class to return always the same socket resolution */
	static QString pathSocket(QString name);
};

#endif // EXTRASOCKET_H
