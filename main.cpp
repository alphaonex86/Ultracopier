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

DebugModel DebugModel::debugModel;
DebugEngine DebugEngine::debugEngine;
ResourcesManager ResourcesManager::resourcesManager;
OptionEngine OptionEngine::optionEngine;
PluginsManager PluginsManager::pluginsManager;
LanguagesManager LanguagesManager::languagesManager;
ThemesManager ThemesManager::themesManager;

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
    //the main code, event loop of Qt and event dispatcher of ultracopier
    {
        EventDispatcher backgroundRunningInstance;
        if(backgroundRunningInstance.shouldBeClosed())
            returnCode=0;
        else
            returnCode=ultracopierApplication.exec();
    }
    return returnCode;
}

