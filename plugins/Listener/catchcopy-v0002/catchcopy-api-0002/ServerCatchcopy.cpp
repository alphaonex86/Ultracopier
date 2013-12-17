/** \file ServerCatchcopy.cpp
\brief Define the server of catchcopy
\author alpha_one_x86
\version 0002
\date 2010 */

#include "ServerCatchcopy.h"
#include "VariablesCatchcopy.h"
#include "ExtraSocketCatchcopy.h"

#include <QFile>

ServerCatchcopy::ServerCatchcopy()
{
    name="Default avanced copier";
    idNextClient=0;
    error_string="Unknown error";
    connect(&server, &QLocalServer::newConnection, this, &ServerCatchcopy::newConnection);
}

ServerCatchcopy::~ServerCatchcopy()
{
    close();
}

bool ServerCatchcopy::isListening() const
{
    return server.isListening();
}

void ServerCatchcopy::setName(const QString & name)
{
    this->name=name;
}

QString ServerCatchcopy::getName() const
{
    return name;
}

/// \brief to get a client list
QStringList ServerCatchcopy::clientsList() const
{
    QStringList clients;
    int index=0;
    int size=clientList.size();
    while(index<size)
    {
        clients << clientList[index].name;
        index++;
    }
    return clients;
}

bool ServerCatchcopy::listen()
{
    QLocalSocket socketTestConnection;
    pathSocket=ExtraSocketCatchcopy::pathSocket();
    socketTestConnection.connectToServer(pathSocket);
    if(socketTestConnection.waitForConnected(CATCHCOPY_COMMUNICATION_TIMEOUT))
    {
        error_string="Other server is listening";
        emit error(error_string);
        return false;
    }
    else
    {
        if(!server.removeServer(pathSocket))
        {
            error_string="Unable to remove the old server";
            emit error(error_string);
        }
        #ifndef Q_OS_MAC
        server.setSocketOptions(QLocalServer::UserAccessOption);
        #endif
        if(server.listen(pathSocket))
            return true;
        else
        {
            error_string=QStringLiteral("Unable to listen %1: %2").arg(pathSocket).arg(server.errorString());
            emit error(error_string);
            return false;
        }
    }
}

void ServerCatchcopy::close()
{
    if(server.isListening())
    {
        int index=0;
        while(index<clientList.size())
        {
            clientList.at(index).socket->disconnectFromServer();
            index++;
        }
        server.close();
        if(!server.removeServer(pathSocket))
        {
            error_string="Unable to remove the old server";
            emit error(error_string);
        }
    }
}

const QString ServerCatchcopy::errorStringServer() const
{
    return server.errorString();
}

const QString ServerCatchcopy::errorString() const
{
    return error_string;
}

/// \brief New connexion
void ServerCatchcopy::newConnection()
{
    while(server.hasPendingConnections())
    {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if(clientSocket!=NULL)
        {
            do
            {
                idNextClient++;
                if(idNextClient>2000000000)
                    idNextClient=0;
            } while(clientIdFound(idNextClient));
            Client newClient;
            newClient.id			= idNextClient;
            newClient.socket		= clientSocket;
            newClient.haveData		= false;
            newClient.firstProtocolReplied	= false;
            newClient.detectTimeOut		= new QTimer(this);
            newClient.detectTimeOut->setSingleShot(true);
            newClient.detectTimeOut->setInterval(CATCHCOPY_COMMUNICATION_TIMEOUT);
            newClient.name="Unknown";
            connect(newClient.socket,	static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),	this, &ServerCatchcopy::connectionError,Qt::QueuedConnection);
            connect(newClient.socket,	&QIODevice::readyRead,										this, &ServerCatchcopy::readyRead,Qt::QueuedConnection);
            connect(newClient.socket,	&QLocalSocket::disconnected,				 					this, &ServerCatchcopy::disconnected,Qt::QueuedConnection);
            connect(newClient.detectTimeOut,&QTimer::timeout,										this, &ServerCatchcopy::checkTimeOut,Qt::QueuedConnection);
            clientList << newClient;
            emit connectedClient(newClient.id);
        }
    }
}

