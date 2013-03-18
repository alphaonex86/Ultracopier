/** \file PluginLoader.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\licence GPL3, see the file COPYING

This class load ALL plugin compatible to listen and catch the copy/move
*/

#ifndef PluginLoader_H
#define PluginLoader_H

#include <QObject>
#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "interface/PluginInterface_PluginLoader.h"
#include "PluginsManager.h"
#include "OptionDialog.h"
#include "LocalPluginOptions.h"

namespace Ui {
    class PluginLoaderOptions;
}

/** \brief Load the plugin

  It use ResourcesManager(), but it provide more higher abstraction. It parse the plugins information, check it, check the dependancies.

  \see ResourcesManager::ResourcesManager()
  */
class PluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit PluginLoader(OptionDialog *optionDialog);
    ~PluginLoader();
    /** \brief to rended the state */
    void resendState();
    /** \brief should load plugin into file manager if needed */
    void load();
    /** \brief should unload plugin into file manager */
    void unload();
private slots:
    void onePluginAdded(const PluginsAvailable &plugin);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    void onePluginWillBeRemoved(const PluginsAvailable &plugin);
    #endif
    #ifdef ULTRACOPIER_DEBUG
    void debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
    #endif // ULTRACOPIER_DEBUG
    void allPluginIsloaded();
    void newState(const Ultracopier::CatchState &state);
private:
    //variable
    struct LocalPlugin
    {
        PluginInterface_PluginLoader * pluginLoaderInterface;
        QPluginLoader * pluginLoader;
        Ultracopier::CatchState state;
        QString path;
        bool inWaitOfReply;
        LocalPluginOptions *options;
    };
    QList<LocalPlugin> pluginList;
    bool needEnable;
    Ultracopier::CatchState last_state;
    bool last_have_plugin,last_inWaitOfReply;
    void sendState(bool force=false);
    OptionDialog *optionDialog;
    bool stopIt;
signals:
    void pluginLoaderReady(Ultracopier::CatchState state,bool havePlugin,bool someAreInWaitOfReply);
    void previouslyPluginAdded(PluginsAvailable);
};

#endif // PluginLoader_H
