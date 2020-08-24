/** \file PluginsManager.cpp
\brief Define the class to manage and load the plugins
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>

#include <iterator>

#include "PluginsManager.h"
#include "cpp11addition.h"
#include "FacilityEngine.h"

/// \brief Create the manager and load the defaults variables
PluginsManager::PluginsManager()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //load the overall instance
    pluginLoaded        = false;
    language            = "en";
    stopIt              = false;
    pluginInformation   = NULL;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    importingPlugin     = false;
    #endif
    editionSemList.release();
    englishPluginType.push_back("CopyEngine");englishPluginType.push_back("Languages");englishPluginType.push_back("Listener");englishPluginType.push_back("PluginLoader");englishPluginType.push_back("SessionLoader");englishPluginType.push_back("Themes");
    //catPlugin << tr("CopyEngine") << tr("Languages") << tr("Listener") << tr("PluginLoader") << tr("SessionLoader") << tr("Themes");
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    connect(&decodeThread,		&QXzDecodeThread::decodedIsFinish,		this,				&PluginsManager::decodingFinished,Qt::QueuedConnection);
    #endif
    connect(this,			&PluginsManager::finished,			this,				&PluginsManager::post_operation,Qt::QueuedConnection);
//	connect(this,			&PluginsManager::pluginListingIsfinish,		options,&OptionEngine::setInterfaceValue);
    //load the plugins list

    /// \bug bug when I put here: moveToThread(this);, due to the direction connection to remove the plugin
    start();
}

/// \brief Destroy the manager
PluginsManager::~PluginsManager()
{
    stopIt=true;
    if(pluginInformation!=NULL)
        delete pluginInformation;
    if(this->isRunning())
        this->wait(0);
}

/// \brief set current language
void PluginsManager::setLanguage(const std::string &language)
{
    this->language=language;
}

void PluginsManager::post_operation()
{
    pluginLoaded=true;
    emit pluginListingIsfinish();
}

bool PluginsManager::allPluginHaveBeenLoaded() const
{
    return pluginLoaded;
}

void PluginsManager::lockPluginListEdition()
{
    editionSemList.acquire();
}

void PluginsManager::unlockPluginListEdition()
{
    editionSemList.release();
}

void PluginsManager::run()
{
    regexp_to_clean_1=std::regex("[\n\r]+");
    regexp_to_clean_2=std::regex("[ \t]+");
    regexp_to_clean_3=std::regex("(&&)+");
    regexp_to_clean_4=std::regex("^&&");
    regexp_to_clean_5=std::regex("&&$");
    regexp_to_dep_1=std::regex("(&&|\\|\\||\\(|\\))");
    regexp_to_dep_2=std::regex("^(<=|<|=|>|>=)[a-zA-Z0-9\\-]+-([0-9]+\\.)*[0-9]+$");
    regexp_to_dep_3=std::regex("(<=|<|=|>|>=)");
    regexp_to_dep_4=std::regex("-([0-9]+\\.)*[0-9]+");
    regexp_to_dep_5=std::regex("[a-zA-Z0-9\\-]+-");
    regexp_to_dep_6=std::regex("[a-zA-Z0-9\\-]+-([0-9]+\\.)*[0-9]+");

    //load the path and plugins into the path
    const std::string &separator=FacilityEngine::separator();
    std::vector<std::string> readPath;
    readPath=ResourcesManager::resourcesManager->getReadPath();
    pluginsList.clear();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pluginsList.size(): "+std::to_string(pluginsList.size()));
    foreach(std::string basePath,readPath)
    {
        foreach(std::string dirSub,englishPluginType)
        {
            std::string pluginComposed=basePath+dirSub+separator;
            QDir dir(QString::fromStdString(pluginComposed));
            if(stopIt)
                return;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"search plugin into: "+pluginComposed);
            if(dir.exists())
            {
                foreach(QString dirName, dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
                {
                    if(stopIt)
                        return;
                    loadPluginInformation(pluginComposed+dirName.toStdString()+separator);
                }
            }
        }
    }
    #ifdef ULTRACOPIER_DEBUG
    unsigned int index_debug=0;
    while(index_debug<pluginsList.size())
    {
        std::string category=categoryToString(pluginsList.at(index_debug).category);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Plugin "+std::to_string(index_debug)+" loaded ("+category+"): "+pluginsList.at(index_debug).path);
        index_debug++;
    }
    #endif
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    while(checkDependencies()!=0){};
    #endif
    //QList<PluginsAvailable> list;
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        if(pluginsList.at(index).errorString.empty())
            emit onePluginAdded(pluginsList.at(index));
        index++;
    }
}

std::string PluginsManager::categoryToString(const PluginType &category) const
{
    switch(category)
    {
        case PluginType_CopyEngine:
            return "CopyEngine";
        break;
        case PluginType_Languages:
            return "Languages";
        break;
        case PluginType_Listener:
            return "Listener";
        break;
        case PluginType_PluginLoader:
            return "PluginLoader";
        break;
        case PluginType_SessionLoader:
            return "SessionLoader";
        break;
        case PluginType_Themes:
            return "Themes";
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"cat text not found: "+std::to_string(category));
            return "Unknown";
        break;
    }
}

std::string PluginsManager::categoryToTranslation(const PluginType &category)
{
    if(pluginInformation==NULL)
    {
        pluginInformation=new PluginInformation();
        connect(this,			&PluginsManager::newLanguageLoaded,		pluginInformation,	&PluginInformation::retranslateInformation,Qt::QueuedConnection);
    }
    return pluginInformation->categoryToTranslation(category);
}

bool PluginsManager::isSamePlugin(const PluginsAvailable &pluginA,const PluginsAvailable &pluginB)
{
    /*if(pluginA.category!=pluginB.category)
        return false;*/
    //only this test should be suffisent
    if(pluginA.path!=pluginB.path)
        return false;
    /*if(pluginA.name!=pluginB.name)
        return false;
    if(pluginA.writablePath!=pluginB.writablePath)
        return false;
    if(pluginA.categorySpecific!=pluginB.categorySpecific)
        return false;
    if(pluginA.version!=pluginB.version)
        return false;
    if(pluginA.informations!=pluginB.informations)
        return false;*/
    return true;
}

