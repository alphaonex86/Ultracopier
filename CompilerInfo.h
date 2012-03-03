/** \file CompilerInfo.h
\brief Define the compiler info
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

/// \def COMPILERINFO the string to identify the compiler
#if defined(Q_CC_GNU)
	#define COMPILERINFO QString("GCC build: ")+__DATE__+" "+__TIME__
#else
	#if defined(__DATE__) && defined(__TIME__)
		#define COMPILERINFO QString("Unknow compiler: ")+__DATE__+" "+__TIME__
	#else
		#define COMPILERINFO QString("Unknow compiler")
	#endif
#endif 
