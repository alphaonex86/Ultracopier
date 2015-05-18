/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "factory.h"

PluginInterface_Themes * ThemesFactory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Themes * newInterface=new Themes(facilityEngine);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(newInterface,&Themes::debugInformation,this,&PluginInterface_ThemesFactory::debugInformation);
    #endif
    connect(this,&ThemesFactory::reloadLanguage,newInterface,&Themes::newLanguageLoaded);
    return newInterface;
}

void ThemesFactory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion)
{
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    this->facilityEngine=facilityInterface;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
    Q_UNUSED(portableVersion);
    Q_UNUSED(options);
}

QWidget * ThemesFactory::options()
{
    return NULL;
}

void ThemesFactory::resetOptions()
{
}

QIcon ThemesFactory::getIcon(const QString &fileName) const
{
    if(fileName=="SystemTrayIcon/exit.png")
    {
        QIcon tempIcon=QIcon::fromTheme("application-exit");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    if(fileName=="SystemTrayIcon/add.png")
    {
        QIcon tempIcon=QIcon::fromTheme("list-add");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    if(fileName=="SystemTrayIcon/informations.png")
    {
        QIcon tempIcon=QIcon::fromTheme("help-about");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    if(fileName=="SystemTrayIcon/options.png")
    {
        QIcon tempIcon=QIcon::fromTheme("applications-system");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    return QIcon(":/Themes/Teracopy/resources/"+fileName);
}

void ThemesFactory::newLanguageLoaded()
{
    emit reloadLanguage();
}
