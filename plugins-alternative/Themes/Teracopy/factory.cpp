/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "factory.h"

PluginInterface_Themes * Factory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    PluginInterface_Themes * newInterface=new InterfacePlugin(facilityEngine);
    connect(newInterface,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
    connect(this,SIGNAL(reloadLanguage()),newInterface,SLOT(newLanguageLoaded()));
    return newInterface;
}

void Factory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion)
{
    this->facilityEngine=facilityEngine;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
    Q_UNUSED(portableVersion);
    Q_UNUSED(options);
    Q_UNUSED(facilityInterface);
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/add.png")
    {
        QIcon tempIcon=QIcon::fromTheme("list-add");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/informations.png")
    {
        QIcon tempIcon=QIcon::fromTheme("help-about");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/options.png")
    {
        QIcon tempIcon=QIcon::fromTheme("applications-system");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    return QIcon(":/resources/"+fileName);
}

void Factory::newLanguageLoaded()
{
    emit reloadLanguage();
}
