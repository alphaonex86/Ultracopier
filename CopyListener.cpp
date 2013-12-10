/** \file CopyListener.h
\brief Define the copy listener
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "CopyListener.h"
#include "LanguagesManager.h"

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
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Listener);
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
    stripSeparatorRegex=QRegularExpression(QStringLiteral("[\\\\/]+$"));
}

CopyListener::~CopyListener()
{
    stopIt=true;
    delete pluginLoader;
}

void CopyListener::resendState()
{
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
    {
        sendState(true);
        pluginLoader->resendState();
    }
}

void CopyListener::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Listener)
        return;
    PluginListener newPluginListener;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("try load: ")+plugin.path+PluginsManager::getResolvedPluginName("listener"));
    //setFileName
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    PluginInterface_Listener *listen;
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("static listener not found"));
        return;
    }
    newPluginListener.pluginLoader=NULL;
    #else
    QPluginLoader *pluginOfPluginLoader=new QPluginLoader(plugin.path+PluginsManager::getResolvedPluginName(QStringLiteral("listener")));
    QObject *pluginInstance = pluginOfPluginLoader->instance();
    if(!pluginInstance)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to load the plugin: ")+pluginOfPluginLoader->errorString());
        return;
    }
    PluginInterface_Listener *listen = qobject_cast<PluginInterface_Listener *>(pluginInstance);
    if(!listen)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to load the plugin: ")+pluginOfPluginLoader->errorString());
        return;
    }
    //check if found
    int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).listenInterface==listen)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Plugin already found %1 for %2").arg(pluginList.at(index).path).arg(plugin.path));
            pluginOfPluginLoader->unload();
            return;
        }
        index++;
    }
    newPluginListener.pluginLoader		= pluginOfPluginLoader;
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("Plugin correctly loaded"));
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

    newPluginListener.path			= plugin.path+PluginsManager::getResolvedPluginName(QStringLiteral("listener"));
    newPluginListener.state			= Ultracopier::NotListening;
    newPluginListener.inWaitOfReply		= false;
    newPluginListener.options=new LocalPluginOptions(QStringLiteral("Listener-")+plugin.name);
    newPluginListener.listenInterface->setResources(newPluginListener.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    optionDialog->addPluginOptionWidget(PluginType_Listener,plugin.name,newPluginListener.listenInterface->options());
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newPluginListener.listenInterface,&PluginInterface_Listener::newLanguageLoaded,Qt::DirectConnection);
    pluginList << newPluginListener;
    connect(pluginList.last().listenInterface,&PluginInterface_Listener::newState,this,&CopyListener::newState,Qt::DirectConnection);
    if(tryListen)
    {
        pluginList.last().inWaitOfReply=true;
        listen->listen();
    }
}

#ifdef ULTRACOPIER_DEBUG
void CopyListener::debugInformation(const Ultracopier::DebugLevel &level, const QString& fonction, const QString& text, const QString& file, const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,QStringLiteral("Listener plugin"));
}
#endif // ULTRACOPIER_DEBUG

void CopyListener::error(const QString &error)
{
    QMessageBox::critical(NULL,tr("Error"),tr("Error during the reception of the copy/move list\n%1").arg(error));
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
    int indexPlugin=0;
    while(indexPlugin<pluginList.size())
    {
        if((plugin.path+PluginsManager::getResolvedPluginName(QStringLiteral("listener")))==pluginList.at(indexPlugin).path)
        {
            int index=0;
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
            pluginList.removeAt(indexPlugin);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    PluginInterface_Listener *temp=qobject_cast<PluginInterface_Listener *>(QObject::sender());
    if(temp==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("listener not located!"));
        return;
    }
    int index=0;
    while(index<pluginList.size())
    {
        if(temp==pluginList.at(index).listenInterface)
        {
            pluginList[index].state=state;
            pluginList[index].inWaitOfReply=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("new state for the plugin %1: %2").arg(index).arg(state));
            sendState(true);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("listener not found!"));
}

void CopyListener::listen()
{
    tryListen=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).listenInterface->listen();
        index++;
    }
    pluginLoader->load();
}

void CopyListener::close()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    tryListen=false;
    pluginLoader->unload();
    int index=0;
    while(index<pluginList.size())
    {
        pluginList[index].inWaitOfReply=true;
        pluginList.at(index).listenInterface->close();
        index++;
    }
    copyRunningList.clear();
}

QStringList CopyListener::stripSeparator(QStringList sources)
{
    int index=0;
    while(index<sources.size())
    {
        sources[index].remove(stripSeparatorRegex);
        index++;
    }
    return sources;
}

/** new copy without destination have been pased by the CLI */
void CopyListener::copyWithoutDestination(QStringList sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    emit newCopyWithoutDestination(incrementOrderId(),QStringList() << QStringLiteral("file"),stripSeparator(sources));
}

