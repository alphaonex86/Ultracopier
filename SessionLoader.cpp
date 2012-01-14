/** \file SessionLoader.h
\brief Define the session loader
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "SessionLoader.h"

SessionLoader::SessionLoader(QObject *parent) :
	QObject(parent)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the options
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("LoadAtSessionStarting"),QVariant(true)));
	options->addOptionGroup("SessionLoader",KeysList);
	connect(options,SIGNAL(newOptionValue(QString,QString,QVariant)),	this,	SLOT(newOptionValue(QString,QString,QVariant)));
	//load the plugin
	plugins->lockPluginListEdition();
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_SessionLoader);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginAdded(currentPlugin);
	qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)));
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	plugins->unlockPluginListEdition();
}

SessionLoader::~SessionLoader()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_SessionLoader);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginWillBeRemoved(currentPlugin);
}

void SessionLoader::onePluginAdded(PluginsAvailable plugin)
{
	if(plugin.category!=PluginType_SessionLoader)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName("sessionLoader");
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"try load: "+pluginPath);
	LocalPlugin newEntry;
	QPluginLoader *pluginLoader= new QPluginLoader(pluginPath);
	newEntry.pluginLoader=pluginLoader;
	QObject *pluginInstance = pluginLoader->instance();
	if(pluginInstance)
	{
		PluginInterface_SessionLoader *sessionLoader = qobject_cast<PluginInterface_SessionLoader *>(pluginInstance);
		//check if found
		int index=0;
		while(index<pluginList.size())
		{
			if(pluginList.at(index).sessionLoaderInterface==sessionLoader)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Plugin already found"));
				pluginLoader->unload();
				return;
			}
			index++;
		}
		if(sessionLoader)
		{
			#ifdef ULTRACOPIER_DEBUG
			connect(sessionLoader,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)));
			#endif // ULTRACOPIER_DEBUG
			newEntry.options=new LocalPluginOptions("SessionLoader-"+plugin.name);
			newEntry.sessionLoaderInterface=sessionLoader;
			newEntry.path=plugin.path;
			newEntry.sessionLoaderInterface->setResources(newEntry.options,plugin.writablePath,plugin.path,ULTRACOPIER_VERSION_PORTABLE_BOOL);
			pluginList << newEntry;
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to cast the plugin: "+pluginLoader->errorString());
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to load the plugin: "+pluginLoader->errorString());
}

void SessionLoader::onePluginWillBeRemoved(PluginsAvailable plugin)
{
	if(plugin.category!=PluginType_SessionLoader)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginList.size())
	{
		if(plugin.path==pluginList.at(index).path)
		{
			if(!pluginList.at(index).pluginLoader->isLoaded() || pluginList.at(index).pluginLoader->unload())
			{
				delete pluginList.at(index).options;
				pluginList.removeAt(index);
			}
			break;
		}
		index++;
	}
}

void SessionLoader::newOptionValue(QString groupName,QString variableName,QVariant value)
{
	if(groupName=="SessionLoader" && variableName=="Enabled")
	{
		bool shouldEnabled=value.toBool();
		int index=0;
		while(index<pluginList.size())
		{
			pluginList.at(index).sessionLoaderInterface->setEnabled(shouldEnabled);
			index++;
		}
	}
}

#ifdef ULTRACOPIER_DEBUG
void SessionLoader::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Session loader plugin");
}
#endif // ULTRACOPIER_DEBUG

