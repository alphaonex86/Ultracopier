/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>

#include "factory.h"

PluginInterface_Themes * Factory::getInstance()
{
	PluginInterface_Themes * newInterface=new InterfacePlugin();
	connect(this,SIGNAL(reloadLanguage()),newInterface,SLOT(newLanguageLoaded()));
	return newInterface;
}

void Factory::setResources(OptionInterface * options,QString writePath,QString pluginPath,FacilityInterface * facilityEngine)
{
	Q_UNUSED(options)
	Q_UNUSED(writePath)
	Q_UNUSED(pluginPath)
	Q_UNUSED(facilityEngine)
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
