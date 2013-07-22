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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //load the rest
    QStringList resourcesPaths=ResourcesManager::resourcesManager->getReadPath();
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
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,&LanguagesManager::previouslyPluginAdded,		this,	&LanguagesManager::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,this,	&LanguagesManager::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,	this,	&LanguagesManager::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,		this,	&LanguagesManager::allPluginIsLoaded,Qt::QueuedConnection);
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Languages);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    //load the GUI option
    QList<QPair<QString, QVariant> > KeysList;
    KeysList.append(qMakePair(QString("Language"),QVariant("en")));
    KeysList.append(qMakePair(QString("Language_force"),QVariant(false)));
    OptionEngine::optionEngine->addOptionGroup("Language",KeysList);
//	connect(this,	&LanguagesManager::newLanguageLoaded,			plugins,&PluginsManager::refreshPluginList);
//	connect(this,	&LanguagesManager::newLanguageLoaded,			this,&LanguagesManager::retranslateTheUI);
    connect(OptionEngine::optionEngine,&OptionEngine::newOptionValue,				this,	&LanguagesManager::newOptionValue,Qt::QueuedConnection);
    connect(this,	&LanguagesManager::newLanguageLoaded,			PluginsManager::pluginsManager,&PluginsManager::newLanguageLoaded,Qt::QueuedConnection);
}

/// \brief Destroy the language manager
LanguagesManager::~LanguagesManager()
{
}

/// \brief load the language selected, return the main short code like en, fr, ..
QString LanguagesManager::getTheRightLanguage() const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(LanguagesAvailableList.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"empty combobox list, failing back to english");
        return "en";
    }
    else
    {
        if(!OptionEngine::optionEngine->getOptionValue("Language","Language_force").toBool())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"language auto-detection, QLocale::system().name(): "+QLocale::system().name()+", QLocale::languageToString(QLocale::system().language()): "+QLocale::languageToString(QLocale::system().language()));
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Autodetection of the language failed, QLocale::languageToString(QLocale::system().language()): "+QLocale::languageToString(QLocale::system().language())+", QLocale::system().name(): "+QLocale::system().name()+", failing back to english");
                    return OptionEngine::optionEngine->getOptionValue("Language","Language").toString();
                }
            }
        }
        else
            return OptionEngine::optionEngine->getOptionValue("Language","Language").toString();
    }
}

/* \brief To set the current language
\param newLanguage Should be short name code found into informations.xml of language file */
void LanguagesManager::setCurrentLanguage(const QString &newLanguage)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+newLanguage);
    //protection for re-set the same language
    if(currentLanguage==newLanguage)
        return;
    //store the language
    PluginsManager::pluginsManager->setLanguage(newLanguage);
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
                if(newLanguage=="en")
                    fileToLoad<<":/Languages/en/translation.qm";
                else
                    fileToLoad<<LanguagesAvailableList.at(index).path+"translation.qm";
                //load the language plugin
                QList<PluginsAvailable> listLoadedPlugins=PluginsManager::pluginsManager->getPlugins();
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Translation to load: "+fileToLoad.at(indexTranslationFile));
                    temp=new QTranslator();
                    if(!temp->load(fileToLoad.at(indexTranslationFile)) || temp->isEmpty())
                    {
                        delete temp;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the translation file: "+fileToLoad.at(indexTranslationFile));
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
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the translation file: qt.qm, into: "+LanguagesAvailableList.at(index).path);
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit newLanguageLoaded()");
            emit newLanguageLoaded(currentLanguage);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to found language: "+newLanguage+", LanguagesAvailableList.size(): "+QString::number(LanguagesAvailableList.size()));
}

/// \brief check if short name is found into language
QString LanguagesManager::getMainShortName(const QString &shortName) const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    setCurrentLanguage(getTheRightLanguage());
}

const QString LanguagesManager::autodetectedLanguage() const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"language auto-detection, QLocale::system().name(): "+QLocale::system().name()+", QLocale::languageToString(QLocale::system().language()): "+QLocale::languageToString(QLocale::system().language()));
    QString tempLanguage=getMainShortName(QLocale::languageToString(QLocale::system().language()));
    if(tempLanguage!="")
        return tempLanguage;
    else
    {
        tempLanguage=getMainShortName(QLocale::system().name());
        if(tempLanguage!="")
            return tempLanguage;
    }
    return "";
}

void LanguagesManager::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Languages)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"fullName empty for: "+plugin.path);
    else if(temp.path.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"path empty for: "+plugin.path);
    else if(temp.mainShortName.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"mainShortName empty for: "+plugin.path);
    else if(temp.shortName.size()<=0)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"temp.shortName.size()<=0 for: "+plugin.path);
    else if(!QFile::exists(temp.path+"flag.png"))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flag file not found for: "+plugin.path);
    else if(!QFile::exists(temp.path+"translation.qm") && temp.mainShortName!="en")
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"translation not found for: "+plugin.path);
    else
        LanguagesAvailableList<<temp;
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        setCurrentLanguage(getTheRightLanguage());
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void LanguagesManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
#endif

void LanguagesManager::newOptionValue(const QString &group)
{
    if(group=="Language")
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"group: "+group);
        setCurrentLanguage(getTheRightLanguage());
    }
}
