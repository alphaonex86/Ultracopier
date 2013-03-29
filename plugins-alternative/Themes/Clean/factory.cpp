/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "factory.h"

PluginInterface_Themes * ThemesFactory::getInstance()
{
    InterfacePlugin * newInterface=new InterfacePlugin(facilityEngine);
    connect(this,&ThemesFactory::reloadLanguage,newInterface,&InterfacePlugin::newLanguageLoaded);
    return newInterface;
}

void ThemesFactory::setResources(OptionInterface * options, const QString &writePath, const QString &pluginPath, FacilityInterface * facilityInterface, const bool &portableVersion)
{
    Q_UNUSED(options)
    Q_UNUSED(writePath)
    Q_UNUSED(pluginPath)
    this->facilityEngine=facilityInterface;
    Q_UNUSED(portableVersion)
}

QWidget * ThemesFactory::options()
{
    return NULL;
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
    return QIcon(":/Themes/Clean/resources/"+fileName);
}

void ThemesFactory::resetOptions()
{
}

void ThemesFactory::newLanguageLoaded()
{
    emit reloadLanguage();
}
