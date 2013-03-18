/** \file ThemesManager.cpp
\brief Define the class for manage and load the themes
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QString>
#include <QFile>
#include <QMessageBox>

#include "ThemesManager.h"
#include "PluginsManager.h"
#include "LanguagesManager.h"

#define ULTRACOPIER_DEFAULT_STYLE "Oxygen"

/// \todo load each plugin to have their options
/// \todo get the current themes instance

/// \brief Create the manager and load the defaults variables
ThemesManager::ThemesManager()
{
    //load the debug engine as external part because ThemesManager is base class
    stopIt=false;
    currentPluginIndex=-1;

    //connect the plugin management
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,   &ThemesManager::previouslyPluginAdded,			this,&ThemesManager::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,			this,&ThemesManager::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,		this,&ThemesManager::onePluginWillBeRemoved,Qt::DirectConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeUnloaded,		this,&ThemesManager::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,			this,&ThemesManager::allPluginIsLoaded,Qt::QueuedConnection);
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Themes);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();

    //do the options
    QList<QPair<QString, QVariant> > KeysList;
    KeysList.append(qMakePair(QString("Ultracopier_current_theme"),QVariant(ULTRACOPIER_DEFAULT_STYLE)));
    OptionEngine::optionEngine->addOptionGroup("Themes",KeysList);

    //load the default and current themes path
    defaultStylePath=":/Themes/"+QString(ULTRACOPIER_DEFAULT_STYLE)+"/";
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Default style: "+defaultStylePath);
    currentStylePath=defaultStylePath;
    connect(OptionEngine::optionEngine,            &OptionEngine::newOptionValue,	this,		&ThemesManager::newOptionValue,Qt::QueuedConnection);
    connect(LanguagesManager::languagesManager,	&LanguagesManager::newLanguageLoaded,		&facilityEngine,&FacilityEngine::retranslate,Qt::QueuedConnection);
}

/// \brief Destroy the themes manager
ThemesManager::~ThemesManager()
{
    stopIt=true;
}

void ThemesManager::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Themes)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+plugin.name);
    PluginsAvailableThemes newPlugin;
    newPlugin.plugin=plugin;
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    PluginInterface_ThemesFactory *factory;
    QObjectList objectList=QPluginLoader::staticInstances();
    int index=0;
    QObject *pluginObject;
    while(index<objectList.size())
    {
        pluginObject=objectList.at(index);
        factory = qobject_cast<PluginInterface_ThemesFactory *>(pluginObject);
        if(factory!=NULL)
            break;
        index++;
    }
    if(index==objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("static themes not found"));
        emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
        emit theThemeIsReloaded();
        return;
    }
    newPlugin.pluginLoader=NULL;
    #else
    QPluginLoader *pluginLoader=new QPluginLoader(newPlugin.plugin.path+QDir::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface"));
    QObject *pluginInstance = pluginLoader->instance();
    if(pluginInstance==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unable to load the plugin %1: %2").arg(newPlugin.plugin.path+QDir::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface")).arg(pluginLoader->errorString()));
        pluginLoader->unload();
        emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
        emit theThemeIsReloaded();
        return;
    }
    PluginInterface_ThemesFactory *factory = qobject_cast<PluginInterface_ThemesFactory *>(pluginInstance);
    if(factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("unable to cast the plugin: %1").arg(pluginLoader->errorString()));
        pluginLoader->unload();
        emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
        emit theThemeIsReloaded();
        return;
    }
    //check if found
    int indexTemp=0;
    while(indexTemp<pluginList.size())
    {
        if(pluginList.at(indexTemp).factory==factory)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Plugin already found, current: %1, conflit plugin: %2, name: %3")
            .arg(newPlugin.plugin.path+QDir::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface"))
            .arg(pluginList.at(indexTemp).plugin.path+QDir::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface"))
            .arg(newPlugin.plugin.name)
            );
            pluginLoader->unload();
            emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
            emit theThemeIsReloaded();
            return;
        }
        indexTemp++;
    }
    newPlugin.pluginLoader=pluginLoader;
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("preload: %1, at the index: %2").arg(newPlugin.plugin.name).arg(QString::number(pluginList.size())));
    #ifdef ULTRACOPIER_DEBUG
    connect(factory,&PluginInterface_ThemesFactory::debugInformation,this,&ThemesManager::debugInformation,Qt::QueuedConnection);
    #endif // ULTRACOPIER_DEBUG
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,factory,&PluginInterface_ThemesFactory::newLanguageLoaded);
    newPlugin.factory=factory;

    newPlugin.options=new LocalPluginOptions("Themes-"+newPlugin.plugin.name);
    newPlugin.factory->setResources(newPlugin.options,newPlugin.plugin.writablePath,newPlugin.plugin.path,&facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    currentStylePath=newPlugin.plugin.path;
    pluginList << newPlugin;
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        allPluginIsLoaded();
    emit newThemeOptions(newPlugin.plugin.name,newPlugin.factory->options(),true,true);
    emit theThemeIsReloaded();
    return;
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void ThemesManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(stopIt)
        return;
    if(plugin.category!=PluginType_Themes)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+plugin.name);
    int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).plugin==plugin)
        {
            if(pluginList.at(index).factory!=NULL)
                delete pluginList.at(index).factory;
            if(pluginList.at(index).pluginLoader!=NULL)
            {
                pluginList.at(index).pluginLoader->unload();
                delete pluginList.at(index).pluginLoader;
            }
            if(currentPluginIndex==index)
                currentPluginIndex=-1;
            if(index<currentPluginIndex)
                currentPluginIndex--;
            pluginList.removeAt(index);
            if(currentPluginIndex>=pluginList.size())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"plugin is out of inder!");
                currentPluginIndex=-1;
            }
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"plugin not found");
}
#endif

/** \brief To get image into the current themes, or default if not found
\param filePath The file path to search, like toto.png resolved with the root of the current themes
\see currentStylePath */
QIcon ThemesManager::loadIcon(const QString &fileName)
{
    if(currentPluginIndex==-1)
        return QIcon();
    if(pluginList.at(currentPluginIndex).factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Try get icon when the factory is not loaded");
        return QIcon();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Send interface pixmap: "+fileName);
    return pluginList.at(currentPluginIndex).factory->getIcon(fileName);
}

void ThemesManager::allPluginIsLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(pluginList.size()==0)
    {
        emit theThemeIsReloaded();
        return;
    }
    QString name=OptionEngine::optionEngine->getOptionValue("Themes","Ultracopier_current_theme").toString();
    int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).plugin.name==name)
        {
            currentPluginIndex=index;
            emit theThemeIsReloaded();
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"theme not found!");
    currentPluginIndex=-1;
    emit theThemeIsReloaded();
}

