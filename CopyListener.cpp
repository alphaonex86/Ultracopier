/** \file CopyListener.h
\brief Define the copy listener
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "CopyListener.h"


CopyListener::CopyListener(QObject *parent) :
	QObject(parent)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the options
	tryListen=false;
	QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QString("CatchCopyAsDefault"),QVariant(true)));
	options->addOptionGroup("CopyListener",KeysList);
	plugins->lockPluginListEdition();
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Listener);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginAdded(currentPlugin);
	qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
	qRegisterMetaType<ListeningState>("ListeningState");
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)));
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,SLOT(allPluginIsloaded()));
	connect(&pluginLoader,SIGNAL(pluginLoaderReady(CatchState,bool,bool)),	this,SIGNAL(pluginLoaderReady(CatchState,bool,bool)));
	plugins->unlockPluginListEdition();
	last_state=NotListening;
	last_have_plugin=false;
	last_inWaitOfReply=false;
}

CopyListener::~CopyListener()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Listener);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginWillBeRemoved(currentPlugin);
}

void CopyListener::resendState()
{
	if(plugins->allPluginHaveBeenLoaded())
	{
		sendState(true);
		pluginLoader.resendState();
	}
}

void CopyListener::onePluginAdded(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_Listener)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try load: "+plugin.path+PluginsManager::getResolvedPluginName("listener"));
	//setFileName
	QPluginLoader *pluginLoader=new QPluginLoader(plugin.path+PluginsManager::getResolvedPluginName("listener"));
	QObject *pluginInstance = pluginLoader->instance();
	if(pluginInstance)
	{
		PluginInterface_Listener *listen = qobject_cast<PluginInterface_Listener *>(pluginInstance);
		//check if found
		int index=0;
		while(index<pluginList.size())
		{
			if(pluginList.at(index).listenInterface==listen)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Plugin already found"));
				pluginLoader->unload();
				return;
			}
			index++;
		}
		if(listen)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Plugin correctly loaded");
			#ifdef ULTRACOPIER_DEBUG
			connect(listen,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)));
			#endif // ULTRACOPIER_DEBUG
			connect(listen,SIGNAL(newCopy(quint32,QStringList)),		this,SLOT(newPluginCopy(quint32,QStringList)));
			connect(listen,SIGNAL(newCopy(quint32,QStringList,QString)),	this,SLOT(newPluginCopy(quint32,QStringList,QString)));
			connect(listen,SIGNAL(newMove(quint32,QStringList)),		this,SLOT(newPluginMove(quint32,QStringList)));
			connect(listen,SIGNAL(newMove(quint32,QStringList,QString)),	this,SLOT(newPluginMove(quint32,QStringList,QString)));
			PluginListener newPluginListener;
			newPluginListener.listenInterface	= listen;
			newPluginListener.pluginLoader		= pluginLoader;
			newPluginListener.path			= plugin.path+PluginsManager::getResolvedPluginName("listener");
			newPluginListener.state			= NotListening;
			newPluginListener.inWaitOfReply		= false;
			newPluginListener.options=new LocalPluginOptions("Listener-"+plugin.name);
			newPluginListener.listenInterface->setResources(newPluginListener.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
			pluginList << newPluginListener;
			connect(pluginList.last().listenInterface,SIGNAL(newState(ListeningState)),this,SLOT(newState(ListeningState)));
			if(tryListen)
			{
				pluginList.last().inWaitOfReply=true;
				listen->listen();
			}
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString());
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString());
}

#ifdef ULTRACOPIER_DEBUG
void CopyListener::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Listener plugin");
}
#endif // ULTRACOPIER_DEBUG

bool CopyListener::oneListenerIsLoaded()
{
	return (pluginList.size()>0);
}

void CopyListener::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_Listener)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"unload the current plugin");
	int indexPlugin=0;
	while(indexPlugin<pluginList.size())
	{
		if((plugin.path+PluginsManager::getResolvedPluginName("listener"))==pluginList.at(indexPlugin).path)
		{
			int index=0;
			while(index<copyRunningList.size())
			{
				if(copyRunningList.at(index).listenInterface==pluginList.at(indexPlugin).listenInterface)
					copyRunningList[index].listenInterface=NULL;
				index++;
			}
			pluginList.at(indexPlugin).listenInterface->close();
			disconnect(pluginList.at(indexPlugin).listenInterface);
			pluginList.at(indexPlugin).pluginLoader->unload();
			delete pluginList.at(indexPlugin).options;
			pluginList.removeAt(indexPlugin);
			sendState();
			return;
		}
		indexPlugin++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"not found");
}

void CopyListener::newState(const ListeningState &state)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	PluginInterface_Listener *temp=qobject_cast<PluginInterface_Listener *>(QObject::sender());
	if(temp==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("listener not located!"));
		return;
	}
	int index=0;
	while(index<pluginList.size())
	{
		if(temp==pluginList.at(index).listenInterface)
		{
			pluginList[index].state=state;
			pluginList[index].inWaitOfReply=false;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("new state for the plugin %1: %2").arg(index).arg(state));
			sendState(true);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("listener not found!"));
}

void CopyListener::listen()
{
	tryListen=true;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginList.size())
	{
		pluginList[index].inWaitOfReply=true;
		pluginList.at(index).listenInterface->listen();
		index++;
	}
	pluginLoader.load();
}

void CopyListener::close()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	tryListen=false;
	pluginLoader.unload();
	int index=0;
	while(index<pluginList.size())
	{
		pluginList[index].inWaitOfReply=true;
		pluginList.at(index).listenInterface->close();
		index++;
	}
	copyRunningList.clear();
}

QStringList CopyListener::parseWildcardSources(QStringList sources)
{
	QStringList returnList;
	int index=0;
	while(index<sources.size())
	{
		if(sources.at(index).contains("*"))
		{
			QFileInfo info(sources.at(index));
			QDir folder(info.absoluteDir());
			QFileInfoList fileFile=folder.entryInfoList(QStringList() << info.fileName());
			int index=0;
			while(index<fileFile.size())
			{
				returnList << fileFile.at(index).absoluteFilePath();
				index++;
			}
		}
		else
			returnList << sources.at(index);
		index++;
	}
	return returnList;
}

QStringList CopyListener::stripSeparator(QStringList sources)
{
	int index=0;
	while(index<sources.size())
	{
		sources[index].remove(QRegExp("[\\\\/]+$"));
		index++;
	}
	return sources;
}

/** new copy without destination have been pased by the CLI */
void CopyListener::newCopy(QStringList sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit newCopy(incrementOrderId(),QStringList() << "file",stripSeparator(parseWildcardSources(sources)));
}

