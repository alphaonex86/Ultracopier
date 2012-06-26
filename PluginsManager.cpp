/** \file PluginsManager.cpp
\brief Define the class to manage and load the plugins
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */ 

#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>

#include "PluginsManager.h"

/// \brief Create the manager and load the defaults variables
PluginsManager::PluginsManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the overall instance
	pluginLoaded=false;
	resources=ResourcesManager::getInstance();
	options=OptionEngine::getInstance();

	language="en";
	stopIt=false;
	editionSemList.release();
	checkPluginThread=new AuthPlugin();
	englishPluginType << "CopyEngine" << "Languages" << "Listener" << "PluginLoader" << "SessionLoader" << "Themes";
	//catPlugin << tr("CopyEngine") << tr("Languages") << tr("Listener") << tr("PluginLoader") << tr("SessionLoader") << tr("Themes");
	importingPlugin=false;
	connect(&decodeThread,		SIGNAL(decodedIsFinish()),		this,				SLOT(decodingFinished()),Qt::QueuedConnection);
	connect(checkPluginThread,	SIGNAL(authentifiedPath(QString)),	this,				SLOT(newAuthPath(QString)),Qt::QueuedConnection);
	connect(this,			SIGNAL(finished()),			this,				SLOT(post_operation()),Qt::QueuedConnection);
	connect(this,			SIGNAL(newLanguageLoaded()),		&pluginInformationWindows,	SLOT(retranslateInformation()),Qt::QueuedConnection);
//	connect(this,			SIGNAL(pluginListingIsfinish()),	options,SLOT(setInterfaceValue()));
	//load the plugins list
	/// \bug bug when I put here: moveToThread(this);, due to the direction connection to remove the plugin
	start();
}

/// \brief Destroy the manager
PluginsManager::~PluginsManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	int loop_size=pluginsList.size();
	while(index<loop_size)
	{
		emit onePluginWillBeUnloaded(pluginsList.at(index));
		index++;
	}
	stopIt=true;
	if(this->isRunning())
		this->wait(0);
	delete checkPluginThread;
	OptionEngine::destroyInstanceAtTheLastCall();
	ResourcesManager::destroyInstanceAtTheLastCall();
}

/// \brief set current language
void PluginsManager::setLanguage(const QString &language)
{
	this->language=language;
}

void PluginsManager::post_operation()
{
	pluginLoaded=true;
	emit pluginListingIsfinish();
}

bool PluginsManager::allPluginHaveBeenLoaded()
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
	//load the path and plugins into the path
	QStringList readPath;
	readPath << resources->getReadPath();
	pluginsList.clear();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"pluginsList.size(): "+QString::number(pluginsList.size()));
	foreach(QString basePath,readPath)
	{
		foreach(QString dirSub,englishPluginType)
		{
			QString pluginComposed=basePath+dirSub+QDir::separator();
			QDir dir(pluginComposed);
			if(stopIt)
				return;
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"search plugin into: "+pluginComposed);
			if(dir.exists())
			{
				foreach(QString dirName, dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
				{
					if(stopIt)
						return;
					loadPluginInformation(pluginComposed+dirName+QDir::separator());
				}
			}
		}
	}
	#ifdef ULTRACOPIER_DEBUG
	int index=0;
	int loop_size=pluginsList.size();
	while(index<loop_size)
	{
		QString category=categoryToString(pluginsList.at(index).category);
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Plugin "+QString::number(index)+" loaded ("+category+"): "+pluginsList.at(index).path);
		index++;
	}
	#endif
	checkPluginThread->loadSearchPath(readPath,englishPluginType);
	checkPluginThread->stop();
	checkPluginThread->start();
	checkDependencies();
}

QString PluginsManager::categoryToString(const PluginType &category)
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
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"cat text not found");
			return "Unknow";
		break;
	}
}

