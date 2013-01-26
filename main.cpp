/** \file main.cpp
\brief Define the main() for the point entry
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QApplication>

#include "Environment.h"
#include "EventDispatcher.h"

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
    DebugEngine::getInstance();
    #endif // ULTRACOPIER_DEBUG
    //the main code, event loop of Qt and event dispatcher of ultracopier
    {
        EventDispatcher backgroundRunningInstance;
        if(backgroundRunningInstance.shouldBeClosed())
            returnCode=0;
        else
            returnCode=ultracopierApplication.exec();
    }
    #ifdef ULTRACOPIER_DEBUG
    DebugEngine::destroyInstanceAtTheLastCall();
    #endif // ULTRACOPIER_DEBUG
    return returnCode;
}

