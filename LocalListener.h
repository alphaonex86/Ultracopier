#ifndef LOCALLISTENER_H
#define LOCALLISTENER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringList>
#include <QString>
#include <QCoreApplication>
#include <QFSFileEngine>
#include <QMessageBox>
#include <QTimer>
#include <QList>

#include "Environment.h"
#include "ExtraSocket.h"

class LocalListener : public QObject
{
    Q_OBJECT
public:
    explicit LocalListener(QObject *parent = 0);
public slots:
	bool tryConnect();
	/// the listen server
	void listenServer();
private:
	QLocalServer localServer;
	QTimer TimeOutQLocalSocket;
	typedef struct {
		QLocalSocket * socket;
		QByteArray data;
		int size;
		bool haveData;
	} composedData;
	QList<composedData> clientList;
private slots:
	//the time is done
	void timeoutDectected();
	/// \brief Data is incomming
	void dataIncomming();
	/// \brief Deconnexion client
	void deconnectClient();
	/// LocalListener New connexion
	void newConnexion();
	#ifdef ULTRACOPIER_DEBUG
	/** \brief If error occured at socket
	\param theErrorDefine The error define */
	void error(QLocalSocket::LocalSocketError theErrorDefine);
	#endif
signals:
	void cli(QStringList ultracopierArguments,bool external);
};

#endif // LOCALLISTENER_H