QString PluginsManager::categoryToTranslation(const PluginType &category)
{
	return pluginInformationWindows.categoryToTranslation(category);
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

bool PluginsManager::loadPluginInformation(const QString &path)
{
	PluginsAvailable tempPlugin;
	tempPlugin.isAuth	= false;
	tempPlugin.path	= path;
	tempPlugin.category	= PluginType_Unknow;
	QDir pluginPath(path);
	if(pluginPath.cdUp() && pluginPath.cdUp() &&
			resources->getWritablePath()!="" &&
			pluginPath==QDir(resources->getWritablePath()))
		tempPlugin.isWritable=true;
	else
		tempPlugin.isWritable=false;
	QFile xmlMetaData(path+"informations.xml");
	if(xmlMetaData.exists())
	{
		if(xmlMetaData.open(QIODevice::ReadOnly))
		{
			loadPluginXml(&tempPlugin,xmlMetaData.readAll());
			xmlMetaData.close();
		}
		else
		{
			tempPlugin.errorString=tr("informations.xml is not accessible");
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"informations.xml is not accessible into the plugin: "+path);
		}
	}
	else
	{
		tempPlugin.errorString=tr("informations.xml not found into the plugin");
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"informations.xml not found into the plugin: "+path);
	}
	editionSemList.acquire();
	pluginsList << tempPlugin;
	if(tempPlugin.errorString=="")
		pluginsListIndexed.insert(tempPlugin.category,tempPlugin);
	editionSemList.release();
	if(tempPlugin.errorString=="")
	{
		emit onePluginAdded(tempPlugin);
		return true;
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Error detected, the not loaded: "+tempPlugin.errorString+", for path: "+tempPlugin.path);
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
		thePlugin->errorString=tr("%1, parse error at line %2, column %3: %4").arg("informations.xml").arg(errorLine).arg(errorColumn).arg(errorStr);
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("%1, Parse error at line %2, column %3: %4").arg("informations.xml").arg(errorLine).arg(errorColumn).arg(errorStr));
	}
	else
	{
		QDomElement root = domDocument.documentElement();
		if (root.tagName() != "package")
		{
			thePlugin->errorString=tr("\"package\" root tag not found for the xml file");
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"\"package\" root balise not found for the xml file");
		}
		//load the variable
		if(thePlugin->errorString=="")
			loadBalise(root,"title",&(thePlugin->informations),&(thePlugin->errorString),true,true,true);
		if(thePlugin->errorString=="")
			loadBalise(root,"website",&(thePlugin->informations),&(thePlugin->errorString),false,true);
		if(thePlugin->errorString=="")
			loadBalise(root,"description",&(thePlugin->informations),&(thePlugin->errorString),true,true,true);
		if(thePlugin->errorString=="")
			loadBalise(root,"author",&(thePlugin->informations),&(thePlugin->errorString),true,false);
		if(thePlugin->errorString=="")
			loadBalise(root,"pubDate",&(thePlugin->informations),&(thePlugin->errorString),true,false);
		if(thePlugin->errorString=="")
		{
			loadBalise(root,"version",&(thePlugin->informations),&(thePlugin->errorString),true,false);
			if(thePlugin->errorString=="")
				thePlugin->version=thePlugin->informations.last().last();
		}
		if(thePlugin->errorString=="")
		{
			loadBalise(root,"category",&(thePlugin->informations),&(thePlugin->errorString),true,false);
			if(thePlugin->errorString=="")
			{
				QString tempCat=thePlugin->informations.last().last();
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
					thePlugin->errorString="Unknow category: "+QString::number((int)thePlugin->category);
				if(thePlugin->errorString.isEmpty())
				{
					if(thePlugin->category!=PluginType_Languages)
					{
						loadBalise(root,"architecture",&(thePlugin->informations),&(thePlugin->errorString),true,false);
						if(thePlugin->errorString=="")
						{
							#ifndef ULTRACOPIER_VERSION_PORTABLE
							if(thePlugin->informations.last().last()!=ULTRACOPIER_PLATFORM_CODE)
								thePlugin->errorString="Wrong platform code: "+thePlugin->informations.last().last();
							#endif
						}
					}
				}
			}
		}
		if(thePlugin->errorString=="")
		{
			loadBalise(root,"name",&(thePlugin->informations),&(thePlugin->errorString),true,false);
			if(thePlugin->errorString=="")
			{
				thePlugin->name=thePlugin->informations.last().last();
				int index=0;
				int loop_size=pluginsList.size();
				int sub_index,loop_sub_size;
				while(index<loop_size)
				{
					sub_index=0;
					loop_sub_size=pluginsList.at(index).informations.size();
					while(sub_index<loop_sub_size)
					{
						if(pluginsList.at(index).informations.at(sub_index).first()=="name" &&
								pluginsList.at(index).name==thePlugin->name &&
								pluginsList.at(index).category==thePlugin->category)
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Plugin duplicate found ("+QString::number((int)thePlugin->category)+"/"+pluginsList.at(index).informations.at(sub_index).last()+"), already loaded, actual version skipped: "+thePlugin->version);
							thePlugin->errorString=tr("Duplicated plugin found, already loaded!");
							break;
							break;
						}
						sub_index++;
					}
					index++;
				}
			}
		}
		if(thePlugin->errorString=="")
			loadBalise(root,"dependencies",&(thePlugin->informations),&(thePlugin->errorString),true,false);
		if(thePlugin->errorString=="")
		{
			QDomElement child = root.firstChildElement("categorySpecific");
			if(!child.isNull() && child.isElement())
				thePlugin->categorySpecific=child;
		}
	}
}

