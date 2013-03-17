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
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "interface/PluginInterface_SessionLoader.h"
#include "PluginsManager.h"
#include "OptionDialog.h"
#include "LocalPluginOptions.h"

#if !defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE) || !defined(ULTRACOPIER_VERSION_PORTABLE)
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
        void newOptionValue(const QString &groupName,const QString &variableName,const QVariant &value);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
        #endif // ULTRACOPIER_DEBUG
    private:
        //variable
        struct LocalPlugin
        {
            PluginInterface_SessionLoader * sessionLoaderInterface;
            QPluginLoader * pluginLoader;
            QString path;
            LocalPluginOptions *options;
        };
        QList<LocalPlugin> pluginList;
        bool shouldEnabled;
        OptionDialog *optionDialog;
    signals:
        void previouslyPluginAdded(PluginsAvailable);
};
#endif

#endif // SESSIONLOADER_H