PluginInterface_Themes * ThemesManager::getThemesInstance()
{
    if(currentPluginIndex==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the interface, copy aborted");
        return NULL;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Send interface: "+pluginList.at(currentPluginIndex).plugin.name);
    if(currentPluginIndex>=pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the interface, internal selection bug");
        return NULL;
    }
    if(pluginList.at(currentPluginIndex).factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No plugin factory loaded to get an instance");
        return NULL;
    }
    return pluginList.at(currentPluginIndex).factory->getInstance();
}

#ifdef ULTRACOPIER_DEBUG
void ThemesManager::debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Theme plugin");
}
#endif // ULTRACOPIER_DEBUG

void ThemesManager::newOptionValue(const QString &group,const QString &name,const QVariant &value)
{
    if(group=="Themes" && name=="Ultracopier_current_theme")
    {
        if(!PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
            return;
        if(currentPluginIndex!=-1 && value.toString()!=pluginList.at(currentPluginIndex).plugin.name)
        {
            int tempCurrentPluginIndex=currentPluginIndex;
            emit theThemeNeedBeUnloaded();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("unload the themes: %1 (%2)").arg(pluginList.at(tempCurrentPluginIndex).plugin.name).arg(tempCurrentPluginIndex));
            if(pluginList.at(tempCurrentPluginIndex).options!=NULL)
            {
                delete pluginList.at(tempCurrentPluginIndex).options;
                pluginList[tempCurrentPluginIndex].options=NULL;
            }
            if(pluginList.at(tempCurrentPluginIndex).factory!=NULL)
            {
                delete pluginList.at(tempCurrentPluginIndex).factory;
                pluginList[tempCurrentPluginIndex].factory=NULL;
            }
            if(pluginList.at(tempCurrentPluginIndex).pluginLoader!=NULL)
            {
                pluginList.at(tempCurrentPluginIndex).pluginLoader->unload();
                delete pluginList.at(tempCurrentPluginIndex).pluginLoader;
                pluginList[tempCurrentPluginIndex].pluginLoader=NULL;
            }
        }
        allPluginIsLoaded();
        //emit theThemeIsReloaded(); -> do into allPluginIsLoaded(); now
    }
}