/// \brief to load the multi-language balise
void PluginsManager::loadBalise(const QDomElement &root,const QString &name,QList<QStringList> *informations,QString *errorString,bool needHaveOneEntryMinimum,bool multiLanguage,bool englishNeedBeFound)
{
	int foundElement=0;
	bool englishTextIsFoundForThisChild=false;
	QDomElement child = root.firstChildElement(name);
	while(!child.isNull())
	{
		if(child.isElement())
		{
			QStringList newInformations;
			if(multiLanguage)
			{
				if(child.hasAttribute("xml:lang"))
				{
					if(child.attribute("xml:lang")=="en")
						englishTextIsFoundForThisChild=true;
					foundElement++;
					newInformations << child.tagName() << child.attribute("xml:lang") << child.text();
				}
				else
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Have not the attribute xml:lang: child.tagName(): %1, child.text(): %2").arg(child.tagName()).arg(child.text()));
			}
			else
			{
				foundElement++;
				newInformations << child.tagName() << child.text();
			}
			*informations << newInformations;
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Is not Element: child.tagName(): %1").arg(child.tagName()));
		child = child.nextSiblingElement(name);
	}
	if(multiLanguage && englishTextIsFoundForThisChild==false && englishNeedBeFound)
	{
		informations->clear();
		*errorString=tr("English text missing into the informations.xml for the tag: %1").arg(name);
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("English text missing into the informations.xml for the tag: %1").arg(name));
		return;
	}
	if(needHaveOneEntryMinimum && foundElement==0)
	{
		informations->clear();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Tag not found: %1").arg(name));
		*errorString=tr("Tag not found: %1").arg(name);
	}
}

