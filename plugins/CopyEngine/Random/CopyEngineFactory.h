/** \file factory.h
\brief Define the factory to create new instance
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "CopyEngine.h"

#ifndef FACTORY_H
#define FACTORY_H

namespace Ui {
    class copyEngineOptions;
}

/** \brief to generate copy engine instance */
class CopyEngineFactory : public PluginInterface_CopyEngineFactory
{
    Q_OBJECT
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.CopyEngineFactory/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_CopyEngineFactory)
    #endif
public:
    CopyEngineFactory();
    ~CopyEngineFactory();
    /// \brief to return the instance of the copy engine
    PluginInterface_CopyEngine * getInstance();
    /// \brief set the resources, to store options, to have facilityInterface
    void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion) override;
    //get mode allowed
    /// \brief define if can copy file, folder or both
    Ultracopier::CopyType getCopyType() override;
    /// \brief to return which kind of transfer list operation is supported
    Ultracopier::TransferListOperation getTransferListOperation() override;
    /// \brief define if can only copy, or copy and move
    bool canDoOnlyCopy() const override;
    /// \brief to get the supported protocols for the source
    std::vector<std::string> supportedProtocolsForTheSource() const override;
    /// \brief to get the supported protocols for the destination
    std::vector<std::string> supportedProtocolsForTheDestination() const override;
    /// \brief to get the options of the copy engine
    QWidget * options() override;
public slots:
    void resetOptions() override;
    void newLanguageLoaded() override;
};

#endif // FACTORY_H
