/** \file GlobalClass.h
\brief Define the class for all the class different of ResourcesManager, DebugEngine, ThemesManager, OptionEngine have object of this class at the global scope
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef GLOBAL_CLASS_H
#define GLOBAL_CLASS_H

#include "Environment.h"
#include "ThemesManager.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"
#include "LanguagesManager.h"
#include "PluginsManager.h"

#include "interface/OptionInterface.h"

/// \brief Define the class for the debug and global variables heritage
class GlobalClass
{
	public:
		/// \brief The constructor where set the global variable to the good pointer
		GlobalClass();
		/// \brief The destructor where destroy the variable if the unique instance is no more longer used
		~GlobalClass();
	protected:
		//for the themes
		ThemesManager *themes;
		//for the resources linked with the themes
		ResourcesManager *resources;
		//for the options
		OptionEngine *options;
		//for the languages
		LanguagesManager *languages;
		//for the plugins
		PluginsManager *plugins;
		#ifdef ULTRACOPIER_DEBUG
		DebugEngine *debug_engine_instance;
		#endif // ULTRACOPIER_DEBUG
};

#endif // GLOBAL_CLASS_H
