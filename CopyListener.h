/** \file CopyListener.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef COPYLISTENER_H
#define COPYLISTENER_H

#include <QObject>
#include <QList>
#include <QPluginLoader>

#include "interface/PluginInterface_Listener.h"
#include "Environment.h"
#include "PluginLoader.h"
#include "OptionDialog.h"

/** \brief to load all the listener and parse all event */
class CopyListener : public QObject
{
    Q_OBJECT
    public:
        explicit CopyListener(OptionDialog *optionDialog);
        ~CopyListener();
        /** \brief send of one listener is loaded */
        bool oneListenerIsLoaded();
        /** \brief to resend the state */
        void resendState();
    private slots:
        //void newPlugin();
        void newPluginCopyWithoutDestination(const quint32 &orderId,const QStringList &sources);
        void newPluginCopy(const quint32 &orderId,const QStringList &sources,const QString &destination);
        void newPluginMoveWithoutDestination(const quint32 &orderId,const QStringList &sources);
        void newPluginMove(const quint32 &orderId,const QStringList &sources,const QString &destination);
        void onePluginAdded(const PluginsAvailable &plugin);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        #endif
        void newState(const Ultracopier::ListeningState &state);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
        #endif // ULTRACOPIER_DEBUG
        void error(const QString &error);
        void allPluginIsloaded();
        void reloadClientList();
    public slots:
        /** \brief the copy is finished
         \param orderId id used when it have send the copy
         \param withError true if it have found error
         \see newCopy()
         \see newMove()
        */
        void copyFinished(const quint32 & orderId,const bool &withError);
        /** \brief the copy is canceled by the user
         \param orderId id used when it have send the copy
         \see newCopy()
         \see newMove()
        */
        void copyCanceled(const quint32 & orderId);
        /** \brief try listen, to get copy/move from external source (mainly the file manager)
         \see close()
        */
        void listen();
        /** \brief stop listen, to get copy/move from external source (mainly the file manager)
         \see listen()
        */
        void close();
        /** new copy without destination have been pased by the CLI */
        void copyWithoutDestination(QStringList sources);
        /** new copy with destination have been pased by the CLI */
        void copy(QStringList sources,QString destination);
        /** new move without destination have been pased by the CLI */
        void moveWithoutDestination(QStringList sources);
        /** new move with destination have been pased by the CLI */
        void move(QStringList sources,QString destination);
    signals:
        void newCopyWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
        void newCopy(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
        void newMoveWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
        void newMove(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
        void listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void previouslyPluginAdded(const PluginsAvailable &);
        void newClientList(const QStringList &clientsList);
    private:
        struct PluginListener
        {
            PluginInterface_Listener *listenInterface;
            QPluginLoader *pluginLoader;
            QString path;
            Ultracopier::ListeningState state;
            bool inWaitOfReply;
            LocalPluginOptions *options;
        };
        QList<PluginListener> pluginList;
        //for the options
        quint32 nextOrderId;
        QList<quint32> orderList;
        //for the copy as suspend
        struct CopyRunning
        {
            PluginInterface_Listener *listenInterface;
            quint32 pluginOrderId;
            quint32 orderId;
        };
        QList<CopyRunning> copyRunningList;
        quint32 incrementOrderId();
        bool tryListen;
        PluginLoader *pluginLoader;
        Ultracopier::ListeningState last_state;
        bool last_have_plugin,last_inWaitOfReply;
        void sendState(bool force=false);
        QStringList stripSeparator(QStringList sources);
        OptionDialog *optionDialog;
        bool stopIt;
};

#endif // COPYLISTENER_H
