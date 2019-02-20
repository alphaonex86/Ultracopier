/** \file CompilerInfo.h
\brief Define the compiler info
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>

/// \def COMPILERINFO the string to identify the compiler
#if defined(Q_CC_GNU)
    #define COMPILERINFO std::string("GCC ")+std::to_string(__GNUC__)+"."+std::to_string(__GNUC_MINOR__)+"."+std::to_string(__GNUC_PATCHLEVEL__)
#else
    #define COMPILERINFO std::string("Unknown compiler: ")
#endif
