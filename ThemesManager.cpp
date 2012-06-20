/** \file ThemesManager.cpp
\brief Define the class for manage and load the themes
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */ 

#include <QString>
#include <QFile>
#include <QMessageBox>

#include "ThemesManager.h"

#define ULTRACOPIER_DEFAULT_STYLE "Oxygen"

/// \todo load each plugin to have their options
/// \todo get the current themes instance

/// \brief Create the manager and load the defaults variables
ThemesManager::ThemesManager()
{
	//load the debug engine as external part because ThemesManager is base class
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	currentPluginIndex=-1;

	//load the overall instance
	resources=ResourcesManager::getInstance();
	plugins=PluginsManager::getInstance();
	options=OptionEngine::getInstance();
	languages=LanguagesManager::getInstance();

	//connect the plugin management
	plugins->lockPluginListEdition();
	connect(this,SIGNAL(previouslyPluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,SLOT(allPluginIsLoaded()),Qt::QueuedConnection);
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Themes);
	foreach(PluginsAvailable currentPlugin,list)
		emit previouslyPluginAdded(currentPlugin);
	plugins->unlockPluginListEdition();

	//do the options
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("Ultracopier_current_theme"),QVariant(ULTRACOPIER_DEFAULT_STYLE)));
	options->addOptionGroup("Themes",KeysList);

	//load the default and current themes path
	defaultStylePath=":/Themes/"+QString(ULTRACOPIER_DEFAULT_STYLE)+"/";
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Default style: "+defaultStylePath);
	currentStylePath=defaultStylePath;
	connect(options,	SIGNAL(newOptionValue(QString,QString,QVariant)),	this,		SLOT(newOptionValue(QString,QString,QVariant)),Qt::QueuedConnection);
	connect(languages,	SIGNAL(newLanguageLoaded(QString)),			&facilityEngine,SLOT(retranslate()),Qt::QueuedConnection);
}

/// \brief Destroy the themes manager
ThemesManager::~ThemesManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Themes);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginWillBeRemoved(currentPlugin);
	LanguagesManager::destroyInstanceAtTheLastCall();
	OptionEngine::destroyInstanceAtTheLastCall();
	PluginsManager::destroyInstanceAtTheLastCall();
	ResourcesManager::destroyInstanceAtTheLastCall();
}

void ThemesManager::onePluginAdded(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_Themes)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+plugin.name);
	PluginsAvailableThemes newPlugin;
	newPlugin.plugin=plugin;
	QPluginLoader *pluginLoader=new QPluginLoader(newPlugin.plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"));
	QObject *pluginInstance = pluginLoader->instance();
	if(pluginInstance==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to load the plugin %1: %2").arg(newPlugin.plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface")).arg(pluginLoader->errorString()));
		pluginLoader->unload();
	}
	else
	{
		PluginInterface_ThemesFactory *factory = qobject_cast<PluginInterface_ThemesFactory *>(pluginInstance);
		//check if found
		int indexTemp=0;
		while(indexTemp<pluginList.size())
		{
			if(pluginList.at(indexTemp).factory==factory)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Plugin already found, current: %1, conflit plugin: %2, name: %3")
				.arg(newPlugin.plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"))
				.arg(pluginList.at(indexTemp).plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"))
				.arg(newPlugin.plugin.name)
				);
				pluginLoader->unload();
				emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
				emit theThemeIsReloaded();
				return;
			}
			indexTemp++;
		}
		if(factory==NULL)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to cast the plugin: %1").arg(pluginLoader->errorString()));
			pluginLoader->unload();
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("preload: %1, at the index: %2").arg(newPlugin.plugin.name).arg(QString::number(pluginList.size())));
			#ifdef ULTRACOPIER_DEBUG
			qRegisterMetaType<DebugLevel>("DebugLevel");
			connect(factory,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)),Qt::QueuedConnection);
			#endif // ULTRACOPIER_DEBUG
			connect(languages,SIGNAL(newLanguageLoaded(QString)),factory,SLOT(newLanguageLoaded()));
			newPlugin.factory=factory;
			newPlugin.pluginLoader=pluginLoader;
			newPlugin.options=new LocalPluginOptions("Themes-"+newPlugin.plugin.name);
			newPlugin.factory->setResources(newPlugin.options,newPlugin.plugin.writablePath,newPlugin.plugin.path,&facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
			currentStylePath=newPlugin.plugin.path;
			pluginList << newPlugin;
			if(plugins->allPluginHaveBeenLoaded())
				allPluginIsLoaded();
			emit newThemeOptions(newPlugin.plugin.name,newPlugin.factory->options(),true,true);
			emit theThemeIsReloaded();
			return;
		}
	}
	emit newThemeOptions(newPlugin.plugin.name,NULL,false,true);
	emit theThemeIsReloaded();
}

void ThemesManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_Themes)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+plugin.name);
	int index=0;
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
			if(currentPluginIndex==index)
				currentPluginIndex=-1;
			if(index<currentPluginIndex)
				currentPluginIndex--;
			pluginList.removeAt(index);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"plugin not found");
}

/** \brief To get image into the current themes, or default if not found
\param filePath The file path to search, like toto.png resolved with the root of the current themes
\see currentStylePath */
QIcon ThemesManager::loadIcon(const QString &fileName)
{
	if(currentPluginIndex==-1)
		return QIcon();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Send interface pixmap: "+fileName);
	return pluginList.at(currentPluginIndex).factory->getIcon(fileName);
}

void ThemesManager::allPluginIsLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(pluginList.size()==0)
	{
		emit theThemeIsReloaded();
		return;
	}
	QString name=options->getOptionValue("Themes","Ultracopier_current_theme").toString();
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).plugin.name==name)
		{
			currentPluginIndex=index;
			emit theThemeIsReloaded();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"theme not found!");
	currentPluginIndex=-1;
	emit theThemeIsReloaded();
}

PluginInterface_Themes * ThemesManager::getThemesInstance()
{
	if(currentPluginIndex==-1)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the interface, copy aborted");
		return NULL;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Send interface: "+pluginList.at(currentPluginIndex).plugin.name);
	return pluginList.at(currentPluginIndex).factory->getInstance();
}

#ifdef ULTRACOPIER_DEBUG
void ThemesManager::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Theme plugin");
}
#endif // ULTRACOPIER_DEBUG

void ThemesManager::newOptionValue(const QString &group,const QString &name,const QVariant &value)
{
	if(group=="Themes" && name=="Ultracopier_current_theme")
	{
		if(!plugins->allPluginHaveBeenLoaded())
			return;
		if(currentPluginIndex!=-1 && value.toString()!=pluginList.at(currentPluginIndex).plugin.name)
		{
			int tempCurrentPluginIndex=currentPluginIndex;
			emit theThemeNeedBeUnloaded();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("unload the themes: %1 (%2)").arg(pluginList.at(tempCurrentPluginIndex).plugin.name).arg(tempCurrentPluginIndex));
			if(pluginList.at(tempCurrentPluginIndex).options!=NULL)
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
			}
		}
		allPluginIsLoaded();
		//emit theThemeIsReloaded(); -> do into allPluginIsLoaded(); now
	}
}
