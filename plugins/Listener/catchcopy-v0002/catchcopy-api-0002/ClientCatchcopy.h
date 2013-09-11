/** \file ClientCatchcopy.h
\brief Define the catchcopy client
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef CLIENTCATCHCOPY_H
#define CLIENTCATCHCOPY_H

#include <QObject>
#include <QLocalSocket>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QTimer>

/// \brief Define the catchcopy client
class ClientCatchcopy : public QObject
{
	Q_OBJECT
	public:
		ClientCatchcopy();
		/// \brief get the socket stat
		QLocalSocket::LocalSocketState state();
		/// \brief error string about the socket
		const QString errorStringSocket();
		/// \brief general error string
		const QString errorString();
	public slots:
		void connectToServer();
		void disconnectFromServer();
		//to test and internal use
		/// \brief to send order
		quint32 sendProtocol();
		/// \brief ask the server name
		quint32 askServerName();
		/// \brief set the client name
		quint32 setClientName(const QString & name);
		/// \brief check protocol extension
		quint32 checkProtocolExtension(const QString & name);
		/// \brief check protocol extension and version
		quint32 checkProtocolExtension(const QString & name,const QString & version);
		/// \brief add copy with destination
		quint32 addCopyWithDestination(const QStringList & sources,const QString & destination);
		/// \brief add copy without destination
		quint32 addCopyWithoutDestination(const QStringList & sources);
		/// \brief add move with destination
		quint32 addMoveWithDestination(const QStringList & sources,const QString & destination);
		/// \brief add move without destination
		quint32 addMoveWithoutDestination(const QStringList & sources);
		/// \brief to send stream of string list
		quint32 sendRawOrderList(const QStringList & order);
	signals:
		/// \brief is connected
		void connected();
		/// \brief is disconnected
		void disconnected();
		/// \brief the socket state have changed
		void stateChanged(QLocalSocket::LocalSocketState socketState);
		/// \brief send the error string
		void error(QString error);
		/// \brief send socket error
		void errorSocket(QLocalSocket::LocalSocketError socketError);
		/// \brief have new reply
		void newReply(quint32 orderId,quint32 returnCode,QStringList returnList);
		/// \brief have data send
		void dataSend(quint32 orderId,QByteArray data);
		/// \brief have data send by string list
		void dataSend(quint32 orderId,QStringList data);
		/// \brief have unknow reply
		void unknowReply(quint32 orderId);
		//reply
		/// \brief protocol is supported
		void protocolSupported(quint32 orderId);
		/// \brief incorrect argument list size
		void incorrectArgumentListSize(quint32 orderId);
		/// \brief incorrect argument
		void incorrectArgument(quint32 orderId);
		/// \brief protocol not supported
		void protocolNotSupported(quint32 orderId);
		/// \brief protocol extension supported
		void protocolExtensionSupported(quint32 orderId,bool isSupported);
		/// \brief client is registred
		void clientRegistered(quint32 orderId);
		/// \brief have the server name
		void serverName(quint32 orderId,QString name);
		/// \brief copy finished
		void copyFinished(quint32 orderId,bool withError);
		/// \brief copy canceled
		void copyCanceled(quint32 orderId);
		/// \brief have unknow order
		void unknowOrder(quint32 orderId); //the server have not understand the order
	private:
		QLocalSocket socket;
		QString error_string;
		quint32 idNextOrder;
		QByteArray data;
		bool haveData;
		int dataSize;
		quint32 orderIdFirstSendProtocol;
		QTimer detectTimeOut;
		bool sendProtocolReplied;
		QList<quint32> notRepliedQuery;
		bool checkDataIntegrity(QByteArray data);
	private slots:
		void readyRead();
		void disconnectedFromSocket();
		void socketIsConnected();
		void checkTimeOut();
	protected:
		bool parseReply(quint32 orderId,quint32 returnCode,QStringList returnList);
};

#endif // CLIENTCATCHCOPY_H
