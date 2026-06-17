/** \file ServerCatchcopy.cpp
\brief Define the server of catchcopy
\author alpha_one_x86*/

#include "ServerCatchcopy.h"
#include "VariablesCatchcopy.h"
#include "ExtraSocketCatchcopy.h"

#include <QFile>
#include <QDataStream>
#include <queue>
#include <vector>
#include <string>
//this is just to support clipboard
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>

#ifdef Q_OS_WIN32
#include <qt_windows.h>
#include <sddl.h>
#include <QWinEventNotifier>
#endif

std::string stringimplode2(const std::vector<std::string>& elems, const std::string &delim)
{
    std::string newString;
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        newString += (*ii);
        if ( ii + 1 != elems.end() ) {
            newString += delim;
        }
    }
    return newString;
}

ServerCatchcopy::ServerCatchcopy()
{
    nextOrderId=0;
    name="Default avanced copier";
    idNextClient=0;
    error_string="Unknown error";
#ifdef Q_OS_WIN32
    winPipeHandle=NULL;
    winConnectOverlapped=NULL;
    winSecurityDescriptor=NULL;
    winConnectNotifier=NULL;
    winListening=false;
#endif
    connect(&server, &QLocalServer::newConnection, this, &ServerCatchcopy::newConnection);
}

ServerCatchcopy::~ServerCatchcopy()
{
    close();
}

bool ServerCatchcopy::isListening() const
{
#ifdef Q_OS_WIN32
    return winListening;
#else
    return server.isListening();
#endif
}

void ServerCatchcopy::setName(const std::string & name)
{
    this->name=name;
}

std::string ServerCatchcopy::getName() const
{
    return name;
}

/// \brief to get a client list
std::vector<std::string> ServerCatchcopy::clientsList() const
{
    std::vector<std::string> clients;
    int index=0;
    int size=clientList.size();
    while(index<size)
    {
        clients.push_back(clientList[index].name);
        index++;
    }
    return clients;
}

bool ServerCatchcopy::listen()
{
    QLocalSocket socketTestConnection;
    pathSocket=ExtraSocketCatchcopy::pathSocket();
    socketTestConnection.connectToServer(QString::fromStdString(pathSocket));
    if(socketTestConnection.waitForConnected(CATCHCOPY_COMMUNICATION_TIMEOUT))
    {
        error_string="Other server is listening";
        emit error(error_string);
        return false;
    }
    else
    {
#ifdef Q_OS_WIN32
        // Create the pipe manually so that a Low integrity label can be attached
        // to it (see winBuildSecurityDescriptor()); QLocalServer's UserAccessOption
        // cannot do this and makes an elevated Ultracopier unreachable from a
        // medium integrity Explorer.
        return winListen();
#else
        if(!server.removeServer(QString::fromStdString(pathSocket)))
        {
            error_string="Unable to remove the old server";
            emit error(error_string);
        }
        #ifndef Q_OS_MAC
        server.setSocketOptions(QLocalServer::UserAccessOption);
        #endif
        if(server.listen(QString::fromStdString(pathSocket)))
            return true;
        else
        {
            error_string=QStringLiteral("Unable to listen %1: %2").arg(QString::fromStdString(pathSocket)).arg(server.errorString()).toStdString();
            emit error(error_string);
            return false;
        }
#endif
    }
}

void ServerCatchcopy::close()
{
#ifdef Q_OS_WIN32
    if(winListening)
    {
        int index=0;
        while(index<clientList.size())
        {
            clientList.at(index).socket->disconnectFromServer();
            index++;
        }
        winClose();
    }
#else
    if(server.isListening())
    {
        int index=0;
        while(index<clientList.size())
        {
            clientList.at(index).socket->disconnectFromServer();
            index++;
        }
        server.close();
        if(!server.removeServer(QString::fromStdString(pathSocket)))
        {
            error_string="Unable to remove the old server";
            emit error(error_string);
        }
    }
#endif
}

/// \brief where is the socket/named pipe
const std::string ServerCatchcopy::fullServerName() const
{
#ifdef Q_OS_WIN32
    return QString::fromStdWString(winPipeName).toStdString();
#else
    return server.fullServerName().toStdString();
#endif
}

const std::string ServerCatchcopy::errorStringServer() const
{
    return server.errorString().toStdString();
}

const std::string ServerCatchcopy::errorString() const
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
            registerClientSocket(clientSocket);
    }
}