/// \brief to load the get dom specific
QString PluginsManager::getDomSpecific(const QDomElement &root,const QString &name,const QList<QPair<QString,QString> > &listChildAttribute)
{
	QDomElement child = root.firstChildElement(name);
	int index,loop_size;
	bool allIsFound;
	while(!child.isNull())
	{
		if(child.isElement())
		{
			allIsFound=true;
			index=0;
			loop_size=listChildAttribute.size();
			while(index<loop_size)
			{
				if(child.attribute(listChildAttribute.at(index).first)!=listChildAttribute.at(index).second)
				{
					allIsFound=false;
					break;
				}
				index++;
			}
			if(allIsFound)
				return child.text();
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Is not Element: child.tagName(): %1").arg(child.tagName()));
		child = child.nextSiblingElement(name);
	}
	return QString();
}

/// \brief to load the get dom specific
QString PluginsManager::getDomSpecific(const QDomElement &root,const QString &name)
{
	QDomElement child = root.firstChildElement(name);
	while(!child.isNull())
	{
		if(child.isElement())
				return child.text();
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Is not Element: child.tagName(): %1").arg(child.tagName()));
		child = child.nextSiblingElement(name);
	}
	return QString();
}

/// \brief check the dependencies
void PluginsManager::checkDependencies()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	int loop_size=pluginsList.size();
	int sub_index,loop_sub_size,resolv_size,indexOfDependencies;
	bool depCheck;
	while(index<loop_size)
	{
		sub_index=0;
		loop_sub_size=pluginsList.at(index).informations.size();
		while(sub_index<loop_sub_size)
		{
			if(pluginsList.at(index).informations.at(sub_index).size()==2 && pluginsList.at(index).informations.at(sub_index).at(0)=="dependencies")
			{
				QString dependencies=pluginsList.at(index).informations.at(sub_index).at(1);
				dependencies=dependencies.replace(QRegExp("[\n\r]+"),"&&");
				dependencies=dependencies.replace(QRegExp("[ \t]+"),"");
				dependencies=dependencies.replace(QRegExp("(&&)+"),"&&");
				dependencies=dependencies.replace(QRegExp("^&&"),"");
				dependencies=dependencies.replace(QRegExp("&&$"),"");
				QStringList dependenciesToResolv=dependencies.split(QRegExp("(&&|\\|\\||\\(|\\))"),QString::SkipEmptyParts);
				indexOfDependencies=0;
				resolv_size=dependenciesToResolv.size();
				while(indexOfDependencies<resolv_size)
				{
					QString dependenciesToParse=dependenciesToResolv.at(indexOfDependencies);
					if(!dependenciesToParse.contains(QRegExp("^(<=|<|=|>|>=)[a-zA-Z0-9\\-]+-([0-9]+\\.)*[0-9]+$")))
					{
						pluginsList[index].informations.clear();
						pluginsList[index].errorString=tr("Dependencies part is wrong");
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Dependencies part is wrong: %1").arg(dependenciesToParse));
						break;
					}
					QString partName=dependenciesToParse;
					partName=partName.remove(QRegExp("(<=|<|=|>|>=)"));
					partName=partName.remove(QRegExp("-([0-9]+\\.)*[0-9]+"));
					QString partVersion=dependenciesToParse;
					partVersion=partVersion.remove(QRegExp("(<=|<|=|>|>=)"));
					partVersion=partVersion.remove(QRegExp("[a-zA-Z0-9\\-]+-"));
					QString partComp=dependenciesToParse;
					partComp=partComp.remove(QRegExp("[a-zA-Z0-9\\-]+-([0-9]+\\.)*[0-9]+"));
					//current version soft
					QString pluginVersion=getPluginVersion(partName);
					depCheck=compareVersion(pluginVersion,partComp,partVersion);
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"dependencies to resolv, partName: "+partName+", partVersion: "+partVersion+", partComp: "+partComp+", pluginVersion: "+pluginVersion+", depCheck: "+QString::number(depCheck));
					if(!depCheck)
					{
						pluginsList[index].informations.clear();
						pluginsList[index].errorString=tr("Dependencies %1 are not satisfied, for plugin: %2").arg(dependenciesToParse).arg(pluginsList[index].path);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Dependencies %1 are not satisfied, for plugin: %2").arg(dependenciesToParse).arg(pluginsList[index].path));
						break;
					}
					indexOfDependencies++;
				}
			}
			sub_index++;
		}
		index++;
	}
}

