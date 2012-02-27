/** \file GlobalClass.cpp
\brief Define the class for all the class different of ResourcesManager, DebugEngine, ThemesManager, OptionEngine can have object of this class at the global scope
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "GlobalClass.h"

/// \brief The constructor where set the global variable to the good pointer
GlobalClass::GlobalClass()
{
	//load the debug engine, themes and resources, after reused in all the program and all class derive from GlobalClass
	#ifdef ULTRACOPIER_DEBUG
	debug_engine_instance=DebugEngine::getInstance();
	#endif // ULTRACOPIER_DEBUG
	resources=ResourcesManager::getInstance();
	plugins=PluginsManager::getInstance();
	options=OptionEngine::getInstance();
	themes=ThemesManager::getInstance();
	languages=LanguagesManager::getInstance();
}

/// \brief The destructor where destroy the variable if the object no more longer used
GlobalClass::~GlobalClass()
{
	//destroy the resources variable if not more used
	LanguagesManager::destroyInstanceAtTheLastCall();
	ThemesManager::destroyInstanceAtTheLastCall();
	OptionEngine::destroyInstanceAtTheLastCall();
	PluginsManager::destroyInstanceAtTheLastCall();
	ResourcesManager::destroyInstanceAtTheLastCall();
	#ifdef ULTRACOPIER_DEBUG
	DebugEngine::destroyInstanceAtTheLastCall();
	#endif // ULTRACOPIER_DEBUG
}




