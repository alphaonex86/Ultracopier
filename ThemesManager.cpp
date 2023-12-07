/** \file ThemesManager.cpp
\brief Define the class for manage and load the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QString>
#include <QFile>
#include <QMessageBox>

#include "ThemesManager.h"
#include "PluginsManager.h"
#include "LanguagesManager.h"

#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
#include "plugins/Themes/Oxygen/ThemesFactory.h"
#endif

#ifdef ULTRACOPIER_MODE_SUPERCOPIER
#define ULTRACOPIER_DEFAULT_STYLE "Supercopier"
#else
#define ULTRACOPIER_DEFAULT_STYLE "Oxygen"
#endif

/// \warning All plugin remain loaded
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
    std::vector<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Themes);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();

    //do the options
    std::vector<std::pair<std::string, std::string> > KeysList;
    KeysList.push_back(std::pair<std::string, std::string>("Ultracopier_current_theme",ULTRACOPIER_DEFAULT_STYLE));
    OptionEngine::optionEngine->addOptionGroup("Themes",KeysList);

    //load the default and current themes path
    defaultStylePath=std::string(":/Themes/")+ULTRACOPIER_DEFAULT_STYLE+"/";
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Default style: "+defaultStylePath);
    currentStylePath=defaultStylePath;
    connect(OptionEngine::optionEngine,            &OptionEngine::newOptionValue,	this,		&ThemesManager::newOptionValue,Qt::QueuedConnection);
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
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"static themes not found");
            emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
            emit theThemeIsReloaded();
            return;
        }
        newPlugin.pluginLoader=NULL;
    #else
        factory=new ThemesFactory();
    #endif
    #else
    QPluginLoader *pluginLoader=new QPluginLoader(QString::fromStdString(newPlugin.plugin.path+FacilityEngine::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface")));
    QObject *pluginInstance = pluginLoader->instance();
    if(pluginInstance==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin "+newPlugin.plugin.path+FacilityEngine::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface")+": "+pluginLoader->errorString().toStdString());
        pluginLoader->unload();
        emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
        emit theThemeIsReloaded();
        return;
    }
    PluginInterface_ThemesFactory *factory = qobject_cast<PluginInterface_ThemesFactory *>(pluginInstance);
    if(factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString().toStdString());
        pluginLoader->unload();
        emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
        emit theThemeIsReloaded();
        return;
    }
    //check if found
    unsigned int indexTemp=0;
    while(indexTemp<pluginList.size())
    {
        if(pluginList.at(indexTemp).factory==factory)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Plugin already found, current: "+newPlugin.plugin.path+FacilityEngine::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface")+", conflit plugin: "+pluginList.at(indexTemp).plugin.path+FacilityEngine::separator()+PluginsManager::pluginsManager->getResolvedPluginName("interface")+", name: "+newPlugin.plugin.name);
            pluginLoader->unload();
            emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
            emit theThemeIsReloaded();
            return;
        }
        indexTemp++;
    }
    newPlugin.pluginLoader=pluginLoader;
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"preload: "+newPlugin.plugin.name+", at the index: "+std::to_string(pluginList.size()));
    #ifdef ULTRACOPIER_DEBUG
    connect(factory,&PluginInterface_ThemesFactory::debugInformation,this,&ThemesManager::debugInformation,Qt::QueuedConnection);
    #endif // ULTRACOPIER_DEBUG
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,factory,&PluginInterface_ThemesFactory::newLanguageLoaded);
    newPlugin.factory=factory;

    newPlugin.options=new LocalPluginOptions("Themes-"+newPlugin.plugin.name);
    newPlugin.factory->setResources(newPlugin.options,newPlugin.plugin.writablePath,newPlugin.plugin.path,&FacilityEngine::facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    currentStylePath=newPlugin.plugin.path;
    pluginList.push_back(newPlugin);
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
    unsigned int index=0;
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
            if(currentPluginIndex==static_cast<int>(index))
                currentPluginIndex=-1;
            if(static_cast<int>(index)<currentPluginIndex)
                currentPluginIndex--;
            pluginList.erase(pluginList.begin()+index);
            if(static_cast<unsigned int>(currentPluginIndex)>=pluginList.size())
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
QIcon ThemesManager::loadIcon(const std::string &fileName)
{
    if(currentPluginIndex==-1)
        return QIcon();
    if(pluginList.at(static_cast<unsigned int>(currentPluginIndex)).factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Try get icon when the factory is not loaded");
        return QIcon();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Send interface pixmap: "+fileName);
    return pluginList.at(static_cast<unsigned int>(currentPluginIndex)).factory->getIcon(fileName);
}

void ThemesManager::allPluginIsLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(pluginList.size()==0)
    {
        emit theThemeIsReloaded();
        return;
    }
    std::string name=OptionEngine::optionEngine->getOptionValue("Themes","Ultracopier_current_theme");
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).plugin.name==name)
        {
            currentPluginIndex=static_cast<int>(index);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Send interface: "+pluginList.at(static_cast<unsigned int>(currentPluginIndex)).plugin.name);
    if(static_cast<unsigned int>(currentPluginIndex)>=pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the interface, internal selection bug");
        return NULL;
    }
    if(pluginList.at(static_cast<unsigned int>(currentPluginIndex)).factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No plugin factory loaded to get an instance");
        return NULL;
    }
    return pluginList.at(static_cast<unsigned int>(currentPluginIndex)).factory->getInstance();
}

#ifdef ULTRACOPIER_DEBUG
void ThemesManager::debugInformation(const Ultracopier::DebugLevel &level,const std::string& fonction,const std::string& text,const std::string& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Theme plugin");
}
#endif // ULTRACOPIER_DEBUG

void ThemesManager::newOptionValue(const std::string &group,const std::string &name,const std::string &value)
{
    if(group=="Themes" && name=="Ultracopier_current_theme")
    {
        if(!PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
            return;
        if(currentPluginIndex!=-1 && value!=pluginList.at(static_cast<unsigned int>(currentPluginIndex)).plugin.name)
        {
            //int tempCurrentPluginIndex=currentPluginIndex;
            emit theThemeNeedBeUnloaded();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unload the themes: "+pluginList.at(static_cast<unsigned int>(currentPluginIndex)).plugin.name+" ("+std::to_string(currentPluginIndex)+")");
            /* Themes remain loaded for the options
             *if(pluginList.at(tempCurrentPluginIndex).options!=NULL)
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
            } */
        }
        allPluginIsLoaded();
        //emit theThemeIsReloaded(); -> do into allPluginIsLoaded(); now
    }
}
