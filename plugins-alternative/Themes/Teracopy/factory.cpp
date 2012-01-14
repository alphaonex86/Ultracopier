/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>

#include "factory.h"

PluginInterface_Themes * Factory::getInstance()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	PluginInterface_Themes * newInterface=new InterfacePlugin(facilityEngine);
	connect(newInterface,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(this,SIGNAL(reloadLanguage()),newInterface,SLOT(newLanguageLoaded()));
	return newInterface;
}

void Factory::setResources(OptionInterface *,QString writePath,QString pluginPath,FacilityInterface * facilityEngine)
{
	this->facilityEngine=facilityEngine;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
}

QWidget * Factory::options()
{
	return NULL;
}

void Factory::resetOptions()
{
}

void Factory::newLanguageLoaded()
{
	emit reloadLanguage();
}

Q_EXPORT_PLUGIN2(interface, Factory);

