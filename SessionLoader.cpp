/** \file SessionLoader.h
\brief Define the session loader
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "SessionLoader.h"
#include "LanguagesManager.h"

#if !defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE) || !defined(ULTRACOPIER_VERSION_PORTABLE)
SessionLoader::SessionLoader(OptionDialog *optionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->optionDialog=optionDialog;
    //load the options
    QList<QPair<QString, QVariant> > KeysList;
    #ifdef ULTRACOPIER_VERSION_PORTABLE
    KeysList.append(qMakePair(QString("LoadAtSessionStarting"),QVariant(false)));
    #else
    KeysList.append(qMakePair(QString("LoadAtSessionStarting"),QVariant(true)));
    #endif
    OptionEngine::optionEngine.addOptionGroup("SessionLoader",KeysList);
    connect(&OptionEngine::optionEngine,&OptionEngine::newOptionValue,	this,	&SessionLoader::newOptionValue,Qt::QueuedConnection);
    //load the plugin
    PluginsManager::pluginsManager.lockPluginListEdition();
    connect(this,&SessionLoader::previouslyPluginAdded,			this,&SessionLoader::onePluginAdded,Qt::QueuedConnection);
    connect(&PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,			this,&SessionLoader::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(&PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,	this,&SessionLoader::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    QList<PluginsAvailable> list=PluginsManager::pluginsManager.getPluginsByCategory(PluginType_SessionLoader);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager.unlockPluginListEdition();
    shouldEnabled=OptionEngine::optionEngine.getOptionValue("SessionLoader","LoadAtSessionStarting").toBool();
}

SessionLoader::~SessionLoader()
{
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    QList<PluginsAvailable> list=PluginsManager::pluginsManager.getPluginsByCategory(PluginType_SessionLoader);
    foreach(PluginsAvailable currentPlugin,list)
        onePluginWillBeRemoved(currentPlugin);
    #endif
}

void SessionLoader::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_SessionLoader)
        return;
    int index=0;
    LocalPlugin newEntry;
    QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName("sessionLoader");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try load: "+pluginPath);
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    PluginInterface_SessionLoader *sessionLoader;
    QObjectList objectList=QPluginLoader::staticInstances();
    index=0;
    QObject *pluginObject;
    while(index<objectList.size())
    {
        pluginObject=objectList.at(index);
        sessionLoader = qobject_cast<PluginInterface_SessionLoader *>(pluginObject);
        if(sessionLoader!=NULL)
            break;
        index++;
    }
    if(index==objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("static session loader not found"));
        return;
    }
    newEntry.pluginLoader=NULL;
    #else
    QPluginLoader *pluginLoader= new QPluginLoader(pluginPath);
    newEntry.pluginLoader=pluginLoader;
    QObject *pluginInstance = pluginLoader->instance();
    if(!pluginInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString());
        return;
    }
    PluginInterface_SessionLoader *sessionLoader = qobject_cast<PluginInterface_SessionLoader *>(pluginInstance);
    if(!sessionLoader)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString());
        return;
    }
    //check if found
    index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).sessionLoaderInterface==sessionLoader)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Plugin already found"));
            pluginLoader->unload();
            return;
        }
        index++;
    }
    #endif
    #ifdef ULTRACOPIER_DEBUG
    connect(sessionLoader,&PluginInterface_SessionLoader::debugInformation,this,&SessionLoader::debugInformation);
    #endif // ULTRACOPIER_DEBUG
    newEntry.options=new LocalPluginOptions("SessionLoader-"+plugin.name);
    newEntry.sessionLoaderInterface=sessionLoader;
    newEntry.path=plugin.path;
    newEntry.sessionLoaderInterface->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    newEntry.sessionLoaderInterface->setEnabled(shouldEnabled);
    optionDialog->addPluginOptionWidget(PluginType_SessionLoader,plugin.name,newEntry.sessionLoaderInterface->options());
    connect(&LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newEntry.sessionLoaderInterface,&PluginInterface_SessionLoader::newLanguageLoaded);
    pluginList << newEntry;
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void SessionLoader::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_SessionLoader)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int index=0;
    while(index<pluginList.size())
    {
        if(plugin.path==pluginList.at(index).path)
        {
            if(pluginList.at(index).pluginLoader!=NULL)
            {
                if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
                {
                    delete pluginList.at(index).options;
                    pluginList.removeAt(index);
                }
            }
            break;
        }
        index++;
    }
}
#endif

void SessionLoader::newOptionValue(const QString &groupName,const QString &variableName,const QVariant &value)
{
    if(groupName=="SessionLoader" && variableName=="LoadAtSessionStarting")
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, value: %1").arg(value.toBool()));
        shouldEnabled=value.toBool();
        int index=0;
        while(index<pluginList.size())
        {
            pluginList.at(index).sessionLoaderInterface->setEnabled(shouldEnabled);
            index++;
        }
    }
}

#ifdef ULTRACOPIER_DEBUG
void SessionLoader::debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Session loader plugin");
}
#endif // ULTRACOPIER_DEBUG
#endif // !defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE) || !defined(ULTRACOPIER_VERSION_PORTABLE)
