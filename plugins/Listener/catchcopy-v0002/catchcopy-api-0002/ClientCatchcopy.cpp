/** \file ClientCatchcopy.cpp
\brief Define the catchcopy client
\author alpha_one_x86
\version 0002
\date 2010 */

#include "ClientCatchcopy.h"
#include "VariablesCatchcopy.h"
#include "ExtraSocketCatchcopy.h"

ClientCatchcopy::ClientCatchcopy()
{
	disconnectedFromSocket();
	error_string="Unknown error";
	detectTimeOut.setSingleShot(true);
	detectTimeOut.setInterval(CATCHCOPY_COMMUNICATION_TIMEOUT); // the max time to without send packet
	connect(&socket,	SIGNAL(connected()),					this,	SIGNAL(connected()));
	connect(&socket,	SIGNAL(disconnected()),					this,	SIGNAL(disconnected()));
	connect(&socket,	SIGNAL(disconnected()),					this,	SLOT(disconnectedFromSocket()));
	connect(&socket,	SIGNAL(stateChanged(QLocalSocket::LocalSocketState)),	this,	SIGNAL(stateChanged(QLocalSocket::LocalSocketState)));
	connect(&socket,	SIGNAL(error(QLocalSocket::LocalSocketError)),		this,	SIGNAL(errorSocket(QLocalSocket::LocalSocketError)));
	connect(&socket,	SIGNAL(readyRead()),					this,	SLOT(readyRead()));
	connect(&detectTimeOut,	SIGNAL(timeout()),					this,	SLOT(checkTimeOut()));
	connect(&socket,	SIGNAL(connected()),					this,	SLOT(socketIsConnected()));
}

void ClientCatchcopy::checkTimeOut()
{
	if(haveData)
	{
		error_string="The server is too long to send the next part of the reply";
		emit error(error_string);
		disconnectFromServer();
	}
}

const QString ClientCatchcopy::errorString()
{
	return error_string;
}

void ClientCatchcopy::socketIsConnected()
{
	orderIdFirstSendProtocol=sendProtocol();
}

void ClientCatchcopy::connectToServer()
{
	socket.connectToServer(ExtraSocketCatchcopy::pathSocket());
}

void ClientCatchcopy::disconnectFromServer()
{
	socket.abort();
	socket.disconnectFromServer();
}

const QString ClientCatchcopy::errorStringSocket()
{
	return socket.errorString();
}

/// \brief to send stream of string list
quint32 ClientCatchcopy::sendRawOrderList(const QStringList & order)
{
	if(!socket.isValid())
	{
		error_string="Socket is not valid, try send: "+order.join(";");
		emit error(error_string);
		return -1;
	}
	if(socket.state()!=QLocalSocket::ConnectedState)
	{
		error_string="Socket is not connected "+QString::number(socket.state());
		emit error(error_string);
		return -1;
	}
	do
	{
		idNextOrder++;
		if(idNextOrder>2000000000)
			idNextOrder=0;
	} while(notRepliedQuery.contains(idNextOrder));
	notRepliedQuery << idNextOrder;
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << int(0);
	out << idNextOrder;
	out << order;
	out.device()->seek(0);
	out << block.size();
	if(idNextOrder!=1) // drop if internal protocol send
	{
		emit dataSend(idNextOrder,block);
		emit dataSend(idNextOrder,order);
	}
	do //cut string list and send it as block of 32KB
	{
		QByteArray blockToSend;
		int byteWriten;
		blockToSend=block.left(32*1024);//32KB
		block.remove(0,blockToSend.size());
		byteWriten = socket.write(blockToSend);
		if(!socket.isValid())
		{
			error_string="Socket is not valid";
			emit error(error_string);
			return -1;
		}
		if(socket.errorString()!="Unknown error" && socket.errorString()!="")
		{
			error_string=socket.errorString();
			emit error(error_string);
			return -1;
		}
		if(blockToSend.size()!=byteWriten)
		{
			error_string="All the bytes have not be written";
			emit error(error_string);
			return -1;
		}
	}
	while(block.size());
	return idNextOrder;
}

