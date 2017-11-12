#include "listener.h"
#include "catchcopy-api-0002/ExtraSocketCatchcopy.h"
#include "../../../cpp11addition.h"

Listener::Listener()
{
    server.setName(tr("Ultracopier").toStdString());
    connect(&server,&ServerCatchcopy::newCopyWithoutDestination,		this,&Listener::copyWithoutDestination);
    connect(&server,&ServerCatchcopy::newCopy,                          this,&Listener::copy);
    connect(&server,&ServerCatchcopy::newMoveWithoutDestination,		this,&Listener::moveWithoutDestination);
    connect(&server,&ServerCatchcopy::newMove,                          this,&Listener::move);
    connect(&server,&ServerCatchcopy::error,                            this,&Listener::errorInternal);
    connect(&server,&ServerCatchcopy::communicationError,               this,&Listener::communicationErrorInternal);
    connect(&server,&ServerCatchcopy::clientName,                       this,&Listener::clientName);
    connect(&server,&ServerCatchcopy::clientName,                       this,&Listener::newClientList);
    connect(&server,&ServerCatchcopy::connectedClient,                  this,&Listener::newClientList);
    connect(&server,&ServerCatchcopy::disconnectedClient,               this,&Listener::newClientList);

}

void Listener::listen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+ExtraSocketCatchcopy::pathSocket());
    if(server.listen())
        emit newState(Ultracopier::FullListening);
    else
        emit newState(Ultracopier::NotListening);
}

void Listener::close()
{
    server.close();
    emit newState(Ultracopier::NotListening);
}

const std::string Listener::errorString() const
{
    return server.errorString();
}

void Listener::setResources(OptionInterface * options, const std::string &writePath, const std::string &pluginPath, const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * Listener::options()
{
    return NULL;
}

/// \brief to get a client list
std::vector<std::string> Listener::clientsList() const
{
    return server.clientsList();
}

void Listener::transferFinished(const uint32_t &orderId, const bool &withError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, orderId: "+std::to_string(orderId)+", withError: "+std::to_string(withError));
    server.copyFinished(orderId,withError);
}

void Listener::transferCanceled(const uint32_t &orderId)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, orderId: "+std::to_string(orderId));
    server.copyCanceled(orderId);
}

/// \brief to reload the translation, because the new language have been loaded
void Listener::newLanguageLoaded()
{
}

void Listener::errorInternal(const std::string &string)
{
    Q_UNUSED(string);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"warning emited from Catchcopy lib: "+string);
}

void Listener::communicationErrorInternal(const std::string &string)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"warning emited from Catchcopy lib: "+string);
    emit error(string);
}

void Listener::clientName(uint32_t client,std::string name)
{
    Q_UNUSED(client);
    Q_UNUSED(name);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("clientName: %1, for the id: %2").arg(QString::fromStdString(name)).arg(client).toStdString());
}

void Listener::copyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("copyWithoutDestination(%1,%2)")
                             .arg(orderId)
                             .arg(QString::fromStdString(stringimplode(sources,";")))
                             .toStdString()
                             );
    emit newCopyWithoutDestination(orderId,sources);
}

void Listener::copy(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("copy(%1,%2,%3)").arg(orderId).arg(QString::fromStdString(stringimplode(sources,";")).arg(QString::fromStdString(destination))).toStdString());
    emit newCopy(orderId,sources,destination);
}

void Listener::moveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("moveWithoutDestination(%1,%2)").arg(orderId).arg(QString::fromStdString(stringimplode(sources,";"))).toStdString());
    emit newMoveWithoutDestination(orderId,sources);
}

void Listener::move(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("move(%1,%2,%3)").arg(orderId).arg(QString::fromStdString(stringimplode(sources,";")).arg(QString::fromStdString(destination))).toStdString());
    emit newMove(orderId,sources,destination);
}
