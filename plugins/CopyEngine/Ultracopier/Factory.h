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
#include <QTimer>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "qstorageinfo.h"
#include "StructEnumDefinition.h"
#include "ui_copyEngineOptions.h"
#include "CopyEngine.h"
#include "Environment.h"
#include "Filters.h"
#include "RenamingRules.h"

#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif
#ifdef Q_OS_LINUX
    #include <unistd.h>
#endif

#ifndef FACTORY_H
#define FACTORY_H

namespace Ui {
    class copyEngineOptions;
}

/** \brief to generate copy engine instance */
class CopyEngineFactory : public PluginInterface_CopyEngineFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.CopyEngineFactory/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_CopyEngineFactory)
public:
    CopyEngineFactory();
    ~CopyEngineFactory();
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
    bool canDoOnlyCopy() const;
    /// \brief to get the supported protocols for the source
    QStringList supportedProtocolsForTheSource() const;
    /// \brief to get the supported protocols for the destination
    QStringList supportedProtocolsForTheDestination() const;
    /// \brief to get the options of the copy engine
    QWidget * options();

private:
    Ui::copyEngineOptions *ui;
    QWidget* tempWidget;
    OptionInterface * optionsEngine;
    QStringList mountSysPoint;
    QList<QStorageInfo::DriveType> driveType;
    bool errorFound;
    FacilityInterface * facilityEngine;
    Filters *filters;
    RenamingRules *renamingRules;
    QStorageInfo storageInfo;
    QTimer lunchInitFunction;
    QStringList includeStrings,includeOptions,excludeStrings,excludeOptions;
    QString firstRenamingRule,otherRenamingRule;

#if defined(Q_OS_WIN32) || (defined(Q_OS_LINUX) && defined(_SC_PHYS_PAGES))
    static size_t getTotalSystemMemory();
#endif
private slots:
    void init();
    void setDoRightTransfer(bool doRightTransfer);
    void setKeepDate(bool keepDate);
    void setBlockSize(int blockSize);
    void setParallelBuffer(int parallelBuffer);
    void setSequentialBuffer(int sequentialBuffer);
    void setParallelizeIfSmallerThan(int parallelizeIfSmallerThan);
    void setAutoStart(bool autoStart);
    void setFolderCollision(int index);
    void setFolderError(int index);
    void setTransferAlgorithm(int index);
    void setCheckDestinationFolder();
    void showFilterDialog();
    void sendNewFilters(const QStringList &includeStrings,const QStringList &includeOptions,const QStringList &excludeStrings,const QStringList &excludeOptions);
    void doChecksum_toggled(bool);
    void checksumOnlyOnError_toggled(bool);
    void osBuffer_toggled(bool);
    void osBufferLimited_toggled(bool);
    void osBufferLimit_editingFinished();
    void checksumIgnoreIfImpossible_toggled(bool);
    void sendNewRenamingRules(const QString &firstRenamingRule, const QString &otherRenamingRule);
    void showRenamingRules();
    void updateBufferCheckbox();
    void logicalDriveChanged(const QString &, bool);
    void setFileCollision(int index);
    void setFileError(int index);
    void updatedBlockSize();
    void deletePartiallyTransferredFiles(bool checked);
    void renameTheOriginalDestination(bool checked);
    void checkDiskSpace(bool checked);
    void defaultDestinationFolderBrowse();
    void defaultDestinationFolder();
    void followTheStrictOrder(bool checked);
    void moveTheWholeFolder(bool checked);
    void on_inodeThreads_editingFinished();
public slots:
    void resetOptions();
    void newLanguageLoaded();
signals:
    void reloadLanguage() const;
    void haveDrive(const QStringList &mountSysPoint, const QList<QStorageInfo::DriveType> &driveType) const;
};

#endif // FACTORY_H