/// \brief register a freshly connected client socket (shared by all platforms)
void ServerCatchcopy::registerClientSocket(QLocalSocket *clientSocket)
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
    #if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(newClient.socket,	static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),	this, &ServerCatchcopy::connectionError,Qt::QueuedConnection);
    #else
    connect(newClient.socket,	static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::errorOccurred),	this, &ServerCatchcopy::connectionError,Qt::QueuedConnection);
    #endif
    connect(newClient.socket,	&QIODevice::readyRead,										this, &ServerCatchcopy::readyRead,Qt::QueuedConnection);
    connect(newClient.socket,	&QLocalSocket::disconnected,				 					this, &ServerCatchcopy::disconnected,Qt::QueuedConnection);
    connect(newClient.detectTimeOut,&QTimer::timeout,										this, &ServerCatchcopy::checkTimeOut,Qt::QueuedConnection);
    clientList << newClient;
    emit connectedClient(newClient.id);
}

#ifdef Q_OS_WIN32
/* Build a security descriptor granting the current user full access to the pipe
 * (DACL) while labelling the object Low integrity (SACL, no-write-up). The Low
 * label is the important part: it lets a medium integrity Explorer connect even
 * when this process runs elevated, which is exactly the case that broke after
 * Windows/UAC changes. */
bool ServerCatchcopy::winBuildSecurityDescriptor()
{
    HANDLE token=NULL;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    DWORD tokenLen=0;
    GetTokenInformation(token, TokenUser, NULL, 0, &tokenLen);
    if(tokenLen==0)
    {
        CloseHandle(token);
        return false;
    }
    PTOKEN_USER tokenUser=(PTOKEN_USER)malloc(tokenLen);
    if(tokenUser==NULL)
    {
        CloseHandle(token);
        return false;
    }
    bool ok=GetTokenInformation(token, TokenUser, tokenUser, tokenLen, &tokenLen);
    CloseHandle(token);
    if(!ok)
    {
        free(tokenUser);
        return false;
    }
    LPWSTR sidString=NULL;
    ok=ConvertSidToStringSidW(tokenUser->User.Sid, &sidString);
    free(tokenUser);
    if(!ok)
        return false;
    // D: DACL -> Allow GENERIC_ALL (GA) to the current user SID only.
    const std::wstring dacl=std::wstring(L"D:(A;;GA;;;")+sidString+L")";
    LocalFree(sidString);
    PSECURITY_DESCRIPTOR pSD=NULL;
    // S: SACL -> Mandatory Label (ML), No-Write-Up (NW), Low integrity (LW). This
    // is what lets a medium/low integrity Explorer connect even when Ultracopier
    // runs elevated. Mandatory integrity labels only exist since Windows Vista, so
    // on Windows XP the ML ACE is rejected: detect that at runtime and fall back to
    // a DACL-only descriptor (XP has no UAC/integrity levels, the label is useless
    // there anyway). Doing it dynamically keeps a single binary working on XP, 7,
    // 10/11, in both 32 and 64 bits.
    const std::wstring sddlWithLabel=dacl+L"S:(ML;;NW;;;LW)";
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddlWithLabel.c_str(), SDDL_REVISION_1, &pSD, NULL))
    {
        if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(dacl.c_str(), SDDL_REVISION_1, &pSD, NULL))
            return false;
    }
    winSecurityDescriptor=pSD;
    return true;
}

bool ServerCatchcopy::winListen()
{
    winPipeName=std::wstring(L"\\\\.\\pipe\\")+QString::fromStdString(pathSocket).toStdWString();
    if(winSecurityDescriptor==NULL && !winBuildSecurityDescriptor())
    {
        error_string="Unable to build the named pipe security descriptor";
        emit error(error_string);
        return false;
    }
    if(!winCreatePipeInstance(true))
    {
        error_string="Unable to create the named pipe instance";
        emit error(error_string);
        return false;
    }
    winListening=true;
    return true;
}

/* Create one named pipe instance and arm an asynchronous ConnectNamedPipe whose
 * completion event is watched by a QWinEventNotifier, so accepting a client stays
 * fully integrated with the Qt event loop (no extra thread). */
