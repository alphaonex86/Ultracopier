/** \file SessionLoader.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\licence GPL3, see the file COPYING

This class load ALL plugin compatible to listen and catch the copy/move
*/

#ifndef SESSIONLOADER_H
#define SESSIONLOADER_H

#include <QObject>
#include <QList>
#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include <QPluginLoader>
#endif
#include <QString>
#include <QStringList>

#include "interface/PluginInterface_SessionLoader.h"
#include "PluginsManager.h"
#include "OptionDialog.h"
#include "LocalPluginOptions.h"

#ifndef ULTRACOPIER_VERSION_PORTABLE
/** \brief manage all SessionLoader plugin */
class SessionLoader : public QObject
{
    Q_OBJECT
    public:
        explicit SessionLoader(OptionDialog *optionDialog);
        ~SessionLoader();
    private slots:
        void onePluginAdded(const PluginsAvailable &plugin);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        #endif
        void newOptionValue(const std::string &groupName,const std::string &variableName,const std::string &value);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string& fonction,const std::string& text,const std::string& file,const int& ligne);
        #endif // ULTRACOPIER_DEBUG
    private:
        //variable
        struct LocalPlugin
        {
            PluginInterface_SessionLoader * sessionLoaderInterface;
            #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
            QPluginLoader * pluginLoader;
            #endif
            std::string path;
            LocalPluginOptions *options;
        };
        std::vector<LocalPlugin> pluginList;
        bool shouldEnabled;
        OptionDialog *optionDialog;
    signals:
        void previouslyPluginAdded(PluginsAvailable) const;
};
#endif

#endif // SESSIONLOADER_H