bool PluginsManager::loadPluginInformation(const std::string &path)
{
    PluginsAvailable tempPlugin;
    tempPlugin.isAuth	= false;
    tempPlugin.path	= path;
    tempPlugin.category	= PluginType_Unknow;
    QDir pluginPath(QString::fromStdString(path));
    if(pluginPath.cdUp() && pluginPath.cdUp() &&
            !ResourcesManager::resourcesManager->getWritablePath().empty() &&
            pluginPath==QDir(QString::fromStdString(ResourcesManager::resourcesManager->getWritablePath())))
        tempPlugin.isWritable=true;
    else
        tempPlugin.isWritable=false;
    QFile xmlMetaData(QString::fromStdString(path)+"informations.xml");
    if(xmlMetaData.exists())
    {
        if(xmlMetaData.open(QIODevice::ReadOnly))
        {
            loadPluginXml(&tempPlugin,xmlMetaData.readAll());
            xmlMetaData.close();
        }
        else
        {
            tempPlugin.errorString=tr("informations.xml is not accessible").toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"informations.xml is not accessible into the plugin: "+path);
        }
    }
    else
    {
        tempPlugin.errorString=tr("informations.xml not found for the plugin").toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"informations.xml not found for the plugin: "+path);
    }
    editionSemList.acquire();
    pluginsList.push_back(tempPlugin);
    if(tempPlugin.errorString.empty())
        pluginsListIndexed[tempPlugin.category].push_back(tempPlugin);
    editionSemList.release();
    if(tempPlugin.errorString.empty())
        return true;
    else
    {
        emit onePluginInErrorAdded(tempPlugin);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error detected, the not loaded: "+tempPlugin.errorString+", for path: "+tempPlugin.path);
        return false;
    }
}