/** new copy with destination have been pased by the CLI */
void CopyListener::copy(QStringList sources,QString destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    emit newCopy(incrementOrderId(),QStringList() << QStringLiteral("file"),stripSeparator(sources),QStringLiteral("file"),destination);
}

/** new move without destination have been pased by the CLI */
void CopyListener::moveWithoutDestination(QStringList sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    emit newMoveWithoutDestination(incrementOrderId(),QStringList() << QStringLiteral("file"),stripSeparator(sources));
}

/** new move with destination have been pased by the CLI */
void CopyListener::move(QStringList sources,QString destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    emit newMove(incrementOrderId(),QStringList() << QStringLiteral("file"),stripSeparator(sources),QStringLiteral("file"),destination);
}

void CopyListener::copyFinished(const quint32 & orderId,const bool &withError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<copyRunningList.size())
    {
        if(orderId==copyRunningList.at(index).orderId)
        {
            orderList.removeAll(orderId);
            if(copyRunningList.at(index).listenInterface!=NULL)
                copyRunningList.at(index).listenInterface->transferFinished(copyRunningList.at(index).pluginOrderId,withError);
            copyRunningList.removeAt(index);
            return;
        }
        index++;
    }
}

void CopyListener::copyCanceled(const quint32 & orderId)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<copyRunningList.size())
    {
        if(orderId==copyRunningList.at(index).orderId)
        {
            orderList.removeAll(orderId);
            if(copyRunningList.at(index).listenInterface!=NULL)
                copyRunningList.at(index).listenInterface->transferCanceled(copyRunningList.at(index).pluginOrderId);
            copyRunningList.removeAt(index);
            return;
        }
        index++;
    }
}

void CopyListener::newPluginCopyWithoutDestination(const quint32 &orderId,const QStringList &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("sources: ")+sources.join(QStringLiteral(";")));
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList << newCopyInformation;
    emit newCopyWithoutDestination(orderId,QStringList() << QStringLiteral("file"),stripSeparator(sources));
}

void CopyListener::newPluginCopy(const quint32 &orderId,const QStringList &sources,const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("sources: ")+sources.join(QStringLiteral(";"))+QStringLiteral(", destination: ")+destination);
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList << newCopyInformation;
    emit newCopy(orderId,QStringList() << QStringLiteral("file"),stripSeparator(sources),QStringLiteral("file"),destination);
}

void CopyListener::newPluginMoveWithoutDestination(const quint32 &orderId,const QStringList &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("sources: ")+sources.join(QStringLiteral(";")));
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList << newCopyInformation;
    emit newMoveWithoutDestination(orderId,QStringList() << QStringLiteral("file"),stripSeparator(sources));
}

void CopyListener::newPluginMove(const quint32 &orderId,const QStringList &sources,const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("sources: ")+sources.join(";")+QStringLiteral(", destination: ")+destination);
    PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
    CopyRunning newCopyInformation;
    newCopyInformation.listenInterface	= plugin;
    newCopyInformation.pluginOrderId	= orderId;
    newCopyInformation.orderId		= incrementOrderId();
    copyRunningList << newCopyInformation;
    emit newMove(orderId,QStringList() << QStringLiteral("file"),stripSeparator(sources),QStringLiteral("file"),destination);
}

quint32 CopyListener::incrementOrderId()
{
    do
    {
        nextOrderId++;
        if(nextOrderId>2000000)
            nextOrderId=0;
    } while(orderList.contains(nextOrderId));
    return nextOrderId;
}

void CopyListener::allPluginIsloaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("with value: ")+QString::number(pluginList.size()>0));
    sendState(true);
    reloadClientList();
}

void CopyListener::reloadClientList()
{
    if(!PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        return;
    QStringList clients;
    int indexPlugin=0;
    while(indexPlugin<pluginList.size())
    {
        if(pluginList.at(indexPlugin).listenInterface!=NULL)
        {
            clients << pluginList[indexPlugin].listenInterface->clientsList();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("ask client to: ")+pluginList[indexPlugin].path);
        }
        indexPlugin++;
    }
    emit newClientList(clients);
}

void CopyListener::sendState(bool force)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, pluginList.size(): %1, force: %2").arg(pluginList.size()).arg(force));
    Ultracopier::ListeningState current_state=Ultracopier::NotListening;
    bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
    int index=0;
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("send listenerReady(%1,%2,%3)").arg(current_state).arg(have_plugin).arg(found_inWaitOfReply));
        emit listenerReady(current_state,have_plugin,found_inWaitOfReply);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("Skip the signal sending"));
    last_state=current_state;
    last_have_plugin=have_plugin;
    last_inWaitOfReply=found_inWaitOfReply;
}
