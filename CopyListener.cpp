/** \file CopyListener.h
\brief Define the copy listener
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "CopyListener.h"
#include "LanguagesManager.h"
#include "cpp11addition.h"

#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include "plugins/Listener/catchcopy-v0002/listener.h"
#endif

#include <QRegularExpression>
#include <QMessageBox>

CopyListener::CopyListener(OptionDialog *optionDialog)
{
    stopIt=false;
    this->optionDialog=optionDialog;
    pluginLoader=new PluginLoader(optionDialog);
    //load the options
    tryListen=false;
    PluginsManager::pluginsManager->lockPluginListEdition();
    std::vector<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Listener);
    connect(this,&CopyListener::previouslyPluginAdded,			this,&CopyListener::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,			this,&CopyListener::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,		this,&CopyListener::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,			this,&CopyListener::allPluginIsloaded,Qt::QueuedConnection);
    connect(pluginLoader,&PluginLoader::pluginLoaderReady,			this,&CopyListener::pluginLoaderReady);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    last_state=Ultracopier::NotListening;
    last_have_plugin=false;
    last_inWaitOfReply=false;
    stripSeparatorRegex=std::regex("[\\\\/]+$");
}

CopyListener::~CopyListener()
{
    stopIt=true;
    if(pluginLoader!=NULL)
    {
        delete pluginLoader;
        pluginLoader=NULL;
    }
}

void CopyListener::resendState()
{
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
    {
        sendState(true);
        if(pluginLoader!=NULL)
            pluginLoader->resendState();
    }
}

void CopyListener::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Listener)
        return;
    PluginListener newPluginListener;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try load: "+plugin.path+PluginsManager::getResolvedPluginName("listener"));
    //setFileName
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    PluginInterface_Listener *listen;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    QObjectList objectList=QPluginLoader::staticInstances();
    int index=0;
    QObject *pluginObject;
    while(index<objectList.size())
    {
        pluginObject=objectList.at(index);
        listen = qobject_cast<PluginInterface_Listener *>(pluginObject);
        if(listen!=NULL)
            break;
        index++;
    }
    if(index==objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"static listener not found");
        return;
    }
    #else
    listen=new Listener();
    #endif
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
        newPluginListener.pluginLoader=NULL;
        #endif
    #else
    QPluginLoader *pluginOfPluginLoader=new QPluginLoader(QString::fromStdString(plugin.path+PluginsManager::getResolvedPluginName("listener")));
    QObject *pluginInstance = pluginOfPluginLoader->instance();
    if(!pluginInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+pluginOfPluginLoader->errorString().toStdString());
        return;
    }
    PluginInterface_Listener *listen = qobject_cast<PluginInterface_Listener *>(pluginInstance);
    if(!listen)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+pluginOfPluginLoader->errorString().toStdString());
        return;
    }
    //check if found
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).listenInterface==listen)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Plugin already found "+pluginList.at(index).path+" for "+plugin.path);
            pluginOfPluginLoader->unload();
            return;
        }
        index++;
    }
    newPluginListener.pluginLoader		= pluginOfPluginLoader;
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Plugin correctly loaded");
    #ifdef ULTRACOPIER_DEBUG
    connect(listen,&PluginInterface_Listener::debugInformation,this,&CopyListener::debugInformation,Qt::DirectConnection);
    #endif // ULTRACOPIER_DEBUG
    connect(listen,&PluginInterface_Listener::error,                            this,&CopyListener::error,Qt::DirectConnection);
    connect(listen,&PluginInterface_Listener::newCopyWithoutDestination,		this,&CopyListener::newPluginCopyWithoutDestination,Qt::DirectConnection);
    connect(listen,&PluginInterface_Listener::newCopy,                          this,&CopyListener::newPluginCopy,Qt::DirectConnection);
    connect(listen,&PluginInterface_Listener::newMoveWithoutDestination,		this,&CopyListener::newPluginMoveWithoutDestination,Qt::DirectConnection);
    connect(listen,&PluginInterface_Listener::newMove,                          this,&CopyListener::newPluginMove,Qt::DirectConnection);
    connect(listen,&PluginInterface_Listener::newClientList,                    this,&CopyListener::reloadClientList,Qt::DirectConnection);
    newPluginListener.listenInterface	= listen;

    newPluginListener.path			= plugin.path+PluginsManager::getResolvedPluginName("listener");
    newPluginListener.state			= Ultracopier::NotListening;
    newPluginListener.inWaitOfReply		= false;
    newPluginListener.options=new LocalPluginOptions("Listener-"+plugin.name);
    newPluginListener.listenInterface->setResources(newPluginListener.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    optionDialog->addPluginOptionWidget(PluginType_Listener,plugin.name,newPluginListener.listenInterface->options());
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newPluginListener.listenInterface,&PluginInterface_Listener::newLanguageLoaded,Qt::DirectConnection);
    pluginList.push_back(newPluginListener);
    connect(pluginList.back().listenInterface,&PluginInterface_Listener::newState,this,&CopyListener::newState,Qt::DirectConnection);
    if(tryListen)
    {
        pluginList.back().inWaitOfReply=true;
        listen->listen();
    }
}

#ifdef ULTRACOPIER_DEBUG
void CopyListener::debugInformation(const Ultracopier::DebugLevel &level, const std::string& fonction, const std::string& text, const std::string& file, const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Listener plugin");
}
#endif // ULTRACOPIER_DEBUG

void CopyListener::error(const std::string &error)
{
    QMessageBox::critical(NULL,tr("Error"),tr("Error during the reception of the copy/move list\n%1").arg(QString::fromStdString(error)));
}

bool CopyListener::oneListenerIsLoaded()
{
    return (pluginList.size()>0);
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void CopyListener::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Listener)
        return;
    unsigned int indexPlugin=0;
    while(indexPlugin<pluginList.size())
    {
        if((plugin.path+PluginsManager::getResolvedPluginName("listener"))==pluginList.at(indexPlugin).path)
        {
            unsigned int index=0;
            while(index<copyRunningList.size())
            {
                if(copyRunningList.at(index).listenInterface==pluginList.at(indexPlugin).listenInterface)
                    copyRunningList[index].listenInterface=NULL;
                index++;
            }
            if(pluginList.at(indexPlugin).listenInterface!=NULL)
            {
                pluginList.at(indexPlugin).listenInterface->close();
                delete pluginList.at(indexPlugin).listenInterface;
            }
            if(pluginList.at(indexPlugin).pluginLoader!=NULL)
            {
                pluginList.at(indexPlugin).pluginLoader->unload();
                delete pluginList.at(indexPlugin).options;
            }
            pluginList.erase(pluginList.cbegin()+indexPlugin);
            sendState();
            return;
        }
        indexPlugin++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"not found");
}
#endif

void CopyListener::newState(const Ultracopier::ListeningState &state)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    PluginInterface_Listener *temp=qobject_cast<PluginInterface_Listener *>(QObject::sender());
    if(temp==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"listener not located!");
        return;
    }
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(temp==pluginList.at(index).listenInterface)
        {
            pluginList[index].state=state;
            pluginList[index].inWaitOfReply=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new state for the plugin "+std::to_string(index)+": "+std::to_string((int)state));
            sendState(true);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"listener not found!");
}

void CopyListener::listen()
{
    tryListen=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).listenInterface->listen();
        index++;
    }
    if(pluginLoader!=NULL)
        pluginLoader->load();
}

void CopyListener::close()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    tryListen=false;
    if(pluginLoader!=NULL)
        pluginLoader->unload();
    unsigned int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).listenInterface->close();
        index++;
    }
    copyRunningList.clear();
}

std::vector<std::string> CopyListener::stripSeparator(std::vector<std::string> sources)
{
    unsigned int index=0;
    while(index<sources.size())
    {
        std::regex_replace(sources[index],stripSeparatorRegex,"");
        index++;
    }
    return sources;
}

/** new copy without destination have been pased by the CLI */
void CopyListener::copyWithoutDestination(std::vector<std::string> sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::vector<std::string> list;
    list.push_back("file");
    emit newCopyWithoutDestination(incrementOrderId(),list,stripSeparator(sources));
}