void ClientCatchcopy::readyRead()
{
	while(socket.bytesAvailable()>0)
	{
		if(!haveData)
		{
			if(socket.bytesAvailable()<(int)sizeof(int))//int of size cuted
			{
			/*	error_string="Bytes available is not sufficient to do a int";
				emit error(error_string);
				disconnectFromServer();*/
				return;
			}
			QDataStream in(&socket);
			in.setVersion(QDataStream::Qt_4_4);
			in >> dataSize;
			dataSize-=sizeof(int);
			if(dataSize>64*1024*1024) // 64MB
			{
				error_string="Reply size is >64MB, seam corrupted";
				emit error(error_string);
				disconnectFromServer();
				return;
			}
			if(dataSize<(int)(sizeof(int) //orderId
				     + sizeof(quint32) //returnCode
				     + sizeof(quint32) //string list size
						))
			{
				error_string="Reply size is too small to have correct code";
				emit error(error_string);
				disconnectFromServer();
				return;
			}
		}
		if(dataSize<(data.size()+socket.bytesAvailable()))
			data.append(socket.read(dataSize-data.size()));
		else
			data.append(socket.readAll());
		if(dataSize==data.size())
		{
			if(!checkDataIntegrity(data))
			{
				data.clear();
				qWarning() << "Data of the reply is wrong";
				return;
			}
			QStringList returnList;
			quint32 orderId;
			quint32 returnCode;
			QDataStream in(data);
			in.setVersion(QDataStream::Qt_4_4);
			in >> orderId;
			in >> returnCode;
			in >> returnList;
			data.clear();
			if(orderId!=orderIdFirstSendProtocol)
			{
				if(!notRepliedQuery.contains(orderId))
					qWarning() << "Unknown query not replied:" << orderId;
				else
				{
					if(!parseReply(orderId,returnCode,returnList))
						emit unknowReply(orderId);
					emit newReply(orderId,returnCode,returnList);
				}
			}
			else
			{
				if(!sendProtocolReplied)
				{
					sendProtocolReplied=true;
					if(returnCode!=1000)
					{
						error_string="Protocol not supported";
						emit error(error_string);
						disconnectFromServer();
						return;
					}
				}
				else
				{
					error_string=QStringLiteral("First send protocol send with the query id %1 have been already previously replied").arg(orderIdFirstSendProtocol);
					emit error(error_string);
					disconnectFromServer();
					return;
				}
			}
		}
	}
	if(haveData)
		detectTimeOut.start();
	else
		detectTimeOut.stop();
}

bool ClientCatchcopy::checkDataIntegrity(QByteArray data)
{
	quint32 orderId;
	qint32 replyCode;
	qint32 listSize;
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	in >> orderId;
	in >> replyCode;
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
			qWarning() << "String size is wrong";
			return false;
		}
		if(stringSize>(in.device()->size()-in.device()->pos()))
		{
			emit error(QStringLiteral("String size is greater than the data: %1>(%2-%3)").arg(stringSize).arg(in.device()->size()).arg(in.device()->pos()));
			qWarning() << QStringLiteral("String size is greater than the data: %1>(%2-%3)").arg(stringSize).arg(in.device()->size()).arg(in.device()->pos());
			return false;
		}
		in.device()->seek(in.device()->pos()+stringSize);
		index++;
	}
	if(in.device()->size()!=in.device()->pos())
	{
		emit error("Remaining data after string list parsing");
		qWarning() << "Remaining data after string list parsing";
		return false;
	}
	return true;
}

QLocalSocket::LocalSocketState ClientCatchcopy::state()
{
	return socket.state();
}

void ClientCatchcopy::disconnectedFromSocket()
{
	haveData		= false;
	orderIdFirstSendProtocol= 0;
	idNextOrder		= 0;
	sendProtocolReplied	= false;
	notRepliedQuery.clear();
}

/// \brief to send the protocol version used
quint32 ClientCatchcopy::sendProtocol()
{
	return sendRawOrderList(QStringList() << "protocol" << CATCHCOPY_PROTOCOL_VERSION);
}

quint32 ClientCatchcopy::askServerName()
{
	return sendRawOrderList(QStringList() << "server" << "name?");
}

quint32 ClientCatchcopy::setClientName(const QString & name)
{
	return sendRawOrderList(QStringList() << "client" << name);
}

quint32 ClientCatchcopy::checkProtocolExtension(const QString & name)
{
	return sendRawOrderList(QStringList() << "protocol extension" << name);
}

quint32 ClientCatchcopy::checkProtocolExtension(const QString & name,const QString & version)
{
	return sendRawOrderList(QStringList() << "protocol extension" << name << version);
}

quint32 ClientCatchcopy::addCopyWithDestination(const QStringList & sources,const QString & destination)
{
	return sendRawOrderList(QStringList() << "cp" << sources << destination);
}

quint32 ClientCatchcopy::addCopyWithoutDestination(const QStringList & sources)
{
	return sendRawOrderList(QStringList() << "cp-?" << sources);
}

quint32 ClientCatchcopy::addMoveWithDestination(const QStringList & sources,const QString & destination)
{
	return sendRawOrderList(QStringList() << "mv" << sources << destination);
}

quint32 ClientCatchcopy::addMoveWithoutDestination(const QStringList & sources)
{
	return sendRawOrderList(QStringList() << "mv-?" << sources);
}

bool ClientCatchcopy::parseReply(quint32 orderId,quint32 returnCode,QStringList returnList)
{
	switch(returnCode)
	{
		case 1000:
			emit protocolSupported(orderId);
		break;
		case 1001:
		case 1002:
			if(returnCode==1001)
				emit protocolExtensionSupported(orderId,true);
			else
				emit protocolExtensionSupported(orderId,false);
		break;
		case 1003:
			emit clientRegistered(orderId);
		break;
		case 1004:
			if(returnList.size()!=1)
				emit unknowOrder(orderId);
			else
				emit serverName(orderId,returnList.last());
		break;
		case 1005:
		case 1006:
			if(returnCode==1005)
				emit copyFinished(orderId,false);
			else
				emit copyFinished(orderId,true);
		break;
		case 1007:
			emit copyCanceled(orderId);
		break;
		case 5000:
			emit incorrectArgumentListSize(orderId);
		break;
		case 5001:
			emit incorrectArgument(orderId);
		break;
		case 5002:
			emit unknowOrder(orderId); //the server have not understand the order
		break;
		case 5003:
			emit protocolNotSupported(orderId);
		break;
		default:
			return false;
	}
	return true;
}