bool ServerCatchcopy::winCreatePipeInstance(bool firstInstance)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength=sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor=winSecurityDescriptor;
    sa.bInheritHandle=FALSE;
    DWORD openMode=PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED;
    if(firstInstance)
        openMode|=FILE_FLAG_FIRST_PIPE_INSTANCE;
    // Same pipe mode as QLocalServer/QLocalSocket expect (byte stream, overlapped).
    HANDLE hPipe=CreateNamedPipeW(
        winPipeName.c_str(),
        openMode,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0, 0, 0, &sa);
    if(hPipe==INVALID_HANDLE_VALUE)
        return false;
    OVERLAPPED *ov=(OVERLAPPED *)winConnectOverlapped;
    if(ov==NULL)
    {
        ov=(OVERLAPPED *)malloc(sizeof(OVERLAPPED));
        if(ov==NULL)
        {
            CloseHandle(hPipe);
            return false;
        }
        winConnectOverlapped=ov;
    }
    ZeroMemory(ov, sizeof(OVERLAPPED));
    ov->hEvent=CreateEventW(NULL, TRUE, FALSE, NULL);
    if(ov->hEvent==NULL)
    {
        CloseHandle(hPipe);
        return false;
    }
    winPipeHandle=hPipe;
    BOOL connected=ConnectNamedPipe(hPipe, ov);
    DWORD err=GetLastError();
    if(connected==FALSE && err!=ERROR_IO_PENDING && err!=ERROR_PIPE_CONNECTED)
    {
        CloseHandle(ov->hEvent);
        ov->hEvent=NULL;
        CloseHandle(hPipe);
        winPipeHandle=NULL;
        return false;
    }
    winConnectNotifier=new QWinEventNotifier(ov->hEvent, this);
    connect(winConnectNotifier, &QWinEventNotifier::activated, this, &ServerCatchcopy::winAcceptCompleted);
    winConnectNotifier->setEnabled(true);
    // A client that connected between CreateNamedPipe and ConnectNamedPipe never
    // signals the event by itself, so trigger the notifier manually.
    if(connected==TRUE || err==ERROR_PIPE_CONNECTED)
        SetEvent(ov->hEvent);
    return true;
}

void ServerCatchcopy::winAcceptCompleted()
{
    OVERLAPPED *ov=(OVERLAPPED *)winConnectOverlapped;
    if(winConnectNotifier!=NULL)
    {
        winConnectNotifier->setEnabled(false);
        winConnectNotifier->deleteLater();
        winConnectNotifier=NULL;
    }
    HANDLE connectedPipe=(HANDLE)winPipeHandle;
    winPipeHandle=NULL;
    if(ov!=NULL && ov->hEvent!=NULL)
    {
        CloseHandle(ov->hEvent);
        ov->hEvent=NULL;
    }
    bool wrapped=false;
    if(connectedPipe!=NULL && connectedPipe!=INVALID_HANDLE_VALUE)
    {
        // Hand the connected pipe to a QLocalSocket: from here the existing
        // protocol code (readyRead/disconnected/reply) works unchanged. This is
        // exactly what QLocalServer does internally on Windows.
        QLocalSocket *clientSocket=new QLocalSocket(this);
        clientSocket->setSocketDescriptor((qintptr)connectedPipe, QLocalSocket::ConnectedState, QIODevice::ReadWrite);
        if(clientSocket->state()==QLocalSocket::ConnectedState)
        {
            registerClientSocket(clientSocket);
            wrapped=true;
        }
        else
            delete clientSocket;
    }
    if(!wrapped && connectedPipe!=NULL && connectedPipe!=INVALID_HANDLE_VALUE)
        CloseHandle(connectedPipe);
    // Keep listening for the next client.
    if(winListening)
        winCreatePipeInstance(false);
}

void ServerCatchcopy::winClose()
{
    winListening=false;
    if(winConnectNotifier!=NULL)
    {
        winConnectNotifier->setEnabled(false);
        winConnectNotifier->deleteLater();
        winConnectNotifier=NULL;
    }
    OVERLAPPED *ov=(OVERLAPPED *)winConnectOverlapped;
    if(winPipeHandle!=NULL && winPipeHandle!=INVALID_HANDLE_VALUE)
    {
        // CancelIo (not the Vista+ CancelIoEx) so shutdown also works on Windows
        // XP; the pending ConnectNamedPipe was issued from this same main thread.
        CancelIo((HANDLE)winPipeHandle);
        DisconnectNamedPipe((HANDLE)winPipeHandle);
        CloseHandle((HANDLE)winPipeHandle);
        winPipeHandle=NULL;
    }
    if(ov!=NULL)
    {
        if(ov->hEvent!=NULL)
        {
            CloseHandle(ov->hEvent);
            ov->hEvent=NULL;
        }
        free(ov);
        winConnectOverlapped=NULL;
    }
    if(winSecurityDescriptor!=NULL)
    {
        LocalFree(winSecurityDescriptor);
        winSecurityDescriptor=NULL;
    }
}
#endif

bool ServerCatchcopy::clientIdFound(const uint32_t &id) const
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
            const uint32_t id=clientList.at(index).id;//copy by value: the element is removed just below
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

