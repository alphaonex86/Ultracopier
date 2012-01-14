/** \file ResourcesManager.cpp
\brief Define the class to manage and load the resources linked with the themes
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#include <QDir>
#include <QFile>
#include <QApplication>
#include <QFSFileEngine>
#include <QFileInfo>


#include "ResourcesManager.h"

/// \brief Create the manager and load the defaults variables
ResourcesManager::ResourcesManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the internal path
	searchPath<<QString(":/");
	//load the user path but only if exists and writable
	//load the ultracopier path
	#ifdef ULTRACOPIER_VERSION_PORTABLE
		#ifdef ULTRACOPIER_VERSION_PORTABLEAPPS
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Ultracopier is compiled with the flag: ULTRACOPIER_VERSION_PORTABLEAPPS");
			//load the data folder path
			QDir dir(QApplication::applicationDirPath());
			dir.cdUp();
			dir.cdUp();
			dir.cd("Data");
			searchPath<<ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
			writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		#else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Ultracopier is compiled with the flag: ULTRACOPIER_VERSION_PORTABLE");
			//load the ultracopier path
			QDir dir(QApplication::applicationDirPath());
			dir.cd("Data");
			searchPath<<ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
			writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		#endif
	#else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Ultracopier is compiled as user privacy mode");
		#ifdef Q_OS_WIN32
			#define EXTRA_HOME_PATH "\\ultracopier\\"
		#else
			#define EXTRA_HOME_PATH "/.config/Ultracopier/"
		#endif
		#ifdef Q_OS_LINUX
			QDir linuxDir("/usr/share/ultracopier/");
			if(linuxDir.exists())
				searchPath<<"/usr/share/ultracopier/";
		#endif
		//load the user path but only if exists and writable
		QDir dir(QFSFileEngine::homePath()+EXTRA_HOME_PATH);
		if(dir.exists())
		{
			writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
			searchPath<<ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		} //if not exists try to create it
		else if(dir.mkpath(dir.absolutePath()))
		{
			//if created, then have write permissions
			writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
			searchPath<<ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		}
		//load the ultracopier path
		searchPath<<ResourcesManager::AddSlashIfNeeded(QApplication::applicationDirPath());
	#endif
	searchPath.removeDuplicates();
	#ifdef ULTRACOPIER_DEBUG
	index=0;
	loop_size=searchPath.size();
	while(index<loop_size) //look at each val
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"searchPath.at("+QString::number(index)+"): "+searchPath.at(index));
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"writablePath: "+writablePath);
	#endif // ULTRACOPIER_DEBUG
}

/// \brief Destroy the resource manager
ResourcesManager::~ResourcesManager()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
}

/// \brief Get folder presence and the path
QString ResourcesManager::getFolderReadPath(QString path)
{
	index=0;
	loop_size=searchPath.size();
	while(index<loop_size) //look at each val
	{
		QDir dir(searchPath.at(index)+path);
		if(dir.exists()) // if the path have been found, then return the full path
			return ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		index++;
	}
	return "";
}

/// \brief Get folder presence, the path and check in the folder and sub-folder the file presence
QString ResourcesManager::getFolderReadPathMultiple(QString path,QStringList fileToCheck)
{
	index=0;
	loop_size=searchPath.size();
	while(index<loop_size) //look at each val
	{
		QDir dir(searchPath.at(index)+path);
		if(checkFolderContent(dir.absolutePath(),fileToCheck))
			return dir.absolutePath()+QDir::separator();
		index++;
	}
	return "";
}

bool ResourcesManager::checkFolderContent(QString path,QStringList fileToCheck)
{
	QDir dir(path);
	if(dir.exists()) // if the path have been found, then return the full path
	{
		bool allFileToCheckIsFound=true;
		index=0;
		loop_size=fileToCheck.size();
		QString partialPath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath());
		while(index<loop_size) //look at each val
		{
			if(!QFile::exists(partialPath+fileToCheck.at(index))) //if a file have been not found, consider the folder as not suitable
			{
				allFileToCheckIsFound=false;
				break;
			}
			index++;
		}
		if(allFileToCheckIsFound==true) // if all file into have been found then return this path
			return true;
	}
	return false;
}

/// \brief add / or \ in function of the platform at the end of path if both / and \ are not found
QString ResourcesManager::AddSlashIfNeeded(QString path)
{
	if(path.contains(QRegExp("[/\\\\]$")))
		return path;
	else
		return path+QDir::separator();
}

/// \brief get the writable path
QString ResourcesManager::getWritablePath()
{
	return writablePath;
}

/// \brief disable the writable path, if ultracopier is unable to write into
bool ResourcesManager::disableWritablePath()
{
	bool returnVal=true;
	if(writablePath=="")
		returnVal=false;
	else
		writablePath="";
	return returnVal;
}

/// \brief get the read path
QStringList ResourcesManager::getReadPath()
{
	return searchPath;
}

/// \brief remove folder
bool ResourcesManager::removeFolder(QString dir)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"folder to remove: "+dir);
	bool errorFound=false;
	QDir currentDir(dir);
	QFileInfoList files = currentDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
	int index=0;
	int loop_size=files.size();
	while(index<loop_size)
	{
		if(files.at(index).isFile())
		{
			QFile file(files.at(index).absoluteFilePath());
			if(!file.remove())
			{
				errorFound=true;
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"remove file failed: "+file.errorString());
			}
			else
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"file removed: "+file.fileName());
		}
		else if(files.at(index).isDir())
			removeFolder(files.at(index).absoluteFilePath());
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unknow file type for: "+files.at(index).absoluteFilePath());
		index++;
	}
	if(!currentDir.rmpath(dir))
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"remove path failed, check right and if is empty: "+dir);
		errorFound=true;
	}
	return !errorFound;
}