void PluginsManager::loadPluginXml(PluginsAvailable * thePlugin,const QByteArray &xml)
{
    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument domDocument;
    if (!domDocument.setContent(xml, false, &errorStr,&errorLine,&errorColumn))
    {
        thePlugin->errorString=tr("%1, parse error at line %2, column %3: %4").arg("informations.xml").arg(errorLine).arg(errorColumn).arg(errorStr).toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"informations.xml, Parse error at line "+std::to_string(errorLine)+", column "+std::to_string(errorColumn)+": "+errorStr.toStdString());
    }
    else
    {
        QDomElement root = domDocument.documentElement();
        if (root.tagName() != QStringLiteral("package"))
        {
            thePlugin->errorString=tr("\"package\" root tag not found for the xml file").toStdString();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"\"package\" root balise not found for the xml file");
        }
        //load the variable
        if(thePlugin->errorString.empty())
            loadBalise(root,"title",&(thePlugin->informations),&(thePlugin->errorString),true,true,true);
        if(thePlugin->errorString.empty())
            loadBalise(root,"website",&(thePlugin->informations),&(thePlugin->errorString),false,true);
        if(thePlugin->errorString.empty())
            loadBalise(root,"description",&(thePlugin->informations),&(thePlugin->errorString),true,true,true);
        if(thePlugin->errorString.empty())
            loadBalise(root,"author",&(thePlugin->informations),&(thePlugin->errorString),true,false);
        if(thePlugin->errorString.empty())
            loadBalise(root,"pubDate",&(thePlugin->informations),&(thePlugin->errorString),true,false);
        if(thePlugin->errorString.empty())
        {
            loadBalise(root,"version",&(thePlugin->informations),&(thePlugin->errorString),true,false);
            if(thePlugin->errorString.empty())
                thePlugin->version=thePlugin->informations.back().back();
        }
        if(thePlugin->errorString.empty())
        {
            loadBalise(root,"category",&(thePlugin->informations),&(thePlugin->errorString),true,false);
            if(thePlugin->errorString.empty())
            {
                std::string tempCat=thePlugin->informations.back().back();
                if(tempCat=="Languages")
                    thePlugin->category=PluginType_Languages;
                else if(tempCat=="CopyEngine")
                    thePlugin->category=PluginType_CopyEngine;
                else if(tempCat=="Listener")
                    thePlugin->category=PluginType_Listener;
                else if(tempCat=="PluginLoader")
                    thePlugin->category=PluginType_PluginLoader;
                else if(tempCat=="SessionLoader")
                    thePlugin->category=PluginType_SessionLoader;
                else if(tempCat=="Themes")
                    thePlugin->category=PluginType_Themes;
                else
                    thePlugin->errorString="Unknow category: "+std::to_string((int)thePlugin->category);
                if(thePlugin->errorString.empty())
                {
                    if(thePlugin->category!=PluginType_Languages)
                    {
                        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
                        loadBalise(root,"architecture",&(thePlugin->informations),&(thePlugin->errorString),true,false);
                        if(thePlugin->errorString.empty())
                        {
                            const std::string &platform=thePlugin->informations.back().back();
                            if(platform!=ULTRACOPIER_PLATFORM_CODE)
                                thePlugin->errorString="Wrong platform code: "+platform+std::string(" should be ")+ULTRACOPIER_PLATFORM_CODE;
                        }
                        #endif
                    }
                }
            }
        }
        if(thePlugin->errorString.empty())
        {
            loadBalise(root,"name",&(thePlugin->informations),&(thePlugin->errorString),true,false);
            if(thePlugin->errorString.empty())
            {
                thePlugin->name=thePlugin->informations.back().back();
                size_t index=0;
                while(index<pluginsList.size())
                {
                    size_t sub_index=0;
                    while(sub_index<pluginsList.at(index).informations.size())
                    {
                        if(pluginsList.at(index).informations.at(sub_index).front()=="name" &&
                                pluginsList.at(index).name==thePlugin->name &&
                                pluginsList.at(index).category==thePlugin->category)
                        {
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Plugin duplicate found ("+std::to_string((int)thePlugin->category)+"/"+pluginsList.at(index).informations.at(sub_index).back()+"), already loaded, actual version skipped: "+thePlugin->version);
                            thePlugin->errorString=tr("Duplicated plugin found, already loaded!").toStdString();
                            break;
                            break;
                        }
                        sub_index++;
                    }
                    index++;
                }
            }
        }
        if(thePlugin->errorString.empty())
            loadBalise(root,"dependencies",&(thePlugin->informations),&(thePlugin->errorString),true,false);
        if(thePlugin->errorString.empty())
        {
            QDomElement child = root.firstChildElement("categorySpecific");
            if(!child.isNull() && child.isElement())
                thePlugin->categorySpecific=child;
        }
    }
}