/** new copy with destination have been pased by the CLI */
void CopyListener::newCopy(QStringList sources,QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit newCopy(incrementOrderId(),QStringList() << "file",stripSeparator(parseWildcardSources(sources)),"file",destination);
}

/** new move without destination have been pased by the CLI */
void CopyListener::newMove(QStringList sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit newMove(incrementOrderId(),QStringList() << "file",stripSeparator(parseWildcardSources(sources)));
}

/** new move with destination have been pased by the CLI */
void CopyListener::newMove(QStringList sources,QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit newMove(incrementOrderId(),QStringList() << "file",stripSeparator(parseWildcardSources(sources)),"file",destination);
}

void CopyListener::copyFinished(const quint32 & orderId,const bool &withError)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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

void CopyListener::newPluginCopy(const quint32 &orderId,const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"sources: "+sources.join(";"));
	PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
	CopyRunning newCopyInformation;
	newCopyInformation.listenInterface	= plugin;
	newCopyInformation.pluginOrderId	= orderId;
	newCopyInformation.orderId		= incrementOrderId();
	copyRunningList << newCopyInformation;
	emit newCopy(orderId,QStringList() << "file",stripSeparator(sources));
}

void CopyListener::newPluginCopy(const quint32 &orderId,const QStringList &sources,const QString &destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"sources: "+sources.join(";")+", destination: "+destination);
	PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
	CopyRunning newCopyInformation;
	newCopyInformation.listenInterface	= plugin;
	newCopyInformation.pluginOrderId	= orderId;
	newCopyInformation.orderId		= incrementOrderId();
	copyRunningList << newCopyInformation;
	emit newCopy(orderId,QStringList() << "file",stripSeparator(sources),"file",destination);
}

void CopyListener::newPluginMove(const quint32 &orderId,const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"sources: "+sources.join(";"));
	PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
	CopyRunning newCopyInformation;
	newCopyInformation.listenInterface	= plugin;
	newCopyInformation.pluginOrderId	= orderId;
	newCopyInformation.orderId		= incrementOrderId();
	copyRunningList << newCopyInformation;
	emit newMove(orderId,QStringList() << "file",stripSeparator(sources));
}

void CopyListener::newPluginMove(const quint32 &orderId,const QStringList &sources,const QString &destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"sources: "+sources.join(";")+", destination: "+destination);
	PluginInterface_Listener *plugin		= qobject_cast<PluginInterface_Listener *>(sender());
	CopyRunning newCopyInformation;
	newCopyInformation.listenInterface	= plugin;
	newCopyInformation.pluginOrderId	= orderId;
	newCopyInformation.orderId		= incrementOrderId();
	copyRunningList << newCopyInformation;
	emit newMove(orderId,QStringList() << "file",stripSeparator(sources),"file",destination);
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"with value: "+QString::number(pluginList.size()>0));
	sendState(true);
}

void CopyListener::sendState(bool force)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, pluginList.size(): %1, force: %2").arg(pluginList.size()).arg(force));
	ListeningState current_state=NotListening;
	bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
	int index=0;
	while(index<pluginList.size())
	{
		if(current_state==NotListening)
		{
			if(pluginList.at(index).state==SemiListening)
				current_state=SemiListening;
			else if(pluginList.at(index).state==NotListening)
				found_not_listen=true;
			else if(pluginList.at(index).state==FullListening)
				found_listen=true;
		}
		if(pluginList.at(index).inWaitOfReply)
			found_inWaitOfReply=true;
		index++;
	}
	if(current_state==NotListening)
	{
		if(found_not_listen && found_listen)
			current_state=SemiListening;
		else if(found_not_listen)
			current_state=NotListening;
		else if(FullListening)
			current_state=FullListening;
		else
			current_state=SemiListening;
	}
	bool have_plugin=pluginList.size()>0;
	if(force || current_state!=last_state || have_plugin!=last_have_plugin || found_inWaitOfReply!=last_inWaitOfReply)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("send listenerReady(%1,%2,%3)").arg(current_state).arg(have_plugin).arg(found_inWaitOfReply));
		emit listenerReady(current_state,have_plugin,found_inWaitOfReply);
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Skip the signal sending"));
	last_state=NotListening;
	last_have_plugin=have_plugin;
	last_inWaitOfReply=found_inWaitOfReply;
}
