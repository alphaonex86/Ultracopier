/** \file PluginInterface_Listener.h
\brief Define the interface of the plugin of type: listener
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_LISTENER_H
#define PLUGININTERFACE_LISTENER_H

#include <QObject>
#include <QString>
#include <QStringList>

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
        virtual const QString errorString() const = 0;
        /// \brief set the resources for the plugin
        virtual void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion) = 0;
        /// \brief to get the options widget, NULL if not have
        virtual QWidget * options() = 0;
        /// \brief to get a client list
        virtual QStringList clientsList() const = 0;
    public slots:
        /// \brief send when copy is finished
        virtual void transferFinished(const quint32 &orderId,const bool &withError) = 0;
        /// \brief send when copy is canceled
        virtual void transferCanceled(const quint32 &orderId) = 0;
        /// \brief to reload the translation, because the new language have been loaded
        virtual void newLanguageLoaded() = 0;
    signals:
        void newState(const Ultracopier::ListeningState &state) const;
        void newCopyWithoutDestination(const quint32 &orderId,const QStringList &sources) const;
        void newCopy(const quint32 &orderId,const QStringList &sources,const QString &destination) const;
        void newMoveWithoutDestination(const quint32 &orderId,const QStringList &sources) const;
        void newMove(const quint32 &orderId,const QStringList &sources,const QString &destination) const;
        void error(const QString &error) const;
        void newClientList() const;
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne) const;
};

Q_DECLARE_INTERFACE(PluginInterface_Listener,"first-world.info.ultracopier.PluginInterface.Listener/1.0.0.0");

#endif // PLUGININTERFACE_LISTENER_H
