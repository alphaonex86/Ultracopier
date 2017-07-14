/** \file main.cpp
\brief Define the main() for the point entry
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QApplication>
#include <QtPlugin>

#include "Environment.h"
#include "EventDispatcher.h"
#include "LanguagesManager.h"
#include "ThemesManager.h"
#include "DebugEngine.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"
#include "PluginsManager.h"

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    Q_IMPORT_PLUGIN(CopyEngineFactory)
    Q_IMPORT_PLUGIN(ThemesFactory)
    Q_IMPORT_PLUGIN(Listener)
    #ifdef Q_OS_WIN32
        Q_IMPORT_PLUGIN(WindowsExplorerLoader)
        #if !defined(ULTRACOPIER_VERSION_PORTABLE)
        Q_IMPORT_PLUGIN(WindowsSessionLoader)
        #endif
    #endif
#endif
#endif

#ifdef ULTRACOPIER_DEBUG
DebugModel *DebugModel::debugModel=NULL;
DebugEngine *DebugEngine::debugEngine=NULL;
#endif
ResourcesManager *ResourcesManager::resourcesManager=NULL;
OptionEngine *OptionEngine::optionEngine=NULL;
PluginsManager *PluginsManager::pluginsManager=NULL;
LanguagesManager *LanguagesManager::languagesManager=NULL;
ThemesManager *ThemesManager::themesManager=NULL;

void registerTheOptions()
{
    OptionEngine::optionEngine=new OptionEngine();

    //register the var
    //add the options to use
    std::vector<std::pair<std::string, std::string> > KeysList;
    //add the options hidden, will not show in options pannel
    KeysList.clear();
    KeysList.push_back(std::pair<std::string, std::string>("Last_version_used","na"));
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    KeysList.push_back(std::pair<std::string, std::string>("key",""));
    #endif
    KeysList.push_back(std::pair<std::string, std::string>("ActionOnManualOpen","1"));
    KeysList.push_back(std::pair<std::string, std::string>("GroupWindowWhen","0"));
    KeysList.push_back(std::pair<std::string, std::string>("displayOSSpecific","true"));
    KeysList.push_back(std::pair<std::string, std::string>("confirmToGroupWindows","true"));
    KeysList.push_back(std::pair<std::string, std::string>("giveGPUTime","true"));
    KeysList.push_back(std::pair<std::string, std::string>("remainingTimeAlgorithm","0"));
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    KeysList.push_back(std::pair<std::string, std::string>("checkTheUpdate","true"));
    #else
    KeysList.push_back(std::pair<std::string, std::string>("checkTheUpdate","false"));
    #endif
    #endif
    OptionEngine::optionEngine->addOptionGroup("Ultracopier",KeysList);

    KeysList.clear();
    KeysList.push_back(std::pair<std::string, std::string>("List","Ultracopier"));
    OptionEngine::optionEngine->addOptionGroup("CopyEngine",KeysList);

    //load the GUI option
    std::string defaultLogFile;
    if(ResourcesManager::resourcesManager->getWritablePath()!="")
        defaultLogFile=ResourcesManager::resourcesManager->getWritablePath()+"ultracopier-files.log";
    KeysList.clear();
    KeysList.push_back(std::pair<std::string, std::string>("enabled","false"));
    KeysList.push_back(std::pair<std::string, std::string>("file",defaultLogFile));
    KeysList.push_back(std::pair<std::string, std::string>("transfer","true"));
    KeysList.push_back(std::pair<std::string, std::string>("error","true"));
    KeysList.push_back(std::pair<std::string, std::string>("folder","true"));
    KeysList.push_back(std::pair<std::string, std::string>("sync","true"));
    KeysList.push_back(std::pair<std::string, std::string>("transfer_format","[%time%] %source% (%size%) %destination%"));
    KeysList.push_back(std::pair<std::string, std::string>("error_format","[%time%] %path%, %error%"));
    KeysList.push_back(std::pair<std::string, std::string>("folder_format","[%time%] %operation% %path%"));
    OptionEngine::optionEngine->addOptionGroup("Write_log",KeysList);

    KeysList.clear();
    KeysList.push_back(std::pair<std::string, std::string>("CatchCopyAsDefault","true"));
    OptionEngine::optionEngine->addOptionGroup("CopyListener",KeysList);

    KeysList.clear();
    KeysList.push_back(std::pair<std::string, std::string>("LoadAtSessionStarting","true"));
    OptionEngine::optionEngine->addOptionGroup("SessionLoader",KeysList);
}

/// \brief Define the main() for the point entry
int main(int argc, char *argv[])
{
    int returnCode;
    QApplication ultracopierApplication(argc, argv);
    ultracopierApplication.setApplicationVersion(ULTRACOPIER_VERSION);
    ultracopierApplication.setQuitOnLastWindowClosed(false);
    qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
    qRegisterMetaType<Ultracopier::DebugLevel>("Ultracopier::DebugLevel");
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");
    qRegisterMetaType<Ultracopier::ItemOfCopyList>("Ultracopier::ItemOfCopyList");

    #ifdef ULTRACOPIER_DEBUG
    DebugModel::debugModel=new DebugModel();
    DebugEngine::debugEngine=new DebugEngine();
    #endif
    ResourcesManager::resourcesManager=new ResourcesManager();
    registerTheOptions();

    PluginsManager::pluginsManager=new PluginsManager();
    LanguagesManager::languagesManager=new LanguagesManager();
    ThemesManager::themesManager=new ThemesManager();

    //the main code, event loop of Qt and event dispatcher of ultracopier
    {
        EventDispatcher backgroundRunningInstance;
        if(backgroundRunningInstance.shouldBeClosed())
            returnCode=0;
        else
            returnCode=ultracopierApplication.exec();
    }

    delete ThemesManager::themesManager;
    ThemesManager::themesManager=NULL;
    delete LanguagesManager::languagesManager;
    LanguagesManager::languagesManager=NULL;
    delete PluginsManager::pluginsManager;
    PluginsManager::pluginsManager=NULL;
    delete OptionEngine::optionEngine;
    OptionEngine::optionEngine=NULL;
    delete ResourcesManager::resourcesManager;
    ResourcesManager::resourcesManager=NULL;
    #ifdef ULTRACOPIER_DEBUG
    delete DebugEngine::debugEngine;
    DebugEngine::debugEngine=NULL;
    delete DebugModel::debugModel;
    DebugModel::debugModel=NULL;
    #endif

    return returnCode;
}