/// \brief get the version
QString PluginsManager::getPluginVersion(const QString &pluginName)
{
	if(pluginName=="ultracopier")
		return ULTRACOPIER_VERSION;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginsList.size())
	{
		QString version,internalName;
		int sub_index=0;
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

/// \brief To compare version
bool PluginsManager::compareVersion(const QString &versionA,const QString &sign,const QString &versionB)
{
	QStringList versionANumber=versionA.split(".");
	QStringList versionBNumber=versionB.split(".");
	int index=0;
	int defaultReturnValue=true;
	if(sign=="<")
		defaultReturnValue=false;
	if(sign==">")
		defaultReturnValue=false;
	while(index<versionANumber.size() && index<versionBNumber.size())
	{
		unsigned int reaNumberA=versionANumber.at(index).toUInt();
		unsigned int reaNumberB=versionBNumber.at(index).toUInt();
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

QList<PluginsAvailable> PluginsManager::getPluginsByCategory(const PluginType &category)
{
	return pluginsListIndexed.values(category);
}

QList<PluginsAvailable> PluginsManager::getPlugins()
{
	QList<PluginsAvailable> list;
	int index=0;
	while(index<pluginsList.size())
	{
		if(pluginsList.at(index).errorString=="")
			list<<pluginsList.at(index);
		index++;
	}
	return list;
}

/// \brief show the information
/// \todo pass plugin info
void PluginsManager::showInformation(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginsList.size())
	{
		if(pluginsList.at(index).path==path)
		{
			pluginInformationWindows.setLanguage(mainShortName);
			pluginInformationWindows.setPlugin(pluginsList.at(index));
			pluginInformationWindows.show();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"item not selected");
}

void PluginsManager::showInformationDoubleClick()
{
//	showInformation(false);
}

void PluginsManager::removeThePluginSelected(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginsList.size())
	{
		if(pluginsList.at(index).path==path)
		{
			QMessageBox::StandardButton reply;
			/// \todo check if previous version as internal, if yes ask if switch to it, else remove and do marked to disable the internal version too
	//		if(pluginsList.at(index).internalVersionAlternative.isEmpty())
				reply = QMessageBox::question(NULL,tr("Remove %1").arg(pluginsList.at(index).name),tr("Are you sure about removing \"%1\" in version %2?").arg(pluginsList.at(index).name).arg(pluginsList.at(index).version),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
	//		else
	//			reply = QMessageBox::question(NULL,tr("Remove %1").arg(getTranslatedText(pluginsList.at(index),"name",mainShortName)),tr("Are you sure to wish remove \"%1\" in version %2 for the internal version %3?").arg(getTranslatedText(pluginsList.at(index),"name",mainShortName)).arg(pluginsList.at(index).version).arg(pluginsList.at(index).internalVersionAlternative),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
			if(reply==QMessageBox::Yes)
			{
				emit onePluginWillBeUnloaded(pluginsList.at(index));
				emit onePluginWillBeRemoved(pluginsList.at(index));
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"remove plugin at this path: "+pluginsList.at(index).path);
				if(!ResourcesManager::removeFolder(pluginsList.at(index).path))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to remove the plugin");
					QMessageBox::critical(NULL,tr("Error"),tr("Error while the removing plugin, please check the rights on the folder: \n%1").arg(pluginsList.at(index).path));
				}
				pluginsListIndexed.remove(pluginsList.at(index).category,pluginsList.at(index));
				pluginsList.removeAt(index);
				checkDependencies();
			}
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"item not selected");
}

void PluginsManager::addPlugin(const ImportBackend &backend)
{
	if(backend==ImportBackend_File)
		excuteTheFileBackendLoader();
}

void PluginsManager::excuteTheFileBackendLoader()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(importingPlugin)
	{
		QMessageBox::information(NULL,tr("Information"),tr("Previous import is in progress..."));
		return;
	}
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Ultracopier plugin"),QString(),tr("Ultracopier plugin (*.urc)"));
	if(fileName!="")
	{
		QFile temp(fileName);
		if(temp.open(QIODevice::ReadOnly))
		{
			importingPlugin=true;
			lunchDecodeThread(temp.readAll());
			temp.close();
		}
		else
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"unable to open the file: "+temp.errorString());
			QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to open the plugin: %1").arg(temp.errorString()));
		}
	}
}

void PluginsManager::lunchDecodeThread(const QByteArray &data)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	decodeThread.setData(data);
	decodeThread.start(QThread::LowestPriority);
}

