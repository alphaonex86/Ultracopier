/** \file PluginInterface_Listener.h
\brief Define the interface of the plugin of type: listener
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_LISTENER_H
#define PLUGININTERFACE_LISTENER_H

#include <QObject>
#include <string>
#include <vector>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

/** \brief To define the interface between Ultracopier and the listener
 * */
class PluginInterface_Listener : public QObject
{
    Q_OBJECT
    public:
        /// \brief put this plugin in listen mode
        virtual void listen() = 0;
        /// \brief put close the listen
        virtual void close() = 0;
        /// \brief to get the error string
        virtual const std::string errorString() const = 0;
        /// \brief set the resources for the plugin
        virtual void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,const bool &portableVersion) = 0;
        /// \brief to get the options widget, NULL if not have
        virtual QWidget * options() = 0;
        /// \brief to get a client list
        virtual std::vector<std::string> clientsList() const = 0;
    public slots:
        /// \brief send when copy is finished
        virtual void transferFinished(const uint32_t &orderId,const bool &withError) = 0;
        /// \brief send when copy is canceled
        virtual void transferCanceled(const uint32_t &orderId) = 0;
        /// \brief to reload the translation, because the new language have been loaded
        virtual void newLanguageLoaded() = 0;
    signals:
        void newState(const Ultracopier::ListeningState &state) const;
        void newCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources) const;
        void newCopy(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination) const;
        void newMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources) const;
        void newMove(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination) const;
        void error(const std::string &error) const;
        void newClientList() const;
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
};

Q_DECLARE_INTERFACE(PluginInterface_Listener,"first-world.info.ultracopier.PluginInterface.Listener/1.2.4.0");

#endif // PLUGININTERFACE_LISTENER_H
