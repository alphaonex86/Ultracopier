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

class ServerCatchcopy : public QObject
{
	Q_OBJECT
	public:
		ServerCatchcopy();
		bool isListening();
		bool listen();
		void close();
		const QString errorStringServer();
		const QString errorString();
		void setAutoReply(bool value);
		bool getAutoReply();
		void setName(const QString & name);
		QString getName();
	private:
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
		void disconnectClient(quint32 id);
		void reply(quint32 client,quint32 orderId,quint32 returnCode,QStringList returnList);
		void reply(quint32 client,quint32 orderId,quint32 returnCode,QString returnString);
		//reply
		void protocolSupported(quint32 client,quint32 orderId,bool value);
		void incorrectArgumentListSize(quint32 client,quint32 orderId);
		void incorrectArgument(quint32 client,quint32 orderId);
		void protocolExtensionSupported(quint32 client,quint32 orderId,bool value);
		void clientRegistered(quint32 client,quint32 orderId);
		void serverName(quint32 client,quint32 orderId,QString name);
		void copyFinished(quint32 client,quint32 orderId,bool withError);
		void copyCanceled(quint32 client,quint32 orderId);
		void copyFinished(quint32 globalOrderId,bool withError);
		void copyCanceled(quint32 globalOrderId);
		void unknowOrder(quint32 client,quint32 orderId);
	signals:
		void connectedClient(quint32 id);
		void disconnectedClient(quint32 id);
		void newQuery(quint32 client,quint32 orderId,QStringList returnList);
		void dataSend(quint32 client,quint32 orderId,quint32 returnCode,QStringList returnList);
		void dataSend(quint32 client,quint32 orderId,quint32 returnCode,QByteArray block);
		void error(QString error);
		//query
		void askProtocolCompatibility(quint32 client,quint32 orderId,QString version);
		void askProtocolExtension(quint32 client,quint32 orderId,QString extension);
		void askProtocolExtension(quint32 client,quint32 orderId,QString extension,QString version);
		void clientName(quint32 client,quint32 orderId,QString name);
		void clientName(quint32 client,QString name);
		void askServerName(quint32 client,quint32 orderId);
		void newCopy(quint32 client,quint32 orderId,QStringList sources);
		void newCopy(quint32 client,quint32 orderId,QStringList sources,QString destination);
		void newMove(quint32 client,quint32 orderId,QStringList sources);
		void newMove(quint32 client,quint32 orderId,QStringList sources,QString destination);
		void newCopy(quint32 globalOrderId,QStringList sources);
		void newCopy(quint32 globalOrderId,QStringList sources,QString destination);
		void newMove(quint32 globalOrderId,QStringList sources);
		void newMove(quint32 globalOrderId,QStringList sources,QString destination);
};

#endif // SERVERCATCHCOPY_H