void ServerCatchcopy::disconnectClient(const uint32_t &id)
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
                             + sizeof(uint32_t) //returnCode
                             + sizeof(uint32_t) //string list size
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
                if(clientList.at(index).dataSize==(uint32_t)clientList.at(index).data.size())
                {
                    if(!checkDataIntegrity(clientList.at(index).data))
                    {
                        emit communicationError("Data integrity wrong: "+QString(clientList.at(index).data.toHex()).toStdString());
                        clientList[index].data.clear();
                        clientList[index].haveData=false;
                        qWarning() << "Data integrity wrong";
                        return;
                    }
                    std::vector<std::string> returnList;
                    QStringList returnListQt;
                    uint32_t orderId;
                    QDataStream in(clientList.at(index).data);
                    in.setVersion(QDataStream::Qt_4_4);
                    in >> orderId;
                    in >> returnListQt;
                    {
                        int index=0;
                        while(index<returnListQt.size())
                        {
                            returnList.push_back(returnListQt.at(index).toStdString());
                            index++;
                        }
                    }
                    clientList[index].data.clear();
                    clientList[index].haveData=false;
                    if(clientList.at(index).queryNoReplied.contains(orderId))
                    {
                        emit communicationError("Duplicate query id");
                        qWarning() << "Duplicate query id";
                        return;
                    }
                    clientList[index].queryNoReplied << orderId;
                    if(!clientList.at(index).firstProtocolReplied && returnList.size()==2 && returnList.front()=="protocol")
                    {
                        clientList[index].firstProtocolReplied=true;
                        protocolSupported(clientList.at(index).id,orderId,(returnList.back()==CATCHCOPY_PROTOCOL_VERSION));
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
    uint32_t orderId;
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
            emit error(QStringLiteral("String size is greater than the data: %1>(%2-%3)").arg(stringSize).arg(in.device()->size()).arg(in.device()->pos()).toStdString());
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

void ServerCatchcopy::parseInput(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &returnList)
{
    const ServerCatchcopy::inputReturnType returnVal=parseInputCurrentProtocol(client,orderId,returnList);
    switch(returnVal)
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
            emit error("Unknown query: "+std::to_string(returnVal)+", with client: "+std::to_string(client)+", orderId: "+std::to_string(orderId)+", returnList: "+stringimplode2(returnList,", "));
            qWarning() << "Unknown query";
            unknowOrder(client,orderId);
        break;
    }
}

ServerCatchcopy::inputReturnType ServerCatchcopy::parseInputCurrentProtocol(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &returnList)
{
    if(returnList.size()==0)
        return WrongArgumentListSize;
    //if is supported
    std::string firstArgument=returnList.front();
    if(firstArgument=="protocol")
    {
        if(returnList.size()!=2)
            return WrongArgumentListSize;
        emit askProtocolCompatibility(client,orderId,returnList.back());
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
                clientList[index].name=returnList.back();
                break;
            }
            index++;
        }
        emit clientName(client,returnList.back());
        clientRegistered(client,orderId);
        return Replied;
    }
    else if(firstArgument=="server")
    {
        if(returnList.size()!=2)
            return WrongArgumentListSize;
        if(returnList.back()!="name?")
            return WrongArgument;
        serverName(client,orderId,name);
        return Replied;
    }
    else if(firstArgument=="cp")
    {
        if(returnList.size()<3)
            return WrongArgumentListSize;
        std::vector<std::string> sourceList=returnList;
        sourceList.erase(sourceList.cbegin());
        sourceList.pop_back();
        emitNewCopy(client,orderId,sourceList,returnList.back());
        return Ok;
    }
    else if(firstArgument=="CBcp")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        QClipboard *clipboard = QGuiApplication::clipboard();
        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"));
        #else
        const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"),Qt::SkipEmptyParts);
        #endif
        /*linux:
        file:///path/test/master.zip
        file:///path/test/New text file
        Windows:
        file://Z:/X
        */
        std::vector<std::string> sourceList;
        int index=0;
        while(index<l.size())
        {
            sourceList.push_back(l.at(index).toStdString());
            index++;
        }
        emitNewCopy(client,orderId,sourceList,returnList.back());
        return Ok;
    }
    else if(firstArgument=="cp-?")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        std::vector<std::string> sourceList=returnList;
        sourceList.erase(sourceList.cbegin());
        emitNewCopyWithoutDestination(client,orderId,sourceList);
        return Ok;
    }
    else if(firstArgument=="mv")
    {
        if(returnList.size()<3)
            return WrongArgumentListSize;
        std::vector<std::string> sourceList=returnList;
        sourceList.erase(sourceList.cbegin());
        sourceList.pop_back();
        emitNewMove(client,orderId,sourceList,returnList.back());
        return Ok;
    }
    else if(firstArgument=="CBmv")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        QClipboard *clipboard = QGuiApplication::clipboard();
        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"));
        #else
        const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"),Qt::SkipEmptyParts);
        #endif
        /*linux:
        file:///path/test/master.zip
        file:///path/test/New text file
        Windows:
        file://Z:/X
        */
        std::vector<std::string> sourceList;
        int index=0;
        while(index<l.size())
        {
            sourceList.push_back(l.at(index).toStdString());
            index++;
        }
        emitNewMove(client,orderId,sourceList,returnList.back());
        return Ok;
    }
    else if(firstArgument=="mv-?")
    {
        if(returnList.size()<2)
            return WrongArgumentListSize;
        std::vector<std::string> sourceList=returnList;
        sourceList.erase(sourceList.cbegin());
        emitNewMoveWithoutDestination(client,orderId,sourceList);
        return Ok;
    }
    else //if is not supported
        return UnknowOrder;
}