bool ServerCatchcopy::clientIdFound(const quint32 &id) const
{
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).id==id)
            return true;
        index++;
    }
    return false;
}

/// \brief new error at connexion
void ServerCatchcopy::connectionError(const QLocalSocket::LocalSocketError &error)
{
    QLocalSocket *socket=qobject_cast<QLocalSocket *>(QObject::sender());
    if(socket==NULL)
    {
        qWarning() << "Unlocated client socket!";
        return;
    }
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).socket==socket)
        {
            if(error!=QLocalSocket::PeerClosedError)
            {
                qWarning() << "error detected for the client: " << index << ", type: " << error;
                clientList.at(index).socket->disconnectFromServer();
            }
            return;
        }
        index++;
    }
}

void ServerCatchcopy::disconnected()
{
    QLocalSocket *socket=qobject_cast<QLocalSocket *>(QObject::sender());
    if(socket==NULL)
    {
        qWarning() << "Unlocated client socket!";
        return;
    }
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).socket==socket)
        {
            const quint32 &id=clientList.at(index).id;
            //ClientList.at(index).socket->disconnectFromServer();//already disconnected
            delete clientList.at(index).detectTimeOut;
            clientList.at(index).socket->deleteLater();
            clientList.removeAt(index);
            emit disconnectedClient(id);
            return;
        }
        index++;
    }
    qWarning() << "Unlocated client!";
}

void ServerCatchcopy::disconnectClient(const quint32 &id)
{
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).id==id)
        {
            clientList.at(index).socket->disconnectFromServer();
            return;
        }
        index++;
    }
    qWarning() << "Unlocated client!";
}

void ServerCatchcopy::readyRead()
{
    QLocalSocket *socket=qobject_cast<QLocalSocket *>(QObject::sender());
    if(socket==NULL)
    {
        qWarning() << "Unlocated client socket!";
        return;
    }
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).socket==socket)
        {
            while(socket->bytesAvailable()>0)
            {
                if(!clientList.at(index).haveData)
                {
                    if(socket->bytesAvailable()<(int)sizeof(int))//ignore because first int is cuted!
                    {
                        /*error_string="Bytes available is not sufficient to do a int";
                        emit error(error_string);
                        disconnectClient(ClientList.at(index).id);*/
                        return;
                    }
                    QDataStream in(socket);
                    in.setVersion(QDataStream::Qt_4_4);
                    in >> clientList[index].dataSize;
                    clientList[index].dataSize-=sizeof(int);
                    if(clientList.at(index).dataSize>64*1024*1024) // 64MB
                    {
                        error_string="Reply size is >64MB, seam corrupted";
                        emit communicationError(error_string);
                        disconnectClient(clientList.at(index).id);
                        return;
                    }
                    if(clientList.at(index).dataSize<(int)(sizeof(int) //orderId
                             + sizeof(quint32) //returnCode
                             + sizeof(quint32) //string list size
                                ))
                    {
                        error_string="Reply size is too small to have correct code";
                        emit communicationError(error_string);
                        disconnectClient(clientList.at(index).id);
                        return;
                    }
                    clientList[index].haveData=true;
                }
                if(clientList.at(index).dataSize<(clientList.at(index).data.size()+socket->bytesAvailable()))
                    clientList[index].data.append(socket->read(clientList.at(index).dataSize-clientList.at(index).data.size()));
                else
                    clientList[index].data.append(socket->readAll());
                if(clientList.at(index).dataSize==(quint32)clientList.at(index).data.size())
                {
                    if(!checkDataIntegrity(clientList.at(index).data))
                    {
                        emit communicationError("Data integrity wrong: "+QString(clientList.at(index).data.toHex()));
                        clientList[index].data.clear();
                        clientList[index].haveData=false;
                        qWarning() << "Data integrity wrong";
                        return;
                    }
                    QStringList returnList;
                    quint32 orderId;
                    QDataStream in(clientList.at(index).data);
                    in.setVersion(QDataStream::Qt_4_4);
                    in >> orderId;
                    in >> returnList;
                    clientList[index].data.clear();
                    clientList[index].haveData=false;
                    if(clientList.at(index).queryNoReplied.contains(orderId))
                    {
                        emit communicationError("Duplicate query id");
                        qWarning() << "Duplicate query id";
                        return;
                    }
                    clientList[index].queryNoReplied << orderId;
                    if(!clientList.at(index).firstProtocolReplied && returnList.size()==2 && returnList.first()=="protocol")
                    {
                        clientList[index].firstProtocolReplied=true;
                        protocolSupported(clientList.at(index).id,orderId,(returnList.last()==CATCHCOPY_PROTOCOL_VERSION));
                    }
                    else
                        parseInput(clientList.at(index).id,orderId,returnList);
                }
            }
            if(clientList.at(index).haveData)
                clientList.at(index).detectTimeOut->start();
            else
                clientList.at(index).detectTimeOut->stop();
            return;
        }
        index++;
    }
    emit error("Unallocated client!");
    qWarning() << "Unallocated client!";
}

