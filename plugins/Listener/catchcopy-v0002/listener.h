/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SERVER_H
#define SERVER_H

#include <string>
#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include <QtPlugin>
#endif

#include "Environment.h"
#include "../../../interface/PluginInterface_Listener.h"
#include "catchcopy-api-0002/ServerCatchcopy.h"

/// \brief Define the server compatible with Ultracopier interface
class Listener : public PluginInterface_Listener
{
    Q_OBJECT
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.Listener/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_Listener)
    #endif
public:
    Listener();
    /// \brief try listen the copy/move
    void listen() override;
    /// \brief stop listen to copy/move
    void close() override;
    /// \brief return the error strong
    const std::string errorString() const override;
    /// \brief set resources for this plugins
    void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,const bool &portableVersion) override;
    /// \brief to get the options widget, NULL if not have
    QWidget * options() override;
    /// \brief to get a client list
    std::vector<std::string> clientsList() const override;
public slots:
    /// \brief say to the client that's the copy/move is finished
    void transferFinished(const uint32_t &orderId,const bool &withError) override;
    /// \brief say to the client that's the copy/move is finished
    void transferCanceled(const uint32_t &orderId) override;
    /// \brief to reload the translation, because the new language have been loaded
    void newLanguageLoaded() override;
private:
    ServerCatchcopy server;
private slots:
    void errorInternal(const std::string &string);
    void communicationErrorInternal(const std::string &string);
    void clientName(uint32_t client,std::string name);
    void copyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources);
    void copy(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
    void moveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &sources);
    void move(const uint32_t &orderId,const std::vector<std::string> &sources,const std::string &destination);
};

#endif // SERVER_H