void PluginsManager::decodingFinished()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(!decodeThread.errorFound())
	{
		QByteArray data=decodeThread.decodedData();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"data.size(): "+QString::number(data.size()));
		QTarDecode tarFile;
		if(!tarFile.decodeData(data))
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"tarFile.errorString(): "+tarFile.errorString());
			QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(tarFile.errorString()));
		}
		else
		{
			QStringList fileList		= tarFile.getFileList();
			QList<QByteArray> dataList	= tarFile.getDataList();
			if(fileList.size()>1)
			{
				QString basePluginArchive="";
				/* block use less for tar?
				if(fileList.at(0).contains(QRegExp("[\\/]")))
				{
					bool folderFoundEveryWhere=true;
					basePluginArchive=fileList.at(0);
					basePluginArchive.remove(QRegExp("[\\/].*$"));
					for (int i = 0; i < list.size(); ++i)
					{
						if(!fileList.at(i).startsWith(basePluginArchive))
						{
							folderFoundEveryWhere=false;
							break;
						}
					}
					if(folderFoundEveryWhere)
					{
						for (int i = 0; i < fileList.size(); ++i)
							fileList[i].remove(0,basePluginArchive.size());
					}
					else
						basePluginArchive="";
				}*/
				PluginsAvailable tempPlugin;
				QString categoryFinal="";
				for (int i = 0; i < fileList.size(); ++i)
					if(fileList.at(i)=="informations.xml")
					{
						loadPluginXml(&tempPlugin,dataList.at(i));
						break;
					}
				if(tempPlugin.errorString=="")
				{
					categoryFinal=categoryToString(tempPlugin.category);
					if(categoryFinal!="")
					{
						QString writablePath=resources->getWritablePath();
						if(writablePath!="")
						{
							QDir dir;
							QString finalPluginPath=writablePath+categoryFinal+QDir::separator()+tempPlugin.name+QDir::separator();
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"writablePath: \""+writablePath+"\"");
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"basePluginArchive: \""+basePluginArchive+"\"");
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"categoryFinal: \""+categoryFinal+"\"");
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"finalPluginPath: \""+finalPluginPath+"\"");
							if(!dir.exists(finalPluginPath))
							{
								bool errorFound=false;
								for (int i = 0; i < fileList.size(); ++i)
								{
									ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"file "+QString::number(i)+": "+finalPluginPath+fileList.at(i));
									fileList[i].remove(QRegExp("^(..?[\\/])+"));
									QFile currentFile(finalPluginPath+fileList.at(i));
									QFileInfo info(currentFile);
									if(!dir.exists(info.absolutePath()))
										if(!dir.mkpath(info.absolutePath()))
										{
											resources->disableWritablePath();
											ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to make the path: "+info.absolutePath());
											QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to create a folder to install the plugin:\n%1").arg(info.absolutePath()));
											errorFound=true;
											break;
										}
									if(currentFile.open(QIODevice::ReadWrite))
									{
										currentFile.write(dataList.at(i));
										currentFile.close();
									}
									else
									{
										resources->disableWritablePath();
										ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to make the file: "+info.absolutePath()+", error:"+currentFile.errorString());
										QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to create a file to install the plugin:\n%1\nsince:%2").arg(info.absolutePath()).arg(currentFile.errorString()));
										errorFound=true;
										break;
									}
								}
								if(!errorFound)
								{
									if(loadPluginInformation(finalPluginPath))
									{
										ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"push the new plugin into the real list");
										checkDependencies();
										emit needLangToRefreshPluginList();
									}
								}
							}
							else
							{
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Folder with same name is present, skip the plugin installation: "+finalPluginPath);
								QMessageBox::critical(NULL,tr("Plugin loader"),tr("Folder with same name is present, skip the plugin installation:\n%1").arg(finalPluginPath));
							}
						}
						else
						{
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Have not writable path, then how add you plugin?");
							QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
						}
					}
					else
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"category into informations.xml not found!");
						QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
					}
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Error in the xml: "+tempPlugin.errorString);
					QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(tempPlugin.errorString));
				}
			}
			else
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"No file found into the plugin");
				QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it"));
			}
		}
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"decodeThread.errorFound(), error: "+decodeThread.errorString());
		QMessageBox::critical(NULL,tr("Plugin loader"),tr("Unable to load the plugin content, please check it: %1").arg(decodeThread.errorString()));
	}
	importingPlugin=false;
}

void PluginsManager::newAuthPath(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginsList.size())
	{
		if(pluginsList.at(index).path==path)
		{
			pluginsList[index].isAuth=true;
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"plugin not located");
}

/// \brief transfor short plugin name into file name
QString PluginsManager::getResolvedPluginName(const QString &name)
{
	#if defined(Q_OS_LINUX)
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
		#error "Platform not supported"
	#endif
}

bool operator==(PluginsAvailable pluginA,PluginsAvailable pluginB)
{
	return PluginsManager::isSamePlugin(pluginA,pluginB);
}