bool ServerCatchcopy::checkDataIntegrity(const QByteArray &data)
{
    quint32 orderId;
    qint32 listSize;
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    in >> orderId;
    in >> listSize;
    if(listSize>65535)
    {
        emit error("List size is wrong");
        qWarning() << "List size is wrong";
        return false;
    }
    int index=0;
    while(index<listSize)
    {
        qint32 stringSize;
        in >> stringSize;
        if(stringSize>65535)
        {
            emit error("String size is wrong");
            return false;
        }
        if(stringSize>(in.device()->size()-in.device()->pos()))
        {
            emit error(QStringLiteral("String size is greater than the data: %1>(%2-%3)").arg(stringSize).arg(in.device()->size()).arg(in.device()->pos()));
            return false;
        }
        in.device()->seek(in.device()->pos()+stringSize);
        index++;
    }
    if(in.device()->size()!=in.device()->pos())
    {
        emit error("Remaining data after string list parsing");
        return false;
    }
    return true;
}

void ServerCatchcopy::parseInput(const quint32 &client,const quint32 &orderId,const QStringList &returnList)
{
    switch(parseInputCurrentProtocol(client,orderId,returnList))
    {
        case Ok:
            emit newQuery(client,orderId,returnList);
        break;
        case Replied:
        break;
        case ExtensionWrong:
            //protocolExtensionSupported(client,orderId,false);
        break;
        case WrongArgument:
            incorrectArgument(client,orderId);
        break;
        case WrongArgumentListSize:
            incorrectArgumentListSize(client,orderId);
        break;
        case UnknowOrder:
            emit error("Unknown query");
            qWarning() << "Unknown query";
            unknowOrder(client,orderId);
        break;
    }
}

