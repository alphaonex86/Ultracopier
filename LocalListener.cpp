/** \file LocalListener.cpp
\brief The have local server, to have unique instance, and send arguments to the current running instance
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LocalListener.h"

#include <QLocalSocket>

LocalListener::LocalListener(QObject *parent) :
    QObject(parent)
{
    //for detect the timeout on QLocalSocket
    TimeOutQLocalSocket.setInterval(500);
    TimeOutQLocalSocket.setSingleShot(true);
    connect(&TimeOutQLocalSocket, &QTimer::timeout, this, &LocalListener::timeoutDectected);
    connect(plugins,&PluginsManager::pluginListingIsfinish,this, &LocalListener::allPluginIsloaded,Qt::QueuedConnection);
}

LocalListener::~LocalListener()
{
    if(localServer.isListening())
    {
        localServer.close();
        if(!QLocalServer::removeServer(ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME)))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to remove the listening server"));
    }
}

bool LocalListener::tryConnect()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QStringList ultracopierArguments=QCoreApplication::arguments();
    //remove excutable path because is useless (unsafe to use)
    ultracopierArguments.removeFirst();
    //add the current path to file full path resolution if needed
    ultracopierArguments.insert(0,QDir::currentPath());
    QLocalSocket localSocket;
    localSocket.connectToServer(ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME),QIODevice::WriteOnly);
    if(localSocket.waitForConnected(1000))
    {
        if(!localSocket.isValid())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"localSocket is not valid!");
            return false;
        }
        emit cli(ultracopierArguments,false,true);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"connection succes, number arguments given: "+QString::number(ultracopierArguments.size()));
        #ifdef ULTRACOPIER_DEBUG
        for (int i = 0; i < ultracopierArguments.size(); ++i) {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"argument["+QString::number(i)+"]: "+ultracopierArguments.at(i));
        }
        #endif // ULTRACOPIER_DEBUG
        //cut string list and send it as block of 32KB
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        //for total size
        out << int(0);
        //send the arguments
        out << ultracopierArguments;
        //write the size content
        out.device()->seek(0);
        out << block.size();
        do
        {
            QByteArray blockToSend;
            int byteWriten;
            blockToSend=block.left(32*1024);//32KB
            block.remove(0,blockToSend.size());
            byteWriten = localSocket.write(blockToSend);
            #ifdef ULTRACOPIER_DEBUG
            if(!localSocket.isValid())
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"localSocket is not valid!");
            if(localSocket.errorString()!="Unknown error" && localSocket.errorString()!="")
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"localSocket->errorString(): "+localSocket.errorString());
            if(blockToSend.size()!=byteWriten)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"blockToSend("+QString::number(blockToSend.size())+")!=byteWriten("+QString::number(byteWriten)+")");
            #endif // ULTRACOPIER_DEBUG
            if(localSocket.waitForBytesWritten(200))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Block send correctly");
            }
            else
            {
                QMessageBox::critical(NULL,"Alert","No arguments send because timeout detected!");
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Block not send correctly");
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"blockToSend: "+QString(blockToSend.toHex()));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"byteWriten: "+QString::number(byteWriten)+", size sending: "+QString::number(blockToSend.size()));
        }
        while(block.size());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"disconnect the socket");
        localSocket.disconnectFromServer();
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"connection failed, continu...");
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultracopierArguments: "+ultracopierArguments.join(";"));
        return false;
    }
}

/// the listen server
void LocalListener::listenServer()
{
    if(!QLocalServer::removeServer(ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME)))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to remove the listening server"));
    #ifndef Q_OS_MAC
    localServer.setSocketOptions(QLocalServer::UserAccessOption);
    #endif
    if(!localServer.listen(ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME)))
    {
        #ifndef Q_OS_MAC
        QMessageBox::critical(NULL,"Alert",QString("Ultracopier have not able to lock unique instance: %1").arg(localServer.errorString()));
        #endif
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Ultracopier have not able to lock unique instance: %1, error code: %2").arg(localServer.errorString()).arg((qint32)localServer.serverError()));
    }
    else
        connect(&localServer, &QLocalServer::newConnection, this, &LocalListener::newConnexion);
}

//the time is done
void LocalListener::timeoutDectected()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"start");
    while(clientList.size()>0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"clientList.first().size: "+QString::number(clientList.first().size));
        clientList.removeFirst();
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Timeout while recomposing data from connected clients");
    QMessageBox::warning(NULL,tr("Warning"),tr("Timeout while recomposing data from connected clients"));
}

/// \brief Data is incomming
void LocalListener::dataIncomming()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start");
    // 1 : we get packets from client

    //Which client send the message (Search of the QLocalSocket of client)
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == 0) // If not found
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"bad socket");
        return;
    }

    int index=-1;
    for (int i=0;i<clientList.size(); ++i) {
        if(clientList.at(i).socket==socket)
            index=i;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"socket->bytesAvailable() "+QString::number(socket->bytesAvailable()));
    if(index!=-1)
    {
        if(!clientList.at(index).haveData)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempComposed index found, but no have data, create new entry");
            // If all is ok we get the message
            QDataStream in(socket);
            in.setVersion(QDataStream::Qt_4_4);

            if (socket->bytesAvailable() < (int)sizeof(quint32)*2) // We have not receveive all the message, ignore because first int is cuted!
            {
                /*socket->readAll();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"wrong size for set the message size");*/
                return;
            }
            in >> clientList[index].size; // Store the size of the message
            clientList[index].size-=sizeof(int);

            // Check if all the message size is the same as the size given
            if(socket->bytesAvailable() < clientList.at(index).size) // If not all get then stop it
            {
                clientList[index].haveData=true;
                clientList[index].data.append(socket->readAll());
                TimeOutQLocalSocket.start();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Need wait to recomposite: "+QString::number(clientList.at(index).data.size())+", targeted: "+QString::number(clientList.at(index).size));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempComposed.data: "+QString(clientList.at(index).data.toHex()));
            }
            else if(socket->bytesAvailable() == clientList.at(index).size) //if the size match
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"socket->bytesAvailable(): "+QString::number(socket->bytesAvailable())+", for total of: "+QString::number(socket->bytesAvailable()+sizeof(quint32)));
                QStringList ultracopierArguments;
                in >> ultracopierArguments;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultracopierArguments: "+ultracopierArguments.join(";"));
                emit cli(ultracopierArguments,true,false);
                clientList[index].data.clear();
                clientList[index].haveData=false;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"socket->bytesAvailable(): "+QString::number(socket->bytesAvailable())+" > clientList.at(index).size!: "+QString::number(clientList.at(index).size));
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Query recomposed with this size: "+QString::number(clientList.at(index).data.size()));
            clientList[index].data.append(socket->readAll());
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Query recomposed with this size: "+QString::number(clientList.at(index).data.size()));
            if(clientList.at(index).data.size()==clientList.at(index).size)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"QByteArray reconstruction finished");
                QDataStream in(clientList.at(index).data);
                QStringList ultracopierArguments;
                in >> ultracopierArguments;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultracopierArguments: "+ultracopierArguments.join(";"));
                emit cli(ultracopierArguments,true,false);
                clientList[index].data.clear();
                clientList[index].haveData=false;
            }
            else
            {
                TimeOutQLocalSocket.start();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Need wait to recomposite: "+QString::number(clientList.at(index).data.size())+", targeted: "+QString::number(clientList.at(index).size));
                return;
            }
        }
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Socket not found???");
}

