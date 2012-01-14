/** \file ClientCatchcopy.h
\brief Define the catchcopy client
\author alpha_one_x86
\version 0002
\date 2010 */

#ifndef CLIENTCATCHCOPY_H
#define CLIENTCATCHCOPY_H

#include <QObject>
#include <QLocalSocket>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QTimer>

class ClientCatchcopy : public QObject
{
	Q_OBJECT
	public:
		ClientCatchcopy();
		QLocalSocket::LocalSocketState state();
		const QString errorStringSocket();
		const QString errorString();
	public slots:
		void connectToServer();
		void disconnectFromServer();
		//to test and internal use
		/// \brief to send order
		quint32 sendProtocol();
		quint32 askServerName();
		quint32 setClientName(const QString & name);
		quint32 checkProtocolExtension(const QString & name);
		quint32 checkProtocolExtension(const QString & name,const QString & version);
		quint32 addCopyWithDestination(const QStringList & sources,const QString & destination);
		quint32 addCopyWithoutDestination(const QStringList & sources);
		quint32 addMoveWithDestination(const QStringList & sources,const QString & destination);
		quint32 addMoveWithoutDestination(const QStringList & sources);
		/// \brief to send stream of string list
		quint32 sendRawOrderList(const QStringList & order);
	signals:
		void connected();
		void disconnected();
		void stateChanged(QLocalSocket::LocalSocketState socketState);
		void error(QString error);
		void errorSocket(QLocalSocket::LocalSocketError socketError);
		void newReply(quint32 orderId,quint32 returnCode,QStringList returnList);
		void dataSend(quint32 orderId,QByteArray data);
		void dataSend(quint32 orderId,QStringList data);
		void unknowReply(quint32 orderId);
		//reply
		void protocolSupported(quint32 orderId);
		void incorrectArgumentListSize(quint32 orderId);
		void incorrectArgument(quint32 orderId);
		void protocolNotSupported(quint32 orderId);
		void protocolExtensionSupported(quint32 orderId,bool isSupported);
		void clientRegistered(quint32 orderId);
		void serverName(quint32 orderId,QString name );
		void copyFinished(quint32 orderId,bool withError);
		void copyCanceled(quint32 orderId);
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
