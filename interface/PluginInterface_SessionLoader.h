/** \file PluginInterface_SessionLoader.h
\brief Define the interface of the plugin of type: session loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_SESSIONLOADER_H
#define PLUGININTERFACE_SESSIONLOADER_H

#include <QString>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

/** \brief To define the interface between Ultracopier and the session loader
 * */
class PluginInterface_SessionLoader : public QObject
{
    Q_OBJECT
    public:
        /// \brief set enabled/disabled
        virtual void setEnabled(const bool &enabled) = 0;
        /// \brief get if is enabled
        virtual bool getEnabled() = 0;
        /// \brief set the resources
        virtual void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion) = 0;
        /// \brief to get the options widget, NULL if not have
        virtual QWidget * options() = 0;
    public slots:
        /// \brief to reload the translation, because the new language have been loaded
        virtual void newLanguageLoaded() = 0;
    signals:
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
};

Q_DECLARE_INTERFACE(PluginInterface_SessionLoader,"first-world.info.ultracopier.PluginInterface.SessionLoader/0.4.0.0");

#endif // PLUGININTERFACE_SESSIONLOADER_H