/// \brief to load the multi-language balise
void PluginsManager::loadBalise(const QDomElement &root,const std::string &name,std::vector<std::vector<std::string> > *informations,std::string *errorString,bool needHaveOneEntryMinimum,bool multiLanguage,bool englishNeedBeFound)
{
    int foundElement=0;
    bool englishTextIsFoundForThisChild=false;
    QDomElement child = root.firstChildElement(QString::fromStdString(name));
    while(!child.isNull())
    {
        if(child.isElement())
        {
            std::vector<std::string> newInformations;
            if(multiLanguage)
            {
                if(child.hasAttribute(QStringLiteral("xml:lang")))
                {
                    if(child.attribute(QStringLiteral("xml:lang"))==QStringLiteral("en"))
                        englishTextIsFoundForThisChild=true;
                    foundElement++;
                    newInformations.push_back(child.tagName().toStdString());
                    newInformations.push_back(child.attribute(QStringLiteral("xml:lang")).toStdString());
                    newInformations.push_back(child.text().toStdString());
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Have not the attribute xml:lang: child.tagName(): "+child.tagName().toStdString()+", child.text(): "+child.text().toStdString());
            }
            else
            {
                foundElement++;
                newInformations.push_back(child.tagName().toStdString());
                newInformations.push_back(child.text().toStdString());
            }
            informations->push_back(newInformations);
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Is not Element: child.tagName(): "+child.tagName().toStdString());
        child = child.nextSiblingElement(QString::fromStdString(name));
    }
    if(multiLanguage && englishTextIsFoundForThisChild==false && englishNeedBeFound)
    {
        informations->clear();
        *errorString=tr("English text missing in the informations.xml for the tag: %1").arg(QString::fromStdString(name)).toStdString();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"English text missing into the informations.xml for the tag: "+name);
        return;
    }
    if(needHaveOneEntryMinimum && foundElement==0)
    {
        informations->clear();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Tag not found: "+name);
        *errorString=tr("Tag not found: %1").arg(QString::fromStdString(name)).toStdString();
    }
}

/// \brief to load the get dom specific
std::string PluginsManager::getDomSpecific(const QDomElement &root,const std::string &name,const std::vector<std::pair<std::string,std::string> > &listChildAttribute) const
{
    QDomElement child = root.firstChildElement(QString::fromStdString(name));
    bool allIsFound;
    while(!child.isNull())
    {
        if(child.isElement())
        {
            allIsFound=true;
            size_t index=0;
            while(index<listChildAttribute.size())
            {
                const std::pair<std::string,std::string> &entry=listChildAttribute.at(index);
                if(child.attribute(QString::fromStdString(entry.first))!=QString::fromStdString(entry.second))
                {
                    allIsFound=false;
                    break;
                }
                index++;
            }
            if(allIsFound)
                return child.text().toStdString();
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Is not Element: child.tagName(): "+child.tagName().toStdString());
        child = child.nextSiblingElement(QString::fromStdString(name));
    }
    return std::string();
}

/// \brief to load the get dom specific
std::string PluginsManager::getDomSpecific(const QDomElement &root,const std::string &name) const
{
    QDomElement child = root.firstChildElement(QString::fromStdString(name));
    while(!child.isNull())
    {
        if(child.isElement())
                return child.text().toStdString();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Is not Element: child.tagName(): "+child.tagName().toStdString());
        child = child.nextSiblingElement(QString::fromStdString(name));
    }
    return std::string();
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
/// \brief check the dependencies
uint32_t PluginsManager::checkDependencies()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    uint32_t errors=0;
    unsigned int index=0;
    bool depCheck;
    while(index<pluginsList.size())
    {
        unsigned int sub_index=0;
        while(sub_index<pluginsList.at(index).informations.size())
        {
            if(pluginsList.at(index).informations.at(sub_index).size()==2 && pluginsList.at(index).informations.at(sub_index).at(0)=="dependencies")
            {
                std::string dependencies = pluginsList.at(index).informations.at(sub_index).at(1);
                dependencies=std::regex_replace(dependencies, regexp_to_clean_1,"&&");
                dependencies=std::regex_replace(dependencies, regexp_to_clean_2,"");
                dependencies=std::regex_replace(dependencies, regexp_to_clean_3,"&&");
                dependencies=std::regex_replace(dependencies, regexp_to_clean_4,"");
                dependencies=std::regex_replace(dependencies, regexp_to_clean_5,"");
                std::sregex_token_iterator iter(dependencies.begin(), dependencies.end(), regexp_to_dep_1, -1), end;
                for ( ; iter != end; ++iter)
                {
                    std::string dependenciesToParse=trim(*iter);
                    if(dependenciesToParse.empty()) {
                      continue;
                    }
                    if(!std::regex_match(dependenciesToParse, regexp_to_dep_2))
                    {
                        pluginsList[index].informations.clear();
                        pluginsList[index].errorString=tr("Dependencies part is wrong").toStdString();
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Dependencies part is wrong: "+dependenciesToParse);
                        emit onePluginInErrorAdded(pluginsList.at(index));
                        errors++;
                        break;
                    }
                    std::string partName=dependenciesToParse;
                    partName=std::regex_replace(partName, regexp_to_dep_3, "");
                    partName=std::regex_replace(partName, regexp_to_dep_4, "");
                    std::string partVersion=dependenciesToParse;
                    partVersion=std::regex_replace(partVersion, regexp_to_dep_3, "");
                    partVersion=std::regex_replace(partVersion, regexp_to_dep_5, "");
                    std::string partComp=dependenciesToParse;
                    partComp=std::regex_replace(partComp, regexp_to_dep_6, "");
                    //current version soft
                    std::string pluginVersion=getPluginVersion(partName);
                    depCheck=compareVersion(pluginVersion,partComp,partVersion);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"dependencies to resolv, partName: "+partName+", partVersion: "+partVersion+", partComp: "+partComp+", pluginVersion: "+pluginVersion+", depCheck: "+std::to_string(depCheck));
                    if(!depCheck)
                    {
                        pluginsList[index].informations.clear();
                        pluginsList[index].errorString=tr("Dependencies %1 are not satisfied, for plugin: %2").arg(QString::fromStdString(dependenciesToParse)).arg(QString::fromStdString(pluginsList[index].path)).toStdString();
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Dependencies "+dependenciesToParse+" are not satisfied, for plugin: "+pluginsList[index].path);
                        pluginsListIndexed.erase(pluginsListIndexed.find(pluginsList.at(index).category));
                        emit onePluginInErrorAdded(pluginsList.at(index));
                        errors++;
                        break;
                    }
                }
            }
            sub_index++;
        }
        index++;
    }
    return errors;
}
#endif

/// \brief get the version
std::string PluginsManager::getPluginVersion(const std::string &pluginName) const
{
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
     if(pluginName=="supercopier")
         return FacilityEngine::version();
     #else
      if(pluginName=="ultracopier")
          return FacilityEngine::version();
     #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        std::string version,internalName;
        unsigned int sub_index=0;
        while(sub_index<pluginsList.at(index).informations.size())
        {
            if(pluginsList.at(index).informations.at(sub_index).size()==2 && pluginsList.at(index).informations.at(sub_index).at(0)=="version")
                version=pluginsList.at(index).informations.at(sub_index).at(1);
            if(pluginsList.at(index).informations.at(sub_index).size()==2 && pluginsList.at(index).informations.at(sub_index).at(0)=="internalName")
                internalName=pluginsList.at(index).informations.at(sub_index).at(1);
            sub_index++;
        }
        if(internalName==pluginName)
            return version;
        index++;
    }
    return "";
}

