/** \file PluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "PluginLoaderCore.h"
#include "LanguagesManager.h"

#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
#ifdef Q_OS_WIN32
#include "plugins/PluginLoader/catchcopy-v0002/pluginLoader.h"
#endif
#endif

PluginLoaderCore::PluginLoaderCore(OptionDialog *optionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->optionDialog=optionDialog;
    //load the overall instance
    //load the plugin
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,&PluginLoaderCore::previouslyPluginAdded,	this,&PluginLoaderCore::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,	this,&PluginLoaderCore::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,this,&PluginLoaderCore::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,	this,&PluginLoaderCore::allPluginIsloaded,Qt::QueuedConnection);
    std::vector<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_PluginLoader);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    needEnable=false;
    last_state=Ultracopier::Uncaught;
    last_have_plugin=false;
    last_inWaitOfReply=false;
    stopIt=false;
}

PluginLoaderCore::~PluginLoaderCore()
{
    stopIt=true;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    {
        /* why it crash here? Only under Window with PluginLoader/catchcopy-v0002
        int index=0;
        while(index<pluginList.size())
        {
            pluginList.at(index).pluginLoaderInterface->setEnabled(false);
            if(pluginList.at(index).pluginLoader!=NULL)
            {
                if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
                {
                    delete pluginList.at(index).options;
                    pluginList.removeAt(index);
                }
                else
                    index++;
            }
            else
                index++;
        }//*/
    }
    #endif
}

void PluginLoaderCore::resendState()
{
    if(stopIt)
        return;
    sendState(true);
}

void PluginLoaderCore::onePluginAdded(const PluginsAvailable &plugin)
{
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    #ifdef Q_OS_WIN32
    PluginInterface_PluginLoader *factory;
    #endif
    #endif
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    if(stopIt)
        return;
    if(plugin.category!=PluginType_PluginLoader)
        return;
    LocalPlugin newEntry;
    std::string pluginPath=plugin.path+PluginsManager::getResolvedPluginName("pluginLoader");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try load: "+pluginPath);
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"static listener not found");
        return;
    }
    newEntry.pluginLoader=NULL;
    #else
    QPluginLoader *pluginLoader= new QPluginLoader(QString::fromStdString(pluginPath));
    QObject *pluginInstance = pluginLoader->instance();
    if(!pluginInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString().toStdString());
        return;
    }
    PluginInterface_PluginLoader *pluginLoaderInstance = qobject_cast<PluginInterface_PluginLoader *>(pluginInstance);
    if(!pluginLoaderInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString().toStdString());
        return;
    }
    newEntry.pluginLoader			= pluginLoader;
    //check if found
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).pluginLoaderInterface==pluginLoaderInstance)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Plugin already found");
            pluginLoader->unload();
            return;
        }
        index++;
    }
    #endif
    #ifdef ULTRACOPIER_DEBUG
    connect(pluginLoaderInstance,&PluginInterface_PluginLoader::debugInformation,this,&PluginLoaderCore::debugInformation,Qt::DirectConnection);
    #endif // ULTRACOPIER_DEBUG

    newEntry.options=new LocalPluginOptions("PluginLoader-"+plugin.name);
    newEntry.pluginLoaderInterface		= pluginLoaderInstance;
    newEntry.path				= plugin.path;
    newEntry.state				= Ultracopier::Uncaught;
    newEntry.inWaitOfReply			= false;
    pluginList.push_back(newEntry);
    pluginLoaderInstance->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    optionDialog->addPluginOptionWidget(PluginType_PluginLoader,plugin.name,newEntry.pluginLoaderInterface->options());
    connect(pluginList.back().pluginLoaderInterface,&PluginInterface_PluginLoader::newState,this,&PluginLoaderCore::newState,Qt::DirectConnection);
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newEntry.pluginLoaderInterface,&PluginInterface_PluginLoader::newLanguageLoaded,Qt::DirectConnection);
    if(needEnable)
    {
        pluginList.back().inWaitOfReply=true;
        newEntry.pluginLoaderInterface->setEnabled(needEnable);
    }
    #else
    #ifdef Q_OS_WIN32
    factory=new WindowsExplorerLoader();
    LocalPlugin newEntry;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    newEntry.pluginLoader=NULL;
    #endif

    newEntry.options=new LocalPluginOptions("PluginLoader-"+plugin.name);
    newEntry.pluginLoaderInterface		= new WindowsExplorerLoader();
    newEntry.path				= plugin.path;
    newEntry.state				= Ultracopier::Uncaught;
    newEntry.inWaitOfReply			= false;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    #ifdef ULTRACOPIER_DEBUG
    connect(newEntry.pluginLoaderInterface,&PluginInterface_PluginLoader::debugInformation,this,&PluginLoaderCore::debugInformation,Qt::DirectConnection);
    #endif // ULTRACOPIER_DEBUG
    #endif
    pluginList.push_back(newEntry);
    newEntry.pluginLoaderInterface->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    optionDialog->addPluginOptionWidget(PluginType_PluginLoader,plugin.name,newEntry.pluginLoaderInterface->options());
    connect(pluginList.back().pluginLoaderInterface,&PluginInterface_PluginLoader::newState,this,&PluginLoaderCore::newState,Qt::DirectConnection);
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newEntry.pluginLoaderInterface,&PluginInterface_PluginLoader::newLanguageLoaded,Qt::DirectConnection);
    if(needEnable)
    {
        pluginList.back().inWaitOfReply=true;
        newEntry.pluginLoaderInterface->setEnabled(needEnable);
    }
    #endif
    Q_UNUSED(plugin);
    #endif
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void PluginLoaderCore::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(stopIt)
        return;
    if(plugin.category!=PluginType_PluginLoader)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(plugin.path==pluginList.at(index).path)
        {
            pluginList.at(index).pluginLoaderInterface->setEnabled(false);
            if(pluginList.at(index).pluginLoader!=NULL)
            {
                if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
                {
                    delete pluginList.at(index).options;
                    pluginList.erase(pluginList.cbegin()+index);
                }
            }
            sendState();
            return;
        }
        index++;
    }
}
#endif