/** new copy with destination have been pased by the CLI */
void CopyListener::copy(std::vector<std::string> sources,std::string destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::vector<std::string> list;
    list.push_back("file");
    emit newCopy(incrementOrderId(),list,stripSeparator(sources),"file",destination);
}

/** new move without destination have been pased by the CLI */
void CopyListener::moveWithoutDestination(std::vector<std::string> sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::vector<std::string> list;
    list.push_back("file");
    emit newMoveWithoutDestination(incrementOrderId(),list,stripSeparator(sources));
}

/** new move with destination have been pased by the CLI */
void CopyListener::move(std::vector<std::string> sources,std::string destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::vector<std::string> list;
    list.push_back("file");
    emit newMove(incrementOrderId(),list,stripSeparator(sources),"file",destination);
}

void CopyListener::copyFinished(const quint32 & orderId,const bool &withError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<copyRunningList.size())
    {
        if(orderId==copyRunningList.at(index).orderId)
        {
            vectorRemoveAll(orderList,orderId);
            if(copyRunningList.at(index).listenInterface!=NULL)
                copyRunningList.at(index).listenInterface->transferFinished(copyRunningList.at(index).pluginOrderId,withError);
            copyRunningList.erase(copyRunningList.cbegin()+index);
            return;
        }
        index++;
    }
}