/// \brief To compare version, \return true is case of error
bool PluginsManager::compareVersion(const std::string &versionA,const std::string &sign,const std::string &versionB)
{
    std::vector<std::string> versionANumber=stringsplit(versionA,'.');
    std::vector<std::string> versionBNumber=stringsplit(versionB,'.');
    unsigned int index=0;
    unsigned int defaultReturnValue=true;
    if(sign=="<")
        defaultReturnValue=false;
    if(sign==">")
        defaultReturnValue=false;
    bool ok;
    while(index<versionANumber.size() && index<versionBNumber.size())
    {
        unsigned int reaNumberA=stringtouint8(versionANumber.at(index),&ok);
        if(!ok)
            return true;
        unsigned int reaNumberB=stringtouint8(versionBNumber.at(index),&ok);
        if(!ok)
            return true;
        if(sign=="=" && reaNumberA!=reaNumberB)
            return false;
        if(sign=="<")
        {
            if(reaNumberA>reaNumberB)
                return false;
            if(reaNumberA<reaNumberB)
                return true;
        }
        if(sign==">")
        {
            if(reaNumberA<reaNumberB)
                return false;
            if(reaNumberA>reaNumberB)
                return true;
        }
        if(sign=="<=")
        {
            if(reaNumberA>reaNumberB)
                return false;
            if(reaNumberA<reaNumberB)
                return true;
        }
        if(sign==">=")
        {
            if(reaNumberA<reaNumberB)
                return false;
            if(reaNumberA>reaNumberB)
                return true;
        }
        index++;
    }
    return defaultReturnValue;
}

