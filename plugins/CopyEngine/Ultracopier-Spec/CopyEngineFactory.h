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
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.CopyEngineFactory/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_CopyEngineFactory)
    #endif
public:
    CopyEngineFactory();
    ~CopyEngineFactory();
    /// \brief to return the instance of the copy engine
    PluginInterface_CopyEngine * getInstance() override;
    /// \brief set the resources, to store options, to have facilityInterface
    void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,
                      FacilityInterface * facilityInterface,const bool &portableVersion) override;
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
    /// \brief to get if have pause
    bool havePause() override;

private:
    Ui::copyEngineOptions *ui;
    QWidget* tempWidget;
    OptionInterface * optionsEngine;
    bool errorFound;
    FacilityInterface * facilityEngine;
    Filters *filters;
    RenamingRules *renamingRules;
    QStorageInfo storageInfo;
    QTimer lunchInitFunction;
    std::vector<std::string> includeStrings,includeOptions,excludeStrings,excludeOptions;
    std::string firstRenamingRule,otherRenamingRule;

#if defined(Q_OS_WIN32) || (defined(Q_OS_LINUX) && defined(_SC_PHYS_PAGES))
    static size_t getTotalSystemMemory();
#endif
private slots:
    void init();
    void setDoRightTransfer(bool doRightTransfer);
    void setKeepDate(bool keepDate);
    void setOsSpecFlags(bool os_spec_flags);
    void setNativeCopy(bool native_copy);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(bool rsync);
    #endif
    void setFolderCollision(int index);
    void setFolderError(int index);
    void setCheckDestinationFolder();
    void setMkFullPath();
    void setChecksum();
    void showFilterDialog();
    void sendNewFilters(const std::vector<std::string> &includeStrings,const std::vector<std::string> &includeOptions,
                        const std::vector<std::string> &excludeStrings,const std::vector<std::string> &excludeOptions);
    void sendNewRenamingRules(const std::string &firstRenamingRule, const std::string &otherRenamingRule);
    void showRenamingRules();
    void setFileCollision(int index);
    void setFileError(int index);
    void deletePartiallyTransferredFiles(bool checked);
    void renameTheOriginalDestination(bool checked);
    void checkDiskSpace(bool checked);
    void defaultDestinationFolderBrowse();
    void defaultDestinationFolder();
    void followTheStrictOrder(bool checked);
    void ignoreBlackList(bool checked);
    void moveTheWholeFolder(bool checked);
    void on_inodeThreads_editingFinished();
    void on_bufferOtherDevice_editingFinished();
    void on_bufferSameDevice_editingFinished();
    void setBuffer(bool checked);
    void setAutoStart(bool autoStart);
public slots:
    void resetOptions() override;
    void newLanguageLoaded() override;
signals:
    void reloadLanguage() const;
};

#endif // FACTORY_H
