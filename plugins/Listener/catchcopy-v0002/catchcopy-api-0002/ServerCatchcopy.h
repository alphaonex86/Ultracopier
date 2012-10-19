/** \file ServerCatchcopy.h
\brief Define the server of catchcopy
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SERVERCATCHCOPY_H
#define SERVERCATCHCOPY_H

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <QStringList>
#include <QString>
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
		bool isListening();
		/// \brief try listen
		bool listen();
		/// \brief try close the server
		void close();
		/// \brief get the error string on the QLocalServer
		const QString errorStringServer();
		/// \brief get the general error string
		const QString errorString();
		/// \brief set the name of the server
		void setName(const QString & name);
		/// \brief get the name
		QString getName();
	private:
		QString pathSocket;
		QString name;
		QString error_string;
		QLocalServer server;
		quint32 idNextClient;
		struct Client
		{
			quint32 id;
			QLocalSocket *socket;
			QByteArray data;
			bool haveData;
			quint32 dataSize;
			bool firstProtocolReplied;
			QList<quint32> queryNoReplied;
			QTimer *detectTimeOut;
		};
		QList<Client> ClientList;
		struct LinkGlobalToLocalClient
		{
			quint32 idClient;
			quint32 orderId;
			quint32 globalOrderId;
		};
		QList<LinkGlobalToLocalClient> LinkGlobalToLocalClientList;
		enum inputReturnType{Ok,Replied,ExtensionWrong,WrongArgument,WrongArgumentListSize,UnknowOrder};
		inputReturnType parseInputCurrentProtocol(const quint32 &client,const quint32 &orderId,const QStringList &returnList);
		bool clientIdFound(const quint32 &id);
		quint32 nextOrderId;
		QList<quint32> orderList;
		quint32 incrementOrderId();
		void emitNewCopyWithoutDestination(const quint32 &client,const quint32 &orderId,const QStringList &sources);
		void emitNewCopy(const quint32 &client,const quint32 &orderId,const QStringList &sources,const QString &destination);
		void emitNewMoveWithoutDestination(const quint32 &client,const quint32 &orderId,const QStringList &sources);
		void emitNewMove(const quint32 &client,const quint32 &orderId,const QStringList &sources,const QString &destination);
		bool checkDataIntegrity(const QByteArray &data);
	protected:
		void parseInput(const quint32 &client,const quint32 &orderId,const QStringList &returnList);
	private slots:
		void newConnection();
		void connectionError(const QLocalSocket::LocalSocketError &error);
		void disconnected();
		void readyRead();
		void checkTimeOut();
	public slots:
		/// \brief disconnect one client
		void disconnectClient(const quint32 &id);
		/// \brief reply to a client with QStringList
		void reply(const quint32 &client,const quint32 &orderId,const quint32 &returnCode,const QStringList &returnList);
		/// \brief reply to a client
		void reply(const quint32 &client,const quint32 &orderId,const quint32 &returnCode,const QString &returnString);
		//reply
		/// \brief send if the protocol is supported
		void protocolSupported(const quint32 &client,const quint32 &orderId,const bool &value);
		/// \brief send incorrect arguement list size
		void incorrectArgumentListSize(const quint32 &client,const quint32 &orderId);
		/// \brief send incorrect arguement
		void incorrectArgument(const quint32 &client,const quint32 &orderId);
		/// \brief the client is registred
		void clientRegistered(const quint32 &client,const quint32 &orderId);
		/// \brief send the server name
		void serverName(const quint32 &client,const quint32 &orderId,const QString &name);
		/// \brief send the copy is finished
		void copyFinished(const quint32 &client,const quint32 &orderId,const bool &withError);
		/// \brief send the copy is canceled
		void copyCanceled(const quint32 &client,const quint32 &orderId);
		/// \brief send the copy is finished by global is order
		void copyFinished(const quint32 &globalOrderId,const bool &withError);
		/// \brief send copy cancel by global is order
		void copyCanceled(const quint32 &globalOrderId);
		/// \brief send the unknow order
		void unknowOrder(const quint32 &client,const quint32 &orderId);
	signals:
		/// \brief send connected client
		void connectedClient(const quint32 &id);
		/// \brief send disconnect client
		void disconnectedClient(const quint32 &id);
		/// \brief have new query
		void newQuery(const quint32 &client,const quint32 &orderId,const QStringList &returnList);
		/// \brief have new error
		void error(const QString &error);
		//query
		/// \brief ask the protocol compatility
		void askProtocolCompatibility(const quint32 &client,const quint32 &orderId,const QString &version);
		/// \brief ask protocol extension
		void askProtocolExtension(const quint32 &client,const quint32 &orderId,const QString &extension);
		/// \brief ask protocol extension with version
		void askProtocolExtension(const quint32 &client,const quint32 &orderId,const QString &extension,const QString &version);
		/// \brief send the client name, without query id
		void clientName(const quint32 &client,const QString &name);
		/// \brief send the client have ask the server name
		void askServerName(const quint32 &client,const quint32 &orderId);
		/// \brief copy is send, by globalOrderId, without destination
		void newCopyWithoutDestination(const quint32 &globalOrderId,const QStringList &sources);
		/// \brief copy is send, by globalOrderId, with destination
		void newCopy(const quint32 &globalOrderId,const QStringList &sources,const QString &destination);
		/// \brief move is send, by globalOrderId, without destination
		void newMoveWithoutDestination(const quint32 &globalOrderId,const QStringList &sources);
		/// \brief move is send, by globalOrderId, with destination
		void newMove(const quint32 &globalOrderId,const QStringList &sources,const QString &destination);
};

#endif // SERVERCATCHCOPY_H