ServerCatchcopy::inputReturnType ServerCatchcopy::parseInputCurrentProtocol(const quint32 &client,const quint32 &orderId,const QStringList &returnList)
{
    if(returnList.size()==0)
        return WrongArgumentListSize;
    //if is supported
    QString firstArgument=returnList.first();
    if(firstArgument=="protocol")
    {
        if(returnList.size()!=2)
            return WrongArgumentListSize;
        emit askProtocolCompatibility(client,orderId,returnList.last());
        return Ok;
    }
    else if(firstArgument=="protocol extension")
    {
        if(returnList.size()>3 || returnList.size()<2)
            return WrongArgumentListSize;
        return ExtensionWrong;
    }
    else if(firstArgument=="client")
    {
        if(returnList.size()!=2)
            return WrongArgumentListSize;
        int index=0;
        int size=clientList.size();
        while(index<size)
        {
            if(clientList.at(index).id==client)
            {
                clientList[index].name=returnList.last();
                break;
            }
            index++;
        }
        emit clientName(client,returnList.last());
        clientRegistered(client,orderId);
        return Replied;
    }
    else if(firstArgument=="server")
    {
        if(returnList.size()!=2)
            return WrongArgumentListSize;
        if(returnList.last()!="name?")
            return WrongArgument;
        serverName(client,orderId,name);
        return Replied;
    }
    else if(firstArgument=="cp")
    {
        if(returnList.size()<3)
            return WrongArgumentListSize;
        QStringList sourceList=returnList;
        sourceList.removeFirst();
        sourceList.removeLast();
        emitNewCopy(client,orderId,sourceList,returnList.last());
        return Ok;
    }
    else if(firstArgument=="cp-?")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        QStringList sourceList=returnList;
        sourceList.removeFirst();
        emitNewCopyWithoutDestination(client,orderId,sourceList);
        return Ok;
    }
    else if(firstArgument=="mv")
    {
        if(returnList.size()<3)
            return WrongArgumentListSize;
        QStringList sourceList=returnList;
        sourceList.removeFirst();
        sourceList.removeLast();
        emitNewMove(client,orderId,sourceList,returnList.last());
        return Ok;
    }
    else if(firstArgument=="mv-?")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        QStringList sourceList=returnList;
        sourceList.removeFirst();
        emitNewMoveWithoutDestination(client,orderId,sourceList);
        return Ok;
    }
    else //if is not supported
        return UnknowOrder;
}

void ServerCatchcopy::emitNewCopyWithoutDestination(const quint32 &client,const quint32 &orderId,const QStringList &sources)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newCopyWithoutDestination(newAssociation.globalOrderId,sources);
}

void ServerCatchcopy::emitNewCopy(const quint32 &client,const quint32 &orderId,const QStringList &sources,const QString &destination)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newCopy(newAssociation.globalOrderId,sources,destination);
}

void ServerCatchcopy::emitNewMoveWithoutDestination(const quint32 &client,const quint32 &orderId,const QStringList &sources)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newMoveWithoutDestination(newAssociation.globalOrderId,sources);
}

void ServerCatchcopy::emitNewMove(const quint32 &client,const quint32 &orderId,const QStringList &sources,const QString &destination)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newMove(newAssociation.globalOrderId,sources,destination);
}

void ServerCatchcopy::copyFinished(const quint32 &globalOrderId,const bool &withError)
{
    int index=0;
    while(index<LinkGlobalToLocalClientList.size())
    {
        if(LinkGlobalToLocalClientList.at(index).globalOrderId==globalOrderId)
        {
            copyFinished(LinkGlobalToLocalClientList.at(index).idClient,LinkGlobalToLocalClientList.at(index).orderId,withError);
            LinkGlobalToLocalClientList.removeAt(index);
            orderList.removeOne(globalOrderId);
            return;
        }
        index++;
    }
}

void ServerCatchcopy::copyCanceled(const quint32 &globalOrderId)
{
    int index=0;
    while(index<LinkGlobalToLocalClientList.size())
    {
        if(LinkGlobalToLocalClientList.at(index).globalOrderId==globalOrderId)
        {
            copyCanceled(LinkGlobalToLocalClientList.at(index).idClient,LinkGlobalToLocalClientList.at(index).orderId);
            LinkGlobalToLocalClientList.removeAt(index);
            orderList.removeOne(globalOrderId);
            return;
        }
        index++;
    }
}

void ServerCatchcopy::reply(const quint32 &client,const quint32 &orderId,const quint32 &returnCode,const QString &returnString)
{
    reply(client,orderId,returnCode,QStringList() << returnString);
}

