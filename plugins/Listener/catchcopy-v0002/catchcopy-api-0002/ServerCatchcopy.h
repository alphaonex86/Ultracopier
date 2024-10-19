/** \file ServerCatchcopy.h
\brief Define the server of catchcopy
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SERVERCATCHCOPY_H
#define SERVERCATCHCOPY_H

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <vector>
#include <string>
#include <QByteArray>
#include <QTimer>

/// \brief Define the server of catchcopy
class ServerCatchcopy : public QObject
{
    Q_OBJECT
    public:
        ServerCatchcopy();
        ~ServerCatchcopy();
        /// \brief return if is listening
        bool isListening() const;
        /// \brief try listen
        bool listen();
        /// \brief try close the server
        void close();
        /// \brief where is the socket/named pipe
        const std::string fullServerName() const;
        /// \brief get the error string on the QLocalServer
        const std::string errorStringServer() const;
        /// \brief get the general error string
        const std::string errorString() const;
        /// \brief set the name of the server
        void setName(const std::string & name);
        /// \brief get the name
        std::string getName() const;
        /// \brief to get a client list
        std::vector<std::string> clientsList() const;
    private:
        std::string pathSocket;
        std::string name;
        std::string error_string;
        QLocalServer server;
        uint32_t idNextClient;
        struct Client
        {
            uint32_t id;
            QLocalSocket *socket;
            QByteArray data;
            bool haveData;
            uint32_t dataSize;
            bool firstProtocolReplied;
            QList<uint32_t> queryNoReplied;
            QTimer *detectTimeOut;
            std::string name;
        };
        QList<Client> clientList;
        struct LinkGlobalToLocalClient
        {
            uint32_t idClient;
            uint32_t orderId;
            uint32_t globalOrderId;
        };
        QList<LinkGlobalToLocalClient> LinkGlobalToLocalClientList;
        enum inputReturnType{Ok,Replied,ExtensionWrong,WrongArgument,WrongArgumentListSize,UnknowOrder};
        inputReturnType parseInputCurrentProtocol(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &returnList);
        bool clientIdFound(const uint32_t &id) const;
        uint32_t nextOrderId;
        QList<uint32_t> orderList;
        uint32_t incrementOrderId();
        void emitNewCopyWithoutDestination(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources);
        void emitNewCopy(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
        void emitNewMoveWithoutDestination(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources);
        void emitNewMove(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
        bool checkDataIntegrity(const QByteArray &data);
    protected:
        void parseInput(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &returnList);
    private slots:
        void newConnection();
        void connectionError(const QLocalSocket::LocalSocketError &error);
        void disconnected();
        void readyRead();
        void checkTimeOut();
    public slots:
        /// \brief disconnect one client
        void disconnectClient(const uint32_t &id);
        /// \brief reply to a client with std::vector<std::string>
        void reply(const uint32_t &client,const uint32_t &orderId,const uint32_t &returnCode,const std::vector<std::string> &returnList);
        /// \brief reply to a client
        void reply(const uint32_t &client,const uint32_t &orderId,const uint32_t &returnCode,const std::string &returnString);
        //reply
        /// \brief send if the protocol is supported
        void protocolSupported(const uint32_t &client,const uint32_t &orderId,const bool &value);
        /// \brief send incorrect arguement list size
        void incorrectArgumentListSize(const uint32_t &client,const uint32_t &orderId);
        /// \brief send incorrect arguement
        void incorrectArgument(const uint32_t &client,const uint32_t &orderId);
        /// \brief the client is registred
        void clientRegistered(const uint32_t &client,const uint32_t &orderId);
        /// \brief send the server name
        void serverName(const uint32_t &client,const uint32_t &orderId,const std::string &name);
        /// \brief send the copy is finished
        void copyFinished(const uint32_t &client,const uint32_t &orderId,const bool &withError);
        /// \brief send the copy is canceled
        void copyCanceled(const uint32_t &client,const uint32_t &orderId);
        /// \brief send the copy is finished by global is order
        void copyFinished(const uint32_t &globalOrderId,const bool &withError);
        /// \brief send copy cancel by global is order
        void copyCanceled(const uint32_t &globalOrderId);
        /// \brief send the unknow order
        void unknowOrder(const uint32_t &client,const uint32_t &orderId);
    signals:
        /// \brief send connected client
        void connectedClient(const uint32_t &id);
        /// \brief send disconnect client
        void disconnectedClient(const uint32_t &id);
        /// \brief have new query
        void newQuery(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &returnList);
        /// \brief have new error
        void error(const std::string &error);
        void communicationError(const std::string &error);
        //query
        /// \brief ask the protocol compatility
        void askProtocolCompatibility(const uint32_t &client,const uint32_t &orderId,const std::string &version);
        /// \brief ask protocol extension
        void askProtocolExtension(const uint32_t &client,const uint32_t &orderId,const std::string &extension);
        /// \brief ask protocol extension with version
        void askProtocolExtension(const uint32_t &client,const uint32_t &orderId,const std::string &extension,const std::string &version);
        /// \brief send the client name, without query id
        void clientName(const uint32_t &client,const std::string &name);
        /// \brief send the client have ask the server name
        void askServerName(const uint32_t &client,const uint32_t &orderId);
        /// \brief copy is send, by globalOrderId, without destination
        void newCopyWithoutDestination(const uint32_t &globalOrderId,const std::vector<std::string> &sources);
        /// \brief copy is send, by globalOrderId, with destination
        void newCopy(const uint32_t &globalOrderId,const std::vector<std::string> &sources,const std::string &destination);
        /// \brief move is send, by globalOrderId, without destination
        void newMoveWithoutDestination(const uint32_t &globalOrderId,const std::vector<std::string> &sources);
        /// \brief move is send, by globalOrderId, with destination
        void newMove(const uint32_t &globalOrderId,const std::vector<std::string> &sources,const std::string &destination);
};

#endif // SERVERCATCHCOPY_H