void CopyListener::copyCanceled(const uint32_t & orderId)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<copyRunningList.size())
    {
        if(orderId==copyRunningList.at(index).orderId)
        {
            vectorRemoveAll(orderList,orderId);
            if(copyRunningList.at(index).listenInterface!=NULL)
                copyRunningList.at(index).listenInterface->transferCanceled(copyRunningList.at(index).pluginOrderId);
            copyRunningList.erase(copyRunningList.cbegin()+index);
            return;
        }
        index++;
    }
}

void CopyListener::newPluginCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources: "+stringimplode(sources,";"));
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList.push_back(newCopyInformation);
    std::vector<std::string> stringList;stringList.push_back("file");
    emit newCopyWithoutDestination(orderId,stringList,stripSeparator(sources));
}

void CopyListener::newPluginCopy(const quint32 &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources: "+stringimplode(sources,";")+", destination: "+destination);
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList.push_back(newCopyInformation);
    std::vector<std::string> stringList;stringList.push_back("file");
    emit newCopy(orderId,stringList,stripSeparator(sources),"file",destination);
}

void CopyListener::newPluginMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources: "+stringimplode(sources,";"));
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList.push_back(newCopyInformation);
    std::vector<std::string> stringList;stringList.push_back("file");
    emit newMoveWithoutDestination(orderId,stringList,stripSeparator(sources));
}

void CopyListener::newPluginMove(const quint32 &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources: "+stringimplode(sources,";")+", destination: "+destination);
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList.push_back(newCopyInformation);
    std::vector<std::string> stringList;stringList.push_back("file");
    emit newMove(orderId,stringList,stripSeparator(sources),"file",destination);
}

uint32_t CopyListener::incrementOrderId()
{
    do
    {
        nextOrderId++;
        if(nextOrderId>2000000)
            nextOrderId=0;
    } while(vectorcontainsAtLeastOne(orderList,nextOrderId));
    return nextOrderId;
}

void CopyListener::allPluginIsloaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"with value: "+std::to_string(pluginList.size()>0));
    sendState(true);
    reloadClientList();
}

void CopyListener::reloadClientList()
{
    if(!PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        return;
    std::vector<std::string> clients;
    unsigned int indexPlugin=0;
    while(indexPlugin<pluginList.size())
    {
        if(pluginList.at(indexPlugin).listenInterface!=NULL)
        {
            const PluginListener &pluginListener=pluginList.at(indexPlugin);
            const std::vector<std::string> &clientsList=pluginListener.listenInterface->clientsList();
            clients.insert(clients.cbegin(),clientsList.cbegin(),clientsList.cend());
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ask client to: "+pluginList.at(indexPlugin).path);
        }
        indexPlugin++;
    }
    emit newClientList(clients);
}

void CopyListener::sendState(bool force)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, pluginList.size(): "+std::to_string(pluginList.size())+", force: "+std::to_string(force));
    Ultracopier::ListeningState current_state=Ultracopier::NotListening;
    bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(current_state==Ultracopier::NotListening)
        {
            if(pluginList.at(index).state==Ultracopier::SemiListening)
                current_state=Ultracopier::SemiListening;
            else if(pluginList.at(index).state==Ultracopier::NotListening)
                found_not_listen=true;
            else if(pluginList.at(index).state==Ultracopier::FullListening)
                found_listen=true;
        }
        if(pluginList.at(index).inWaitOfReply)
            found_inWaitOfReply=true;
        index++;
    }
    if(current_state==Ultracopier::NotListening)
    {
        if(found_not_listen && found_listen)
            current_state=Ultracopier::SemiListening;
        else if(found_not_listen)
            current_state=Ultracopier::NotListening;
        else if(!found_not_listen && found_listen)
            current_state=Ultracopier::FullListening;
        else
            current_state=Ultracopier::SemiListening;
    }
    bool have_plugin=pluginList.size()>0;
    if(force || current_state!=last_state || have_plugin!=last_have_plugin || found_inWaitOfReply!=last_inWaitOfReply)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"send listenerReady("+std::to_string(current_state)+","+std::to_string(have_plugin)+","+std::to_string(found_inWaitOfReply)+")");
        emit listenerReady(current_state,have_plugin,found_inWaitOfReply);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Skip the signal sending");
    last_state=current_state;
    last_have_plugin=have_plugin;
    last_inWaitOfReply=found_inWaitOfReply;
}
