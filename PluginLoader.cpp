/** \file PluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "PluginLoader.h"

PluginLoader::PluginLoader(OptionDialog *optionDialog)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->optionDialog=optionDialog;
	//load the overall instance
	plugins=PluginsManager::getInstance();
	//load the plugin
	plugins->lockPluginListEdition();
	qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
	qRegisterMetaType<CatchState>("CatchState");
	connect(this,SIGNAL(previouslyPluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,SLOT(allPluginIsloaded()),Qt::QueuedConnection);
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_PluginLoader);
	foreach(PluginsAvailable currentPlugin,list)
		emit previouslyPluginAdded(currentPlugin);
	plugins->unlockPluginListEdition();
	needEnable=false;
	last_state=Uncaught;
	last_have_plugin=false;
	last_inWaitOfReply=false;
}

PluginLoader::~PluginLoader()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName("pluginLoader");
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try load: "+pluginPath);
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
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Plugin already found"));
				pluginLoader->unload();
				return;
			}
			index++;
		}
		if(PluginLoader)
		{
			#ifdef ULTRACOPIER_DEBUG
			connect(PluginLoader,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)));
			#endif // ULTRACOPIER_DEBUG
			LocalPlugin newEntry;
			newEntry.options=new LocalPluginOptions("PluginLoader-"+plugin.name);
			newEntry.pluginLoader			= pluginLoader;
			newEntry.PluginLoaderInterface		= PluginLoader;
			newEntry.path				= plugin.path;
			newEntry.state				= Uncaught;
			newEntry.inWaitOfReply			= false;
			pluginList << newEntry;
			PluginLoader->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
			optionDialog->addPluginOptionWidget(PluginType_PluginLoader,plugin.name,newEntry.PluginLoaderInterface->options());
			connect(pluginList.last().PluginLoaderInterface,SIGNAL(newState(CatchState)),this,SLOT(newState(CatchState)));
			connect(languages,SIGNAL(newLanguageLoaded(QString)),newEntry.PluginLoaderInterface,SLOT(newLanguageLoaded()));
			if(needEnable)
			{
				pluginList.last().inWaitOfReply=true;
				newEntry.PluginLoaderInterface->setEnabled(needEnable);
			}

		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString());
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString());
}

void PluginLoader::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_PluginLoader)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
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
void PluginLoader::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Plugin loader plugin");
}
#endif // ULTRACOPIER_DEBUG

void PluginLoader::allPluginIsloaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"with value: "+QString::number(pluginList.size()>0));
	sendState(true);
}

void PluginLoader::sendState(bool force)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, pluginList.size(): %1, force: %2").arg(pluginList.size()).arg(force));
	CatchState current_state=Uncaught;
	bool found_not_listen=false,found_listen=false,found_inWaitOfReply=false;
	int index=0;
	while(index<pluginList.size())
	{
		if(current_state==Uncaught)
		{
			if(pluginList.at(index).state==Semiuncaught)
				current_state=Semiuncaught;
			else if(pluginList.at(index).state==Uncaught)
				found_not_listen=true;
			else if(pluginList.at(index).state==Caught)
				found_listen=true;
		}
		if(pluginList.at(index).inWaitOfReply)
			found_inWaitOfReply=true;
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("current_state: %1").arg(current_state));
	if(current_state==Uncaught)
	{
		if(!found_not_listen && !found_listen)
		{
			if(needEnable)
				current_state=Caught;
		}
		else if(found_not_listen && !found_listen)
			current_state=Uncaught;
		else if(!found_not_listen && found_listen)
			current_state=Caught;
		else
			current_state=Semiuncaught;
	}
	bool have_plugin=pluginList.size()>0;
	if(force || current_state!=last_state || have_plugin!=last_have_plugin || found_inWaitOfReply!=last_inWaitOfReply)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("send pluginLoaderReady(%1,%2,%3)").arg(current_state).arg(have_plugin).arg(found_inWaitOfReply));
		emit pluginLoaderReady(current_state,have_plugin,found_inWaitOfReply);
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Skip the signal sending"));
	last_state=current_state;
	last_have_plugin=have_plugin;
	last_inWaitOfReply=found_inWaitOfReply;
}

void PluginLoader::newState(const CatchState &state)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, state: %1").arg(state));
	PluginInterface_PluginLoader *temp=qobject_cast<PluginInterface_PluginLoader *>(QObject::sender());
	if(temp==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("listener not located!"));
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("listener not found!"));
}