/// \brief Deconnexion client
/// \todo Remove the data in wait linker with this socket
void LocalListener::deconnectClient()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");

    // Wich client leave
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == 0) // If not found
        return;
    for (int i = 0; i < clientList.size(); ++i) {
        if(clientList.at(i).socket==socket)
            clientList.removeAt(i);
    }
    socket->deleteLater();
}

/// LocalListener New connexion
void LocalListener::newConnexion()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start");
    composedData newClient;
    newClient.socket = localServer.nextPendingConnection();
    #ifdef ULTRACOPIER_DEBUG
    connect(newClient.socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error), this, &LocalListener::error);
    //connect(newClient.socket, &QLocalSocket::error, this, &LocalListener::error);
    //connect(newClient.socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(error(QLocalSocket::LocalSocketError)));
    #endif
    connect(newClient.socket, &QLocalSocket::readyRead, this, &LocalListener::dataIncomming);
    connect(newClient.socket, &QLocalSocket::disconnected, this, &LocalListener::deconnectClient);
    newClient.size=-1;
    newClient.haveData=false;
    clientList << newClient;
}

#ifdef ULTRACOPIER_DEBUG
/** \brief If error occured at socket
\param theErrorDefine The error define */
void LocalListener::error(QLocalSocket::LocalSocketError theErrorDefine)
{
    if(theErrorDefine!=QLocalSocket::PeerClosedError)
    {
        QLocalSocket *client=qobject_cast<QLocalSocket *>(QObject::sender());
        if(client!=NULL)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Value:"+QString::number(theErrorDefine)+", Error message: "+client->errorString());
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Value:"+QString::number(theErrorDefine));
    }
}
#endif

/// \can now parse the cli
void LocalListener::allPluginIsloaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QStringList ultracopierArguments=QCoreApplication::arguments();
    //remove excutable path because is useless (unsafe to use)
    ultracopierArguments.removeFirst();
    //add the current path to file full path resolution if needed
    ultracopierArguments.insert(0,QDir::currentPath());
    emit cli(ultracopierArguments,false,false);
}
