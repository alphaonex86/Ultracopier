/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "factory.h"

QRegularExpression ThemesFactory::slashEnd;
QRegularExpression ThemesFactory::isolateName;

ThemesFactory::ThemesFactory()
{
    slashEnd=QRegularExpression("/$");
    isolateName=QRegularExpression("^.*/([^/]+)$");
}

PluginInterface_Themes * ThemesFactory::getInstance()
{
    Themes * newInterface=new Themes(facilityEngine);
    connect(this,&ThemesFactory::reloadLanguage,newInterface,&Themes::newLanguageLoaded);
    return newInterface;
}

void ThemesFactory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityEngine,const bool &portableVersion)
{
    Q_UNUSED(options)
    Q_UNUSED(writePath)
    Q_UNUSED(pluginPath)
    this->facilityEngine=facilityEngine;
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
    return QIcon(":/resources/"+fileName);
}

void ThemesFactory::resetOptions()
{
}

void ThemesFactory::newLanguageLoaded()
{
    emit reloadLanguage();
}
