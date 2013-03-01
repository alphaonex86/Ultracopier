/** \file factory.h
\brief Define the factory to create new instance
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "StructEnumDefinition_CopyEngine.h"

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QProcess>
#include <QStorageInfo>
#include <QTimer>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "StructEnumDefinition.h"
#include "ui_options.h"
#include "CopyEngine.h"
#include "Environment.h"
#include "Filters.h"
#include "RenamingRules.h"

#ifndef FACTORY_H
#define FACTORY_H

namespace Ui {
    class options;
}

/** \brief to generate copy engine instance */
class Factory : public PluginInterface_CopyEngineFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.CopyEngineFactory/0.4.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_CopyEngineFactory)
public:
    Factory();
    ~Factory();
    /// \brief to return the instance of the copy engine
    PluginInterface_CopyEngine * getInstance();
    /// \brief set the resources, to store options, to have facilityInterface
    void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion);
    //get mode allowed
    /// \brief define if can copy file, folder or both
    Ultracopier::CopyType getCopyType();
    /// \brief to return which kind of transfer list operation is supported
    Ultracopier::TransferListOperation getTransferListOperation();
    /// \brief define if can only copy, or copy and move
    bool canDoOnlyCopy();
    /// \brief to get the supported protocols for the source
    QStringList supportedProtocolsForTheSource();
    /// \brief to get the supported protocols for the destination
    QStringList supportedProtocolsForTheDestination();
    /// \brief to get the options of the copy engine
    QWidget * options();
private:
    Ui::options *ui;
    QWidget* tempWidget;
    OptionInterface * optionsEngine;
    QStringList mountSysPoint;
    bool errorFound;
    FacilityInterface * facilityEngine;
    Filters *filters;
    RenamingRules *renamingRules;
    QStorageInfo storageInfo;
    QTimer lunchInitFunction;
private slots:
    void init();
    void setDoRightTransfer(bool doRightTransfer);
    void setKeepDate(bool keepDate);
    void setBlockSize(int blockSize);
    void setAutoStart(bool autoStart);
    void setFolderCollision(int index);
    void setFolderError(int index);
    void setCheckDestinationFolder();
    void showFilterDialog();
    void sendNewFilters(const QStringList &includeStrings,const QStringList &includeOptions,const QStringList &excludeStrings,const QStringList &excludeOptions);
    void doChecksum_toggled(bool);
    void checksumOnlyOnError_toggled(bool);
    void osBuffer_toggled(bool);
    void osBufferLimited_toggled(bool);
    void osBufferLimit_editingFinished();
    void checksumIgnoreIfImpossible_toggled(bool);
    void sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
    void showRenamingRules();
    void updateBufferCheckbox();
    void logicalDriveChanged(const QString &, bool);
    void setFileCollision(int index);
    void setFileError(int index);
public slots:
    void resetOptions();
    void newLanguageLoaded();
signals:
    void reloadLanguage();
    void haveDrive(const QStringList &drives);
};

#endif // FACTORY_H
