/** \file PluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "PluginLoader.h"
#include "LanguagesManager.h"

PluginLoader::PluginLoader(OptionDialog *optionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    this->optionDialog=optionDialog;
    //load the overall instance
    //load the plugin
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,&PluginLoader::previouslyPluginAdded,	this,&PluginLoader::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,	this,&PluginLoader::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,this,&PluginLoader::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,	this,&PluginLoader::allPluginIsloaded,Qt::QueuedConnection);
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_PluginLoader);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    needEnable=false;
    last_state=Ultracopier::Uncaught;
    last_have_plugin=false;
    last_inWaitOfReply=false;
    stopIt=false;
}

PluginLoader::~PluginLoader()
{
    stopIt=true;
    int index=0;
    int loop_size=pluginList.size();
    while(index<loop_size)
    {
        pluginList.at(index).pluginLoaderInterface->setEnabled(false);
        if(pluginList.at(index).pluginLoader!=NULL)
        {
            if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
            {
                delete pluginList.at(index).options;
                pluginList.removeAt(index);
            }
        }
        index++;
    }
}

void PluginLoader::resendState()
{
    if(stopIt)
        return;
    sendState(true);
}

void PluginLoader::onePluginAdded(const PluginsAvailable &plugin)
{
    if(stopIt)
        return;
    if(plugin.category!=PluginType_PluginLoader)
        return;
    LocalPlugin newEntry;
    QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName(QStringLiteral("pluginLoader"));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("try load: ")+pluginPath);
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    PluginInterface_PluginLoader *pluginLoaderInstance;
    QObjectList objectList=QPluginLoader::staticInstances();
    int index=0;
    QObject *pluginObject;
    while(index<objectList.size())
    {
        pluginObject=objectList.at(index);
        pluginLoaderInstance = qobject_cast<PluginInterface_PluginLoader *>(pluginObject);
        if(pluginLoaderInstance!=NULL)
            break;
        index++;
    }
    if(index==objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("static listener not found"));
        return;
    }
    newEntry.pluginLoader=NULL;
    #else
    QPluginLoader *pluginLoader= new QPluginLoader(pluginPath);
    QObject *pluginInstance = pluginLoader->instance();
    if(!pluginInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to load the plugin: ")+pluginLoader->errorString());
        return;
    }
    PluginInterface_PluginLoader *pluginLoaderInstance = qobject_cast<PluginInterface_PluginLoader *>(pluginInstance);
    if(!pluginLoaderInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to cast the plugin: ")+pluginLoader->errorString());
        return;
    }
    newEntry.pluginLoader			= pluginLoader;
    //check if found
    int index=0;
    int loop_size=pluginList.size();
    while(index<loop_size)
    {
        if(pluginList.at(index).pluginLoaderInterface==pluginLoaderInstance)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Plugin already found"));
            pluginLoader->unload();
            return;
        }
        index++;
    }
    #endif
    #ifdef ULTRACOPIER_DEBUG
    connect(pluginLoaderInstance,&PluginInterface_PluginLoader::debugInformation,this,&PluginLoader::debugInformation,Qt::DirectConnection);
    #endif // ULTRACOPIER_DEBUG

    newEntry.options=new LocalPluginOptions(QStringLiteral("PluginLoader-")+plugin.name);
    newEntry.pluginLoaderInterface		= pluginLoaderInstance;
    newEntry.path				= plugin.path;
    newEntry.state				= Ultracopier::Uncaught;
    newEntry.inWaitOfReply			= false;
    pluginList << newEntry;
    pluginLoaderInstance->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    optionDialog->addPluginOptionWidget(PluginType_PluginLoader,plugin.name,newEntry.pluginLoaderInterface->options());
    connect(pluginList.last().pluginLoaderInterface,&PluginInterface_PluginLoader::newState,this,&PluginLoader::newState,Qt::DirectConnection);
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newEntry.pluginLoaderInterface,&PluginInterface_PluginLoader::newLanguageLoaded,Qt::DirectConnection);
    if(needEnable)
    {
        pluginList.last().inWaitOfReply=true;
        newEntry.pluginLoaderInterface->setEnabled(needEnable);
    }
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void PluginLoader::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(stopIt)
        return;
    if(plugin.category!=PluginType_PluginLoader)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    int loop_size=pluginList.size();
    while(index<loop_size)
    {
        if(plugin.path==pluginList.at(index).path)
        {
            pluginList.at(index).pluginLoaderInterface->setEnabled(false);
            if(pluginList.at(index).pluginLoader!=NULL)
            {
                if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
                {
                    delete pluginList.at(index).options;
                    pluginList.removeAt(index);
                }
            }
            sendState();
            return;
        }
        index++;
    }
}
#endif

void PluginLoader::load()
{
    if(stopIt)
        return;
    needEnable=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).pluginLoaderInterface->setEnabled(true);
        index++;
    }
    sendState(true);
}

void PluginLoader::unload()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    needEnable=false;
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).pluginLoaderInterface->setEnabled(false);
        index++;
    }
    sendState(true);
}

#ifdef ULTRACOPIER_DEBUG
void PluginLoader::debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,QStringLiteral("Plugin loader plugin"));
}
#endif // ULTRACOPIER_DEBUG

void PluginLoader::allPluginIsloaded()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("with value: ")+QString::number(pluginList.size()>0));
    sendState(true);
}

void PluginLoader::sendState(bool force)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, pluginList.size(): %1, force: %2").arg(pluginList.size()).arg(force));
    Ultracopier::CatchState current_state=Ultracopier::Uncaught;
    bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
    int index=0;
    while(index<pluginList.size())
    {
        if(current_state==Ultracopier::Uncaught)
        {
            if(pluginList.at(index).state==Ultracopier::Semiuncaught)
                current_state=Ultracopier::Semiuncaught;
            else if(pluginList.at(index).state==Ultracopier::Uncaught)
                found_not_listen=true;
            else if(pluginList.at(index).state==Ultracopier::Caught)
                found_listen=true;
        }
        if(pluginList.at(index).inWaitOfReply)
            found_inWaitOfReply=true;
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("current_state: %1").arg(current_state));
    if(current_state==Ultracopier::Uncaught)
    {
        if(!found_not_listen && !found_listen)
        {
            if(needEnable)
                current_state=Ultracopier::Caught;
        }
        else if(found_not_listen && !found_listen)
            current_state=Ultracopier::Uncaught;
        else if(!found_not_listen && found_listen)
            current_state=Ultracopier::Caught;
        else
            current_state=Ultracopier::Semiuncaught;
    }
    bool have_plugin=pluginList.size()>0;
    if(force || current_state!=last_state || have_plugin!=last_have_plugin || found_inWaitOfReply!=last_inWaitOfReply)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("send pluginLoaderReady(%1,%2,%3)").arg(current_state).arg(have_plugin).arg(found_inWaitOfReply));
        emit pluginLoaderReady(current_state,have_plugin,found_inWaitOfReply);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("Skip the signal sending"));
    last_state=current_state;
    last_have_plugin=have_plugin;
    last_inWaitOfReply=found_inWaitOfReply;
}

void PluginLoader::newState(const Ultracopier::CatchState &state)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, state: %1").arg(state));
    PluginInterface_PluginLoader *temp=qobject_cast<PluginInterface_PluginLoader *>(QObject::sender());
    if(temp==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("listener not located!"));
        return;
    }
    int index=0;
    while(index<pluginList.size())
    {
        if(temp==pluginList.at(index).pluginLoaderInterface)
        {
            pluginList[index].state=state;
            pluginList[index].inWaitOfReply=false;
            sendState(true);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("listener not found!"));
}