void ServerCatchcopy::reply(const quint32 &client,const quint32 &orderId,const quint32 &returnCode,const QStringList &returnList)
{
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).id==client)
        {
            if(clientList.at(index).socket->isValid() && clientList.at(index).socket->state()==QLocalSocket::ConnectedState)
            {
                if(!clientList.at(index).queryNoReplied.contains(orderId))
                {
                    qWarning() << "Reply to missing query or previously replied";
                    return;
                }
                clientList[index].queryNoReplied.removeOne(orderId);
                //cut string list and send it as block of 32KB
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << int(0);
                out << orderId;
                out << returnCode;
                out << returnList;
                out.device()->seek(0);
                out << block.size();
                do
                {
                    QByteArray blockToSend;
                    int byteWriten;
                    blockToSend=block.left(32*1024);//32KB
                    block.remove(0,blockToSend.size());
                    byteWriten = clientList[index].socket->write(blockToSend);
                    if(!clientList[index].socket->isValid())
                    {
                        error_string="Socket is not valid";
                        emit error(error_string);
                        return;
                    }
                    if(clientList[index].socket->error()!=QLocalSocket::UnknownSocketError && clientList[index].socket->error()!=QLocalSocket::PeerClosedError)
                    {
                        error_string="Error with socket: "+clientList[index].socket->errorString();
                        emit error(error_string);
                        return;
                    }
                    if(blockToSend.size()!=byteWriten)
                    {
                        error_string="All the bytes have not be written";
                        emit error(error_string);
                        return;
                    }
                }
                while(block.size());
            }
            else
            {
                error_string="Socket is not valid or not connected";
                emit error(error_string);
            }
            return;
        }
        index++;
    }
    qWarning() << "Client id not found:" << client;
}

void ServerCatchcopy::protocolSupported(const quint32 &client,const quint32 &orderId,const bool &value)
{
    if(value)
        reply(client,orderId,1000,"protocol supported");
    else
        reply(client,orderId,5003,"protocol not supported");
}

void ServerCatchcopy::incorrectArgumentListSize(const quint32 &client,const quint32 &orderId)
{
    reply(client,orderId,5000,"incorrect argument list size");
}

void ServerCatchcopy::incorrectArgument(const quint32 &client,const quint32 &orderId)
{
    reply(client,orderId,5001,"incorrect argument");
}

void ServerCatchcopy::clientRegistered(const quint32 &client,const quint32 &orderId)
{
    reply(client,orderId,1003,"client registered");
}

void ServerCatchcopy::serverName(const quint32 &client,const quint32 &orderId,const QString &name)
{
    reply(client,orderId,1004,name);
}

void ServerCatchcopy::copyFinished(const quint32 &client,const quint32 &orderId,const bool &withError)
{
    if(!withError)
        reply(client,orderId,1005,"finished");
    else
        reply(client,orderId,1006,"finished with error(s)");
}

void ServerCatchcopy::copyCanceled(const quint32 &client,const quint32 &orderId)
{
    reply(client,orderId,1007,"canceled");
}

void ServerCatchcopy::unknowOrder(const quint32 &client,const quint32 &orderId)
{
    reply(client,orderId,5002,"unknown order");
}

void ServerCatchcopy::checkTimeOut()
{
    QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
    if(timer==NULL)
    {
        qWarning() << "Unallocated client timer!";
        return;
    }
    int index=0;
    while(index<clientList.size())
    {
        if(clientList.at(index).detectTimeOut==timer)
        {
            clientList.at(index).detectTimeOut->stop();
            if(clientList.at(index).haveData)
            {
                error_string="The client is too long to send the next part of the reply: "+clientList.at(index).data;
                clientList[index].haveData=false;
                clientList[index].data.clear();
                clientList.at(index).socket->disconnectFromServer();
                emit error(error_string);
            }
            return;
        }
        index++;
    }
}

quint32 ServerCatchcopy::incrementOrderId()
{
    do
    {
        nextOrderId++;
        if(nextOrderId>2000000)
            nextOrderId=0;
    } while(orderList.contains(nextOrderId));
    return nextOrderId;
}
