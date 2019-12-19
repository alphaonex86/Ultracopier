/** \file ServerCatchcopy.h
\brief Define the server of catchcopy
\author alpha_one_x86
\version 0002
\date 2010 */

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
		/// \brief set if automatic reply is used 
		void setAutoReply(bool value);
		/// \brief get if autoReply is set
		bool getAutoReply();
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
		inputReturnType parseInputCurrentProtocol(quint32 client,quint32 orderId,QStringList returnList);
		bool autoReply;
		bool clientIdFound(quint32 id);
		quint32 nextOrderId;
		QList<quint32> orderList;
		quint32 incrementOrderId();
		void emitNewCopy(quint32 client,quint32 orderId,QStringList sources);
		void emitNewCopy(quint32 client,quint32 orderId,QStringList sources,QString destination);
		void emitNewMove(quint32 client,quint32 orderId,QStringList sources);
		void emitNewMove(quint32 client,quint32 orderId,QStringList sources,QString destination);
		bool checkDataIntegrity(QByteArray data);
	protected:
		void parseInput(quint32 client,quint32 orderId,QStringList returnList);
	private slots:
		void newConnection();
		void connectionError(QLocalSocket::LocalSocketError error);
		void disconnected();
		void readyRead();
		void checkTimeOut();
	public slots:
		/// \brief disconnect one client
		void disconnectClient(quint32 id);
		/// \brief reply to a client with QStringList
		void reply(quint32 client,quint32 orderId,quint32 returnCode,QStringList returnList);
		/// \brief reply to a client
		void reply(quint32 client,quint32 orderId,quint32 returnCode,QString returnString);
		//reply
		/// \brief send if the protocol is supported
		void protocolSupported(quint32 client,quint32 orderId,bool value);
		/// \brief send incorrect arguement list size
		void incorrectArgumentListSize(quint32 client,quint32 orderId);
		/// \brief send incorrect arguement
		void incorrectArgument(quint32 client,quint32 orderId);
		/// \brief send if protocol extension is supported
		void protocolExtensionSupported(quint32 client,quint32 orderId,bool value);
		/// \brief the client is registred
		void clientRegistered(quint32 client,quint32 orderId);
		/// \brief send the server name
		void serverName(quint32 client,quint32 orderId,QString name);
		/// \brief send the copy is finished
		void copyFinished(quint32 client,quint32 orderId,bool withError);
		/// \brief send the copy is canceled
		void copyCanceled(quint32 client,quint32 orderId);
		/// \brief send the copy is finished by global is order
		void copyFinished(quint32 globalOrderId,bool withError);
		/// \brief send copy cancel by global is order
		void copyCanceled(quint32 globalOrderId);
		/// \brief send the unknow order
		void unknowOrder(quint32 client,quint32 orderId);
	signals:
		/// \brief send connected client
		void connectedClient(quint32 id);
		/// \brief send disconnect client
		void disconnectedClient(quint32 id);
		/// \brief have new query
		void newQuery(quint32 client,quint32 orderId,QStringList returnList);
		/// \brief send new data as string list
		void dataSend(quint32 client,quint32 orderId,quint32 returnCode,QStringList returnList);
		/// \brief send new data as raw data
		void dataSend(quint32 client,quint32 orderId,quint32 returnCode,QByteArray block);
		/// \brief have new error
		void error(QString error);
		//query
		/// \brief ask the protocol compatility
		void askProtocolCompatibility(quint32 client,quint32 orderId,QString version);
		/// \brief ask protocol extension
		void askProtocolExtension(quint32 client,quint32 orderId,QString extension);
		/// \brief ask protocol extension with version
		void askProtocolExtension(quint32 client,quint32 orderId,QString extension,QString version);
		/// \brief send the client name, with query id
		void clientName(quint32 client,quint32 orderId,QString name);
		/// \brief send the client name, without query id
		void clientName(quint32 client,QString name);
		/// \brief send the client have ask the server name
		void askServerName(quint32 client,quint32 orderId);
		/// \brief copy is send, without destination
		void newCopy(quint32 client,quint32 orderId,QStringList sources);
		/// \brief copy is send, with destination
		void newCopy(quint32 client,quint32 orderId,QStringList sources,QString destination);
		/// \brief move is send, without destination
		void newMove(quint32 client,quint32 orderId,QStringList sources);
		/// \brief move is send, with destination
		void newMove(quint32 client,quint32 orderId,QStringList sources,QString destination);
		/// \brief copy is send, by globalOrderId, without destination
		void newCopy(quint32 globalOrderId,QStringList sources);
		/// \brief copy is send, by globalOrderId, with destination
		void newCopy(quint32 globalOrderId,QStringList sources,QString destination);
		/// \brief move is send, by globalOrderId, without destination
		void newMove(quint32 globalOrderId,QStringList sources);
		/// \brief move is send, by globalOrderId, with destination
		void newMove(quint32 globalOrderId,QStringList sources,QString destination);
};

#endif // SERVERCATCHCOPY_H
