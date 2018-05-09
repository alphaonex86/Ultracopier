/** \file pluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGIN_LOADER_TEST_H
#define PLUGIN_LOADER_TEST_H

#include <QObject>
#include <QMessageBox>

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QSet>
#include <QKeySequence>

#include "../../../interface/PluginInterface_PluginLoader.h"
#include "Environment.h"
#include "OptionsWidget.h"

/// \brief \brief Define the plugin loader
class KeyBindPlugin : public PluginInterface_PluginLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.PluginLoader/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_PluginLoader)
public:
    KeyBindPlugin();
    ~KeyBindPlugin();
    /// \brief try enable/disable the catching
    void setEnabled(const bool &needBeRegistred);
    /// \brief to set resources, writePath can be empty if read only mode
    void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion);
    /// \brief to get the options widget, NULL if not have
    QWidget * options();
public slots:
    /// \brief to reload the translation, because the new language have been loaded
    void newLanguageLoaded();
private:
    OptionInterface * optionsEngine;
    OptionsWidget optionsWidget;
private slots:
    void setKeyBind(const QKeySequence &keySequence);
};

#endif // PLUGIN_LOADER_TEST_H
