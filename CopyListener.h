/** \file CopyListener.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef COPYLISTENER_H
#define COPYLISTENER_H

#include <QObject>
#include <QList>
#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include <QPluginLoader>
#endif

#include "interface/PluginInterface_Listener.h"
#include "Environment.h"
#include "PluginLoaderCore.h"
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
        void newPluginCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources);
        void newPluginCopy(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
        void newPluginMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources);
        void newPluginMove(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
        void onePluginAdded(const PluginsAvailable &plugin);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        #endif
        void newState(const Ultracopier::ListeningState &state);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string& fonction,const std::string& text,const std::string& file,const int& ligne);
        #endif // ULTRACOPIER_DEBUG
        void error(const std::string &error);
        void allPluginIsloaded();
        void reloadClientList();
    public slots:
        /** \brief the copy is finished
         \param orderId id used when it have send the copy
         \param withError true if it have found error
         \see newCopy()
         \see newMove()
        */
        void copyFinished(const uint32_t & orderId,const bool &withError);
        /** \brief the copy is canceled by the user
         \param orderId id used when it have send the copy
         \see newCopy()
         \see newMove()
        */
        void copyCanceled(const uint32_t & orderId);
        /** \brief try listen, to get copy/move from external source (mainly the file manager)
         \see close()
        */
        void listen();
        /** \brief stop listen, to get copy/move from external source (mainly the file manager)
         \see listen()
        */
        void close();
        /** new copy without destination have been pased by the CLI */
        void copyWithoutDestination(std::vector<std::string> sources);
        /** new copy with destination have been pased by the CLI */
        void copy(std::vector<std::string> sources,std::string destination);
        /** new move without destination have been pased by the CLI */
        void moveWithoutDestination(std::vector<std::string> sources);
        /** new move with destination have been pased by the CLI */
        void move(std::vector<std::string> sources,std::string destination);
    signals:
        void newCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources) const;
        void newCopy(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination) const;
        void newMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources) const;
        void newMove(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination) const;
        void listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply) const;
        void pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply) const;
        void previouslyPluginAdded(const PluginsAvailable &) const;
        void newClientList(const std::vector<std::string> &clientsList) const;
    private:
        struct PluginListener
        {
            PluginInterface_Listener *listenInterface;
            #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
            QPluginLoader *pluginLoader;
            #endif
            std::string path;
            Ultracopier::ListeningState state;
            bool inWaitOfReply;
            LocalPluginOptions *options;
        };
        std::vector<PluginListener> pluginList;
        //for the options
        uint32_t nextOrderId;
        std::vector<uint32_t> orderList;
        //for the copy as suspend
        struct CopyRunning
        {
            PluginInterface_Listener *listenInterface;
            uint32_t pluginOrderId;
            uint32_t orderId;
        };
        std::vector<CopyRunning> copyRunningList;
        uint32_t incrementOrderId();
        bool tryListen;
        PluginLoaderCore *pluginLoader;
        Ultracopier::ListeningState last_state;
        bool last_have_plugin,last_inWaitOfReply;
        void sendState(bool force=false);
        std::vector<std::string> stripSeparator(std::vector<std::string> sources);
        OptionDialog *optionDialog;
        bool stopIt;
        std::regex stripSeparatorRegex;
};

#endif // COPYLISTENER_H
