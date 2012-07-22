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

void Factory::setResources(OptionInterface *,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityEngine,bool portableVersion)
{
	this->facilityEngine=facilityEngine;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
	Q_UNUSED(portableVersion);
}

QWidget * Factory::options()
{
	return NULL;
}

void Factory::resetOptions()
{
}

QIcon Factory::getIcon(const QString &fileName)
{
	if(fileName=="SystemTrayIcon/exit.png")
	{
		QIcon tempIcon=QIcon::fromTheme("application-exit");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/add.png")
	{
		QIcon tempIcon=QIcon::fromTheme("list-add");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/informations.png")
	{
		QIcon tempIcon=QIcon::fromTheme("help-about");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/options.png")
	{
		QIcon tempIcon=QIcon::fromTheme("applications-system");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	return QIcon(":/resources/"+fileName);
}

void Factory::newLanguageLoaded()
{
	emit reloadLanguage();
}
