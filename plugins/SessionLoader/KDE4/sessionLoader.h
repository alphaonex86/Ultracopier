/** \file sessionLoader.h
\brief Define the session loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SESSION_LOADER_PLUGIN_H
#define SESSION_LOADER_PLUGIN_H

#include <QObject>
#include "Environment.h"
#include "../../../interface/PluginInterface_SessionLoader.h"

/// \brief Define the session loader
class KDESessionLoader : public PluginInterface_SessionLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.SessionLoader/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_SessionLoader)
public:
    /// \brief to set if it's enabled or not
    void setEnabled(const bool &enabled);
    /// \brief to get if is enabled
    bool getEnabled() const;
    /// \brief set the resources for the plugins
    void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion);
    /// \brief to get the options widget, NULL if not have
    QWidget * options();
public slots:
    /// \brief to reload the translation, because the new language have been loaded
    void newLanguageLoaded();
};

#endif // SESSION_LOADER_PLUGIN_H