std::vector<PluginsAvailable> PluginsManager::getPluginsByCategory(const PluginType &category) const
{
    if(pluginsListIndexed.find(category)==pluginsListIndexed.cend())
        return std::vector<PluginsAvailable>();
    return pluginsListIndexed.at(category);
}

std::vector<PluginsAvailable> PluginsManager::getPlugins(bool withError) const
{
    std::vector<PluginsAvailable> list;
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        if(withError || pluginsList.at(index).errorString.empty())
            list.push_back(pluginsList.at(index));
        index++;
    }
    return list;
}

/// \brief show the information
void PluginsManager::showInformation(const std::string &path)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        if(pluginsList.at(index).path==path)
        {
            if(pluginInformation==NULL)
            {
                pluginInformation=new PluginInformation();
                connect(this,			&PluginsManager::newLanguageLoaded,		pluginInformation,	&PluginInformation::retranslateInformation,Qt::QueuedConnection);
            }
            pluginInformation->setLanguage(mainShortName);
            pluginInformation->setPlugin(pluginsList.at(index));
            pluginInformation->show();
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"item not selected");
}

void PluginsManager::showInformationDoubleClick()
{
//	showInformation(false);
}

#ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
void PluginsManager::removeThePluginSelected(const std::string &path)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        if(pluginsList.at(index).path==path)
        {
            QMessageBox::StandardButton reply;
    //		if(pluginsList.at(index).internalVersionAlternative.isEmpty())
                reply = QMessageBox::question(NULL,tr("Remove %1").arg(QString::fromStdString(pluginsList.at(index).name)),tr("Are you sure about removing \"%1\" in version %2?").arg(QString::fromStdString(pluginsList.at(index).name)).arg(QString::fromStdString(pluginsList.at(index).version)),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    //		else
    //			reply = QMessageBox::question(NULL,tr("Remove %1").arg(getTranslatedText(pluginsList.at(index),"name",mainShortName)),tr("Are you sure to wish remove \"%1\" in version %2 for the internal version %3?").arg(getTranslatedText(pluginsList.at(index),"name",mainShortName)).arg(pluginsList.at(index).version).arg(pluginsList.at(index).internalVersionAlternative),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
            if(reply==QMessageBox::Yes)
            {
                emit onePluginWillBeUnloaded(pluginsList.at(index));
                emit onePluginWillBeRemoved(pluginsList.at(index));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"remove plugin at this path: "+pluginsList.at(index).path);
                if(!ResourcesManager::removeFolder(pluginsList.at(index).path))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove the plugin");
                    QMessageBox::critical(NULL,tr("Error"),tr("Error while the removing plugin, please check the rights on the folder: \n%1").arg(QString::fromStdString(pluginsList.at(index).path)));
                }
                pluginsListIndexed.erase(pluginsListIndexed.find(pluginsList.at(index).category));
                pluginsList.erase(pluginsList.begin()+index);
                while(checkDependencies()!=0){};
            }
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"item not selected");
}

