/** \file LanguagesManager.cpp
\brief Define the class to manage and load the languages
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */ 

#include <QDir>
#include <QLibraryInfo>

#include "LanguagesManager.h"

/// \brief Create the manager and load the defaults variables
LanguagesManager::LanguagesManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the overall instance
	resources=ResourcesManager::getInstance();
	options=OptionEngine::getInstance();
	plugins=PluginsManager::getInstance();
	//load the rest
	QStringList resourcesPaths=resources->getReadPath();
	int index=0;
	while(index<resourcesPaths.size())
	{
		QString composedTempPath=resourcesPaths.at(index)+"Languages"+QDir::separator();
		QDir LanguagesConfiguration(composedTempPath);
		if(LanguagesConfiguration.exists())
			languagePath<<composedTempPath;
		index++;
	}
	//load the plugins
	plugins->lockPluginListEdition();
	qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
	connect(this,SIGNAL(previouslyPluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,	SLOT(onePluginAdded(PluginsAvailable)),Qt::QueuedConnection);
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,	SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,	SLOT(allPluginIsLoaded()),Qt::QueuedConnection);
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_Languages);
	foreach(PluginsAvailable currentPlugin,list)
		emit previouslyPluginAdded(currentPlugin);
	plugins->unlockPluginListEdition();
	//load the GUI option
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("Language"),QVariant("en")));
	KeysList.append(qMakePair(QString("Language_autodetect"),QVariant(true)));
	options->addOptionGroup("Language",KeysList);
//	connect(this,	SIGNAL(newLanguageLoaded(QString)),			plugins,SLOT(refreshPluginList(QString)));
//	connect(this,	SIGNAL(newLanguageLoaded(QString)),			this,SLOT(retranslateTheUI()));
	connect(options,SIGNAL(newOptionValue(QString,QString,QVariant)),	this,	SLOT(newOptionValue(QString)),Qt::QueuedConnection);
	connect(this,	SIGNAL(newLanguageLoaded(QString)),			plugins,SIGNAL(newLanguageLoaded()),Qt::QueuedConnection);
}

/// \brief Destroy the language manager
LanguagesManager::~LanguagesManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	PluginsManager::destroyInstanceAtTheLastCall();
	OptionEngine::destroyInstanceAtTheLastCall();
	ResourcesManager::destroyInstanceAtTheLastCall();
}

/// \brief load the language selected, return the main short code like en, fr, ..
QString LanguagesManager::getTheRightLanguage()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(LanguagesAvailableList.size()==0)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"empty combobox list, failing back to english");
		return "en";
	}
	else
	{
		if(options->getOptionValue("Language","Language_autodetect").toBool())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"language auto-detection, QLocale::system().name(): "+QLocale::system().name()+", QLocale::languageToString(QLocale::system().language()): "+QLocale::languageToString(QLocale::system().language()));
			QString tempLanguage=getMainShortName(QLocale::languageToString(QLocale::system().language()));
			if(tempLanguage!="")
				return tempLanguage;
			else
			{
				tempLanguage=getMainShortName(QLocale::system().name());
				if(tempLanguage!="")
					return tempLanguage;
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Autodetection of the language failed, QLocale::languageToString(QLocale::system().language()): "+QLocale::languageToString(QLocale::system().language())+", QLocale::system().name(): "+QLocale::system().name()+", failing back to english");
					return options->getOptionValue("Language","Language").toString();
				}
			}
		}
		else
			return options->getOptionValue("Language","Language").toString();
	}
}