void ServerCatchcopy::emitNewCopyWithoutDestination(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newCopyWithoutDestination(newAssociation.globalOrderId,sources);
}

void ServerCatchcopy::emitNewCopy(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newCopy(newAssociation.globalOrderId,sources,destination);
}

void ServerCatchcopy::emitNewMoveWithoutDestination(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newMoveWithoutDestination(newAssociation.globalOrderId,sources);
}

void ServerCatchcopy::emitNewMove(const uint32_t &client,const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    LinkGlobalToLocalClient newAssociation;
    newAssociation.idClient=client;
    newAssociation.orderId=orderId;
    newAssociation.globalOrderId=incrementOrderId();
    LinkGlobalToLocalClientList << newAssociation;
    emit newMove(newAssociation.globalOrderId,sources,destination);
}

void ServerCatchcopy::copyFinished(const uint32_t &globalOrderId,const bool &withError)
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

void ServerCatchcopy::copyCanceled(const uint32_t &globalOrderId)
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

void ServerCatchcopy::reply(const uint32_t &client,const uint32_t &orderId,const uint32_t &returnCode,const std::string &returnString)
{
    std::vector<std::string> returnList;
    returnList.push_back(returnString);
    reply(client,orderId,returnCode,returnList);
}

void ServerCatchcopy::reply(const uint32_t &client,const uint32_t &orderId,const uint32_t &returnCode,const std::vector<std::string> &returnList)
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
                QStringList returnListQt;
                {
                    unsigned int index=0;
                    while(index<returnList.size())
                    {
                        returnListQt << QString::fromStdString(returnList.at(index));
                        index++;
                    }
                }
                out << returnListQt;
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
                        error_string="Error with socket: "+clientList[index].socket->errorString().toStdString();
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

void ServerCatchcopy::protocolSupported(const uint32_t &client,const uint32_t &orderId,const bool &value)
{
    if(value)
        reply(client,orderId,1000,"protocol supported");
    else
        reply(client,orderId,5003,"protocol not supported");
}

void ServerCatchcopy::incorrectArgumentListSize(const uint32_t &client,const uint32_t &orderId)
{
    reply(client,orderId,5000,"incorrect argument list size");
}

void ServerCatchcopy::incorrectArgument(const uint32_t &client,const uint32_t &orderId)
{
    reply(client,orderId,5001,"incorrect argument");
}

void ServerCatchcopy::clientRegistered(const uint32_t &client,const uint32_t &orderId)
{
    reply(client,orderId,1003,"client registered");
}

void ServerCatchcopy::serverName(const uint32_t &client,const uint32_t &orderId,const std::string &name)
{
    reply(client,orderId,1004,name);
}

void ServerCatchcopy::copyFinished(const uint32_t &client,const uint32_t &orderId,const bool &withError)
{
    if(!withError)
        reply(client,orderId,1005,"finished");
    else
        reply(client,orderId,1006,"finished with error(s)");
}

void ServerCatchcopy::copyCanceled(const uint32_t &client,const uint32_t &orderId)
{
    reply(client,orderId,1007,"canceled");
}

void ServerCatchcopy::unknowOrder(const uint32_t &client,const uint32_t &orderId)
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
                error_string="The client is too long to send the next part of the reply: "+QString(clientList.at(index).data.toHex()).toStdString();
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

uint32_t ServerCatchcopy::incrementOrderId()
{
    do
    {
        nextOrderId++;
        if(nextOrderId>2000000)
            nextOrderId=0;
    } while(orderList.contains(nextOrderId));
    return nextOrderId;
}