void PluginLoaderCore::load()
{
    if(stopIt)
        return;
    needEnable=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).pluginLoaderInterface->setEnabled(true);
        index++;
    }
    sendState(true);
}

void PluginLoaderCore::unload()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    needEnable=false;
    unsigned int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).pluginLoaderInterface->setEnabled(false);
        index++;
    }
    sendState(true);
}

#ifdef ULTRACOPIER_DEBUG
void PluginLoaderCore::debugInformation(const Ultracopier::DebugLevel &level,const std::string& fonction,const std::string& text,const std::string& file,const unsigned int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Plugin loader plugin");
}
#endif // ULTRACOPIER_DEBUG

void PluginLoaderCore::allPluginIsloaded()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"with value: "+std::to_string(pluginList.size()>0));
    sendState(true);
}

void PluginLoaderCore::sendState(bool force)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, pluginList.size(): "+std::to_string(pluginList.size())+", force: "+std::to_string(force));
    Ultracopier::CatchState current_state=Ultracopier::Uncaught;
    bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
    unsigned int index=0;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"current_state: "+std::to_string(current_state));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"send pluginLoaderReady("+std::to_string(current_state)+","+std::to_string(have_plugin)+","+std::to_string(found_inWaitOfReply)+")");
        emit pluginLoaderReady(current_state,have_plugin,found_inWaitOfReply);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Skip the signal sending");
    last_state=current_state;
    last_have_plugin=have_plugin;
    last_inWaitOfReply=found_inWaitOfReply;
}

void PluginLoaderCore::newState(const Ultracopier::CatchState &state)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, state: "+std::to_string(state));
    PluginInterface_PluginLoader *temp=qobject_cast<PluginInterface_PluginLoader *>(QObject::sender());
    if(temp==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"listener not located!");
        return;
    }
    unsigned int index=0;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"listener not found!");
}
