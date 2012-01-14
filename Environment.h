/** \file Environment.h
\brief Define the environment variable and global function
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "Variable.h"
/// \brief The global include
#include "StructEnumDefinition.h"
#include "StructEnumDefinition_UltracopierSpecific.h"
#include "PlatformMacro.h"
#include "DebugEngineMacro.h"

#ifdef ULTRACOPIER_VERSION_PORTABLE
	#define ULTRACOPIER_VERSION_PORTABLE_BOOL true
#else
	#define ULTRACOPIER_VERSION_PORTABLE_BOOL false
#endif