/* \brief To set the current language
\param newLanguage Should be short name code found into informations.xml of language file */
void LanguagesManager::setCurrentLanguage(const QString &newLanguage)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+newLanguage);
	//protection for re-set the same language
	if(currentLanguage==newLanguage)
		return;
	//store the language
	plugins->setLanguage(newLanguage);
	//unload the old language
	if(currentLanguage!="en")
	{
		int indexTranslator=0;
		while(indexTranslator<installedTranslator.size())
		{
			QCoreApplication::removeTranslator(installedTranslator[indexTranslator]);
			delete installedTranslator[indexTranslator];
			indexTranslator++;
		}
		installedTranslator.clear();
	}
	if(newLanguage=="en")
	{
		currentLanguage="en";
		emit newLanguageLoaded(currentLanguage);
	}
	else
	{
		int index=0;
		while(index<LanguagesAvailableList.size())
		{
			if(LanguagesAvailableList.at(index).mainShortName==newLanguage)
			{
				//load the new language
				if(newLanguage!="en")
				{
					QTranslator *temp;
					QStringList fileToLoad;
					//load the language main
					fileToLoad<<LanguagesAvailableList.at(index).path+"translation.qm";
					//load the language plugin
					QList<PluginsAvailable> listLoadedPlugins=plugins->getPlugins();
					int indexPluginIndex=0;
					while(indexPluginIndex<listLoadedPlugins.size())
					{
						if(listLoadedPlugins.at(indexPluginIndex).category!=PluginType_Languages)
						{
							QString tempPath=listLoadedPlugins.at(indexPluginIndex).path+"Languages"+QDir::separator()+LanguagesAvailableList.at(index).mainShortName+QDir::separator()+"translation.qm";
							if(QFile::exists(tempPath))
								fileToLoad<<tempPath;
						}
						indexPluginIndex++;
					}
					int indexTranslationFile=0;
					while(indexTranslationFile<fileToLoad.size())
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Translation to load: "+fileToLoad.at(indexTranslationFile));
						temp=new QTranslator();
						if(!temp->load(fileToLoad.at(indexTranslationFile)) || temp->isEmpty())
						{
							delete temp;
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the translation file: "+fileToLoad.at(indexTranslationFile));
						}
						else
						{
							QCoreApplication::installTranslator(temp);
							installedTranslator<<temp;
						}
						indexTranslationFile++;
					}
					temp=new QTranslator();
					if(temp->load(QString("qt_")+newLanguage, QLibraryInfo::location(QLibraryInfo::TranslationsPath)) && !temp->isEmpty())
					{
						QCoreApplication::installTranslator(temp);
						installedTranslator<<temp;
					}
					else
					{
						if(!temp->load(LanguagesAvailableList.at(index).path+"qt.qm") || temp->isEmpty())
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the translation file: qt.qm, into: "+LanguagesAvailableList.at(index).path);
							delete temp;
						}
						else
						{
							QCoreApplication::installTranslator(temp);
							installedTranslator<<temp;
						}
					}
				}
				currentLanguage=newLanguage;
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"emit newLanguageLoaded()");
				emit newLanguageLoaded(currentLanguage);
				return;
			}
			index++;
		}
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to found language: "+newLanguage+", LanguagesAvailableList.size(): "+QString::number(LanguagesAvailableList.size()));
	}
}

/// \brief check if short name is found into language
QString LanguagesManager::getMainShortName(const QString &shortName)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<LanguagesAvailableList.size())
	{
		if(LanguagesAvailableList.at(index).shortName.contains(shortName) || LanguagesAvailableList.at(index).fullName.contains(shortName))
			return LanguagesAvailableList.at(index).mainShortName;
		index++;
	}
	return "";
}

/// \brief load the language in languagePath
void LanguagesManager::allPluginIsLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	setCurrentLanguage(getTheRightLanguage());
}

/*QString LanguagesManager::getMainShortName()
{
	return currentLanguage;
}*/

void LanguagesManager::onePluginAdded(const PluginsAvailable &plugin)
{
	if(plugin.category!=PluginType_Languages)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QDomElement child = plugin.categorySpecific.firstChildElement("fullName");
	LanguagesAvailable temp;
	if(!child.isNull() && child.isElement())
		temp.fullName=child.text();
	child = plugin.categorySpecific.firstChildElement("shortName");
	while(!child.isNull())
	{
		if(child.isElement())
		{
			if(child.hasAttribute("mainCode") && child.attribute("mainCode")=="true")
				temp.mainShortName=child.text();
			temp.shortName<<child.text();
		}
		child = child.nextSiblingElement("shortName");
	}
	temp.path=plugin.path;
	if(temp.fullName.isEmpty())
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"fullName empty for: "+plugin.path);
	else if(temp.path.isEmpty())
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"path empty for: "+plugin.path);
	else if(temp.mainShortName.isEmpty())
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"mainShortName empty for: "+plugin.path);
	else if(temp.shortName.size()<=0)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"temp.shortName.size()<=0 for: "+plugin.path);
	else if(!QFile::exists(temp.path+"flag.png"))
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"flag file not found for: "+plugin.path);
	else if(!QFile::exists(temp.path+"translation.qm") && temp.mainShortName!="en")
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"translation not found for: "+plugin.path);
	else
		LanguagesAvailableList<<temp;
	if(plugins->allPluginHaveBeenLoaded())
		setCurrentLanguage(getTheRightLanguage());
}

void LanguagesManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<LanguagesAvailableList.size())
	{
		if(plugin.path==LanguagesAvailableList.at(index).path)
		{
			return;
		}
		index++;
	}
}

void LanguagesManager::newOptionValue(const QString &group)
{
	if(group=="Language")
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"group: "+group);
		setCurrentLanguage(getTheRightLanguage());
	}
}