void PluginsManager::addPlugin(const ImportBackend &backend)
{
    if(backend==ImportBackend_File)
        executeTheFileBackendLoader();
}

void PluginsManager::executeTheFileBackendLoader()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(importingPlugin)
    {
        QMessageBox::information(NULL,tr("Information"),tr("Previous import is in progress..."));
        return;
    }
    std::string fileName = QFileDialog::getOpenFileName(NULL,tr("Open Ultracopier plugin"),QString(),tr("Ultracopier plugin (*.urc)")).toStdString();
    if(fileName!="")
        tryLoadPlugin(fileName);
}

void PluginsManager::tryLoadPlugin(const std::string &file)
{
    QFile temp(QString::fromStdString(file));
    if(temp.open(QIODevice::ReadOnly))
    {
        importingPlugin=true;
        lunchDecodeThread(temp.readAll());
        temp.close();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unable to open the file: "+temp.errorString().toStdString());
        QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to open the plugin: %1").arg(temp.errorString()));
    }
}

void PluginsManager::lunchDecodeThread(const QByteArray &data)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    decodeThread.setData(data);
    decodeThread.start(QThread::LowestPriority);
}

void PluginsManager::decodingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!decodeThread.errorFound())
    {
        QByteArray data=decodeThread.decodedData();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data.size(): "+std::to_string(data.size()));
        QTarDecode tarFile;
        std::vector<char> cppdata;
        cppdata.resize(data.size());
        memcpy(cppdata.data(),data.data(),data.size());
        if(!tarFile.decodeData(cppdata))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"tarFile.errorString(): "+tarFile.errorString());
            QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(QString::fromStdString(tarFile.errorString())));
        }
        else
        {
            std::vector<std::string> fileList		= tarFile.getFileList();
            std::vector<std::vector<char> > dataList	= tarFile.getDataList();
            if(fileList.size()>1)
            {
                std::string basePluginArchive="";
                /* block use less for tar?
                if(fileList.at(0).contains(QRegularExpression("[\\/]")))
                {
                    bool folderFoundEveryWhere=true;
                    basePluginArchive=fileList.at(0);
                    basePluginArchive=std::regex_replace(basePluginArchive, std::regex("[\\/].*$"), "");
                    for (int i = 0; i < list.size(); ++i)
                    {
                        if(!stringStartWith(fileList.at(i),basePluginArchive))
                        {
                            folderFoundEveryWhere=false;
                            break;
                        }
                    }
                    if(folderFoundEveryWhere)
                    {
                        for (int i = 0; i < fileList.size(); ++i)
                            fileList[i].substr(basePluginArchive.size());
                    }
                    else
                        basePluginArchive="";
                }*/
                PluginsAvailable tempPlugin;
                std::string categoryFinal="";
                for (unsigned int i = 0; i < fileList.size(); ++i)
                    if(fileList.at(i)=="informations.xml")
                    {
                        loadPluginXml(&tempPlugin,QByteArray(dataList.at(i).data(),static_cast<int>(dataList.at(i).size())));
                        break;
                    }
                if(tempPlugin.errorString=="")
                {
                    categoryFinal=categoryToString(tempPlugin.category);
                    if(categoryFinal!="")
                    {
                        std::string writablePath=ResourcesManager::resourcesManager->getWritablePath();
                        if(writablePath!="")
                        {
                            QDir dir;
                            std::string finalPluginPath=writablePath+categoryFinal+FacilityEngine::separator()+tempPlugin.name+FacilityEngine::separator();
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"writablePath: \""+writablePath+"\"");
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"basePluginArchive: \""+basePluginArchive+"\"");
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"categoryFinal: \""+categoryFinal+"\"");
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"finalPluginPath: \""+finalPluginPath+"\"");
                            if(!dir.exists(QString::fromStdString(finalPluginPath)))
                            {
                                bool errorFound=false;
                                for (unsigned int i = 0; i < fileList.size(); ++i)
                                {
                                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"file "+std::to_string(i)+": "+finalPluginPath+fileList.at(i));
                                    std::string fileListEntry=fileList[i];
                                    fileListEntry=std::regex_replace(fileListEntry, std::regex("^(..?[\\/])+"), "");
                                    QFile currentFile(QString::fromStdString(finalPluginPath+fileListEntry));
                                    QFileInfo info(currentFile);
                                    if(!dir.exists(info.absolutePath()))
                                        if(!dir.mkpath(info.absolutePath()))
                                        {
                                            ResourcesManager::resourcesManager->disableWritablePath();
                                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Unable to make the path: "+info.absolutePath().toStdString());
                                            QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to create a folder to install the plugin:\n%1").arg(info.absolutePath()));
                                            errorFound=true;
                                            break;
                                        }
                                    if(currentFile.open(QIODevice::ReadWrite))
                                    {
                                        currentFile.write(QByteArray(dataList.at(i).data(),static_cast<int>(dataList.at(i).size())));
                                        currentFile.close();
                                    }
                                    else
                                    {
                                        ResourcesManager::resourcesManager->disableWritablePath();
                                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Unable to make the file: "+info.absolutePath().toStdString()+", error:"+currentFile.errorString().toStdString());
                                        QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to create a file to install the plugin:\n%1\nError:%2").arg(info.absolutePath()).arg(currentFile.errorString()));
                                        errorFound=true;
                                        break;
                                    }
                                }
                                if(!errorFound)
                                {
                                    if(loadPluginInformation(finalPluginPath))
                                    {
                                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"push the new plugin into the real list");
                                        while(checkDependencies()!=0){};
                                        emit needLangToRefreshPluginList();
                                        emit manuallyAdded(tempPlugin);
                                    }
                                }
                            }
                            else
                            {
                                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Folder with same name is present, skip the plugin installation: "+finalPluginPath);
                                QMessageBox::critical(NULL,tr("Plugin loader"),tr("Folder with same name is present, skip the plugin installation:\n%1").arg(QString::fromStdString(finalPluginPath)));
                            }
                        }
                        else
                        {
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Have not writable path, then how add you plugin?");
                            QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
                        }
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"category into informations.xml not found!");
                        QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
                    }
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error in the xml: "+tempPlugin.errorString);
                    QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(QString::fromStdString(tempPlugin.errorString)));
                }
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No file found into the plugin");
                QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
            }
        }
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"decodeThread.errorFound(), error: "+decodeThread.errorString().toStdString());
        QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(decodeThread.errorString()));
    }
    importingPlugin=false;
}
#endif

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void PluginsManager::newAuthPath(const std::string &path)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<pluginsList.size())
    {
        if(pluginsList.at(index).path==path)
        {
            pluginsList[index].isAuth=true;
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"plugin not located");
}
#endif

/// \brief transfor short plugin name into file name
std::string PluginsManager::getResolvedPluginName(const std::string &name)
{
    #if defined(Q_OS_LINUX) || defined(Q_OS_HURD) || defined(__HAIKU__)
        return "lib"+name+".so";
    #elif defined(Q_OS_MAC)
        #if defined(QT_DEBUG)
            return "lib"+name+"_debug.dylib";
        #else
            return "lib"+name+".dylib";
        #endif
    #elif defined(Q_OS_WIN32)
        #if defined(QT_DEBUG)
            return name+"d.dll";
        #else
            return name+".dll";
        #endif
    #else
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        #error "Platform not supported"
        #else
        return std::string();
        #endif
    #endif
}

bool operator==(PluginsAvailable pluginA,PluginsAvailable pluginB)
{
    return PluginsManager::isSamePlugin(pluginA,pluginB);
}
