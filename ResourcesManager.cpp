/** \file ResourcesManager.cpp
\brief Define the class to manage and load the resources linked with the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDir>
#include <QFile>
#include <QApplication>
#include <QFileInfo>

#include "cpp11addition.h"
#include "ResourcesManager.h"
#include "FacilityEngine.h"

std::regex ResourcesManager::slashEnd;

/// \brief Create the manager and load the defaults variables
ResourcesManager::ResourcesManager()
{
    slashEnd=std::regex("[/\\\\]$");

    //load the internal path
    searchPath.push_back(":/");
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        //load the user path but only if exists and writable
        //load the ultracopier path
        #ifdef ULTRACOPIER_VERSION_PORTABLE
            #ifdef ULTRACOPIER_VERSION_PORTABLEAPPS
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Ultracopier is compiled with the flag: ULTRACOPIER_VERSION_PORTABLEAPPS");
                //load the data folder path
                QDir dir(QApplication::applicationDirPath());
                dir.cdUp();
                dir.cdUp();
                dir.cd(QStringLiteral("Data");
                searchPath.push_back(ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString()));
                writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
            #else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Ultracopier is compiled with the flag: ULTRACOPIER_VERSION_PORTABLE");
                //load the ultracopier path
                QDir dir(QApplication::applicationDirPath());
                dir.cd(QStringLiteral("Data"));
                searchPath.push_back(ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString()));
                writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
            #endif
        #else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Ultracopier is compiled as user privacy mode");
            #ifdef Q_OS_WIN32
                #define EXTRA_HOME_PATH QStringLiteral("\\ultracopier\\")
            #else
                #define EXTRA_HOME_PATH QStringLiteral("/.config/Ultracopier/")
            #endif
            #ifdef Q_OS_LINUX
                QDir linuxArchIndepDir(QStringLiteral("/usr/share/ultracopier/"));
                if(linuxArchIndepDir.exists())
                    searchPath.push_back(ResourcesManager::AddSlashIfNeeded(linuxArchIndepDir.absolutePath().toStdString()));
                QDir linuxPluginsDir(QStringLiteral("/usr/lib/ultracopier/"));
                if(linuxPluginsDir.exists())
                    searchPath.push_back(ResourcesManager::AddSlashIfNeeded(linuxPluginsDir.absolutePath().toStdString()));
            #endif
            //load the user path but only if exists and writable
            QDir dir(QDir::homePath()+EXTRA_HOME_PATH);
            if(dir.exists())
            {
                writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
                searchPath.push_back(ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString()));
            } //if not exists try to create it
            else if(dir.mkpath(dir.absolutePath()))
            {
                //if created, then have write permissions
                writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
                searchPath.push_back(ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString()));
            }
            //load the ultracopier path
            searchPath.push_back(ResourcesManager::AddSlashIfNeeded(QApplication::applicationDirPath().toStdString()));
        #endif
    #else
    QDir dir(QApplication::applicationDirPath());
    writablePath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
    #endif
    vectorRemoveDuplicatesForSmallList(searchPath);
    #ifdef ULTRACOPIER_DEBUG
    int index=0;
    const int &loop_size=searchPath.size();
    while(index<loop_size) //look at each val
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"searchPath.at("+std::to_string(index)+"): "+searchPath.at(index));
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"writablePath: "+writablePath);
    #endif // ULTRACOPIER_DEBUG
}

/// \brief Destroy the resource manager
ResourcesManager::~ResourcesManager()
{
}

/// \brief Get folder presence and the path
std::string ResourcesManager::getFolderReadPath(const std::string &path) const
{
    int index=0;
    const int &loop_size=searchPath.size();
    while(index<loop_size) //look at each val
    {
        QDir dir(QString::fromStdString(searchPath.at(index)+path));
        if(dir.exists()) // if the path have been found, then return the full path
            return ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
        index++;
    }
    return std::string();
}

/// \brief Get folder presence, the path and check in the folder and sub-folder the file presence
std::string ResourcesManager::getFolderReadPathMultiple(const std::string &path,const std::vector<std::string> &fileToCheck) const
{
    int index=0;
    const int &loop_size=searchPath.size();
    while(index<loop_size) //look at each val
    {
        QDir dir(QString::fromStdString(searchPath.at(index)+path));
        if(checkFolderContent(dir.absolutePath().toStdString(),fileToCheck))
            return dir.absolutePath().toStdString()+FacilityEngine::separator();
        index++;
    }
    return std::string();
}

bool ResourcesManager::checkFolderContent(const std::string &path,const std::vector<std::string> &fileToCheck) const
{
    QDir dir(QString::fromStdString(path));
    if(dir.exists()) // if the path have been found, then return the full path
    {
        bool allFileToCheckIsFound=true;
        int index=0;
        const int &loop_size=fileToCheck.size();
        std::string partialPath=ResourcesManager::AddSlashIfNeeded(dir.absolutePath().toStdString());
        while(index<loop_size) //look at each val
        {
            if(!QFile::exists(QString::fromStdString(partialPath+fileToCheck.at(index)))) //if a file have been not found, consider the folder as not suitable
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
std::string ResourcesManager::AddSlashIfNeeded(const std::string &path)
{
    if(path.empty())
        return "/";
    if(path.at(path.size()-1)=='/')
        return path;
    else
        return path+FacilityEngine::separator();
}

/// \brief get the writable path
const std::string &ResourcesManager::getWritablePath() const
{
    return writablePath;
}

/// \brief disable the writable path, if ultracopier is unable to write into
bool ResourcesManager::disableWritablePath()
{
    bool returnVal=true;
    if(writablePath.empty())
        returnVal=false;
    else
        writablePath.clear();
    return returnVal;
}

/// \brief get the read path
const std::vector<std::string> &ResourcesManager::getReadPath() const
{
    return searchPath;
}

/// \brief remove folder
bool ResourcesManager::removeFolder(const std::string &dir)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"folder to remove: "+dir);
    bool errorFound=false;
    QDir currentDir(QString::fromStdString(dir));
    QFileInfoList files = currentDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    int index=0;
    const int &loop_size=files.size();
    while(index<loop_size)
    {
        if(files.at(index).isFile())
        {
            QFile file(files.at(index).absoluteFilePath());
            if(!file.remove())
            {
                errorFound=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"remove file failed: "+file.errorString().toStdString());
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file removed: "+file.fileName().toStdString());
        }
        else if(files.at(index).isDir())
            removeFolder(files.at(index).absoluteFilePath().toStdString());
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unknow file type for: "+files.at(index).absoluteFilePath().toStdString());
        index++;
    }
    if(!currentDir.rmpath(QString::fromStdString(dir)))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"remove path failed, check right and if is empty: "+dir);
        errorFound=true;
    }
    return !errorFound;
}
