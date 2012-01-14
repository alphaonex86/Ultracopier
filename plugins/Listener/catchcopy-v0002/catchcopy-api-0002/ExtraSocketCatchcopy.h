/** \file ExtraSocketCatchcopy.h
\brief Define the socket of catchcopy
\author alpha_one_x86
\version 0002
\date 2010 */

#ifndef EXTRASOCKETCATCHCOPY_H
#define EXTRASOCKETCATCHCOPY_H

#include <QString>

#ifdef Q_OS_UNIX
	#include <unistd.h>
	#include <sys/types.h>
#else
	#include <windows.h>
#endif

class ExtraSocketCatchcopy
{
public:
	static const QString pathSocket();
};

#endif // EXTRASOCKETCATCHCOPY_H
