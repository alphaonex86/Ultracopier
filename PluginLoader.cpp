/** \file PluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "PluginLoader.h"

PluginLoader::PluginLoader(OptionDialog *optionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->optionDialog=optionDialog;
    //load the overall instance
    plugins=PluginsManager::getInstance();
    //load the plugin
    plugins->lockPluginListEdition();
    qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
    qRegisterMetaType<Ultracopier::CatchState>("Ultracopier::CatchState");
    connect(this,&PluginLoader::previouslyPluginAdded,	this,&PluginLoader::onePluginAdded,Qt::QueuedConnection);
    connect(plugins,&PluginsManager::onePluginAdded,	this,&PluginLoader::onePluginAdded,Qt::QueuedConnection);
    connect(plugins,&PluginsManager::onePluginWillBeRemoved,this,&PluginLoader::onePluginWillBeRemoved,Qt::DirectConnection);
    connect(plugins,&PluginsManager::pluginListingIsfinish,	this,&PluginLoader::allPluginIsloaded,Qt::QueuedConnection);
    QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_PluginLoader);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    plugins->unlockPluginListEdition();
    needEnable=false;
    last_state=Ultracopier::Uncaught;
    last_have_plugin=false;
    last_inWaitOfReply=false;
}

PluginLoader::~PluginLoader()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_PluginLoader);
    foreach(PluginsAvailable currentPlugin,list)
        onePluginWillBeRemoved(currentPlugin);
    PluginsManager::destroyInstanceAtTheLastCall();
}

void PluginLoader::resendState()
{
    sendState(true);
}

void PluginLoader::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_PluginLoader)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName("pluginLoader");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try load: "+pluginPath);
    QPluginLoader *pluginLoader= new QPluginLoader(pluginPath);
    QObject *pluginInstance = pluginLoader->instance();
    if(pluginInstance)
    {
        PluginInterface_PluginLoader *PluginLoader = qobject_cast<PluginInterface_PluginLoader *>(pluginInstance);
        //check if found
        index=0;
        loop_size=pluginList.size();
        while(index<loop_size)
        {
            if(pluginList.at(index).PluginLoaderInterface==PluginLoader)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Plugin already found"));
                pluginLoader->unload();
                return;
            }
            index++;
        }
        if(PluginLoader)
        {
            #ifdef ULTRACOPIER_DEBUG
            connect(PluginLoader,&PluginInterface_PluginLoader::debugInformation,this,&PluginLoader::debugInformation);
            #endif // ULTRACOPIER_DEBUG
            LocalPlugin newEntry;
            newEntry.options=new LocalPluginOptions("PluginLoader-"+plugin.name);
            newEntry.pluginLoader			= pluginLoader;
            newEntry.PluginLoaderInterface		= PluginLoader;
            newEntry.path				= plugin.path;
            newEntry.state				= Ultracopier::Uncaught;
            newEntry.inWaitOfReply			= false;
            pluginList << newEntry;
            PluginLoader->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
            optionDialog->addPluginOptionWidget(PluginType_PluginLoader,plugin.name,newEntry.PluginLoaderInterface->options());
            connect(pluginList.last().PluginLoaderInterface,&PluginInterface_PluginLoader::newState,this,&PluginLoader::newState);
            connect(languages,&LanguagesManager::newLanguageLoaded,newEntry.PluginLoaderInterface,&PluginInterface_PluginLoader::newLanguageLoaded);
            if(needEnable)
            {
                pluginList.last().inWaitOfReply=true;
                newEntry.PluginLoaderInterface->setEnabled(needEnable);
            }

        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString());
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString());
}

void PluginLoader::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_PluginLoader)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    index=0;
    loop_size=pluginList.size();
    while(index<loop_size)
    {
        if(plugin.path==pluginList.at(index).path)
        {
            pluginList.at(index).PluginLoaderInterface->setEnabled(false);
            if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
            {
                delete pluginList.at(index).options;
                pluginList.removeAt(index);
            }
            sendState();
            return;
        }
        index++;
    }
}

void PluginLoader::load()
{
    needEnable=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).PluginLoaderInterface->setEnabled(true);
        index++;
    }
    sendState(true);
}

void PluginLoader::unload()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    needEnable=false;
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).PluginLoaderInterface->setEnabled(false);
        index++;
    }
    sendState(true);
}

#ifdef ULTRACOPIER_DEBUG
void PluginLoader::debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Plugin loader plugin");
}
#endif // ULTRACOPIER_DEBUG

void PluginLoader::allPluginIsloaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"with value: "+QString::number(pluginList.size()>0));
    sendState(true);
}

void PluginLoader::sendState(bool force)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, pluginList.size(): %1, force: %2").arg(pluginList.size()).arg(force));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("current_state: %1").arg(current_state));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("send pluginLoaderReady(%1,%2,%3)").arg(current_state).arg(have_plugin).arg(found_inWaitOfReply));
        emit pluginLoaderReady(current_state,have_plugin,found_inWaitOfReply);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Skip the signal sending"));
    last_state=current_state;
    last_have_plugin=have_plugin;
    last_inWaitOfReply=found_inWaitOfReply;
}

void PluginLoader::newState(const Ultracopier::CatchState &state)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, state: %1").arg(state));
    PluginInterface_PluginLoader *temp=qobject_cast<PluginInterface_PluginLoader *>(QObject::sender());
    if(temp==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("listener not located!"));
        return;
    }
    int index=0;
    while(index<pluginList.size())
    {
        if(temp==pluginList.at(index).PluginLoaderInterface)
        {
            pluginList[index].state=state;
            pluginList[index].inWaitOfReply=false;
            sendState(true);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("listener not found!"));
}
