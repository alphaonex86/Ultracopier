/** \file PlatformMacro.h
\brief Define the macro for the platform
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>

#ifndef PLATFORM_MACRO_H
#define PLATFORM_MACRO_H

//windows
#if defined(Q_OS_WIN32)
	#if defined(_M_X64) //_WIN64
		//windows 64Bits
		#define ULTRACOPIER_PLATFORM_NAME tr("Windows 64Bits")
		#define ULTRACOPIER_PLATFORM_CODE "windows-x86_64"
	#else
		//windows 32Bits
		#define ULTRACOPIER_PLATFORM_NAME tr("Windows 32Bits")
		#define ULTRACOPIER_PLATFORM_CODE "windows-x86"
	#endif
#elif defined(Q_OS_MAC)
	//Mac OS X
	#define ULTRACOPIER_PLATFORM_NAME tr("Mac OS X")
	#define ULTRACOPIER_PLATFORM_CODE "mac-os-x"
#elif defined(Q_OS_LINUX)
	#if defined(__i386__)
		//linux pc i386
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux pc i386")
		#define ULTRACOPIER_PLATFORM_CODE "linux-i386-pc"
	#elif  defined(__i486__)
		//linux pc i486
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux pc i486")
		#define ULTRACOPIER_PLATFORM_CODE "linux-i486-pc"
	#elif  defined(__i586__)
		//linux pc i586
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux pc i586")
		#define ULTRACOPIER_PLATFORM_CODE "linux-i586-pc"
	#elif  defined(__i686__)
		//linux pc i686
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux pc i686")
		#define ULTRACOPIER_PLATFORM_CODE "linux-i686-pc"
	#elif defined(__x86_64__)
		//linux pc 64Bits
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux pc 64Bits")
		#define ULTRACOPIER_PLATFORM_CODE "linux-x86_64-pc"
	#else
		//linux unknow
		#define ULTRACOPIER_PLATFORM_NAME tr("Linux unknow platform")
		#define ULTRACOPIER_PLATFORM_CODE "linux-unknow-pc"
	#endif
#else
	//unknow
	#define ULTRACOPIER_PLATFORM_NAME tr("Unknow platform")
	#define ULTRACOPIER_PLATFORM_CODE "unknow"
#endif

#endif // PLATFORM_MACRO_H
 
