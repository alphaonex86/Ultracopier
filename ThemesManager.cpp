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
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Themes);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginAdded(currentPlugin);
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)));
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,SLOT(allPluginIsLoaded()));
	plugins->unlockPluginListEdition();

	//do the options
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("Ultracopier_current_theme"),QVariant(ULTRACOPIER_DEFAULT_STYLE)));
	options->addOptionGroup("Themes",KeysList);

	//load the default and current themes path
	defaultStylePath=":/Themes/"+QString(ULTRACOPIER_DEFAULT_STYLE)+"/";
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Default style: "+defaultStylePath);
	currentStylePath=defaultStylePath;
	connect(options,	SIGNAL(newOptionValue(QString,QString,QVariant)),	this,		SLOT(newOptionValue(QString,QString,QVariant)));
	connect(languages,	SIGNAL(newLanguageLoaded(QString)),			&facilityEngine,SLOT(retranslate()));
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

void ThemesManager::onePluginAdded(PluginsAvailable plugin)
{
	if(plugin.category!=PluginType_Themes)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+plugin.name);
	PluginsAvailableThemes newThemes;
	newThemes.plugin=plugin;
	newThemes.factory=NULL;
	newThemes.pluginLoader=NULL;
	pluginList << newThemes;
	//setFileName
	/**/
}

void ThemesManager::onePluginWillBeRemoved(PluginsAvailable plugin)
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
QPixmap ThemesManager::loadPixmap(QString filePath)
{
	if(getResourceFound(filePath))
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Current theme file is found: "+currentStylePath+filePath);
		return QPixmap(currentStylePath+filePath);
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Default theme file loaded: "+currentStylePath+filePath);
		return QPixmap(defaultStylePath+filePath);
	}
}

/** \brief To get if resource is found
\param filePath The file path to search, like toto.png resolved with the root of the current themes
\see currentStylePath */
bool ThemesManager::getResourceFound(QString filePath)
{
	return QFile::exists(currentStylePath+filePath);
}

void ThemesManager::allPluginIsLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	currentPluginIndex=-1;
	if(pluginList.size()==0)
	{
		emit newThemeOptions(NULL,false,false);
		return;
	}
	QString name=options->getOptionValue("Themes","Ultracopier_current_theme").toString();
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).plugin.name==name)
		{
			QPluginLoader *pluginLoader=new QPluginLoader(pluginList.at(index).plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"));
			QObject *pluginInstance = pluginLoader->instance();
			if(pluginInstance==NULL)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to load the plugin %1: %2").arg(pluginList.at(index).plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface")).arg(pluginLoader->errorString()));
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
						.arg(pluginList.at(index).plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"))
						.arg(pluginList.at(indexTemp).plugin.path+QDir::separator()+plugins->getResolvedPluginName("interface"))
						.arg(name)
						);
						pluginLoader->unload();
						emit newThemeOptions(NULL,false,true);
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
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"preload: "+name+", at the index: "+QString::number(index)+", name at this index: "+pluginList[index].plugin.name);
					#ifdef ULTRACOPIER_DEBUG
					qRegisterMetaType<DebugLevel>("DebugLevel");
					connect(factory,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)),Qt::QueuedConnection);
					#endif // ULTRACOPIER_DEBUG
					connect(languages,SIGNAL(newLanguageLoaded(QString)),factory,SLOT(newLanguageLoaded()));
					pluginList[index].factory=factory;
					pluginList[index].pluginLoader=pluginLoader;
					pluginList[index].options=new LocalPluginOptions("Themes-"+name);
					pluginList.at(index).factory->setResources(pluginList[index].options,pluginList.at(index).plugin.writablePath,pluginList.at(index).plugin.path,&facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
					currentPluginIndex=index;
					currentStylePath=pluginList[index].plugin.path;
					emit newThemeOptions(pluginList.at(index).factory->options(),true,true);
					return;
				}
			}
			emit newThemeOptions(NULL,false,true);
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"theme not found!");
	emit newThemeOptions(NULL,false,true);
}

PluginInterface_Themes * ThemesManager::getThemesInstance()
{
	if(currentPluginIndex==-1)
		return NULL;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Send interface: "+pluginList.at(currentPluginIndex).plugin.name);
	return pluginList.at(currentPluginIndex).factory->getInstance();
}

#ifdef ULTRACOPIER_DEBUG
void ThemesManager::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Theme plugin");
}
#endif // ULTRACOPIER_DEBUG

void ThemesManager::newOptionValue(QString group,QString name,QVariant value)
{
	if(group=="Themes" && name=="Ultracopier_current_theme")
	{
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
		emit theThemeIsReloaded();
	}
}
