/** \file PluginInterface_Themes.h
\brief Define the interface of the plugin of type: copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_THEMES_H
#define PLUGININTERFACE_THEMES_H

#include <vector>
#include <string>
#include <QObject>
#include <QWidget>
#include <utility>
#include <QIcon>

#include "OptionInterface.h"
#include "FacilityInterface.h"
#include "../StructEnumDefinition.h"

/** \brief To define the interface between Ultracopier and the interface
 * */
class PluginInterface_Themes : public QWidget
{
    Q_OBJECT
    public slots:
        //send information about the copy
        /// \brief to set the action in progress
        virtual void actionInProgess(const Ultracopier::EngineActionInProgress&) = 0;

        /// \brief the new folder is listing
        virtual void newFolderListing(const std::string &path) = 0;
        /** \brief show the detected speed
         * in byte per seconds */
        virtual void detectedSpeed(const uint64_t &speed) = 0;
        /** \brief show the remaining time
         * time in seconds */
        virtual void remainingTime(const int &remainingSeconds) = 0;
        /// \brief set one error is detected
        virtual void errorDetected() = 0;
        /// \brief new error
        virtual void errorToRetry(const std::string &source,const std::string &destination,const std::string &error) = 0;
        /** \brief support speed limitation */
        virtual void setSupportSpeedLimitation(const bool &supportSpeedLimitationBool) = 0;
        /// \brief get action on the transfer list (add/move/remove)
        virtual void getActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList> &returnActions) = 0;
        /// \brief show the general progression
        virtual void setGeneralProgression(const uint64_t &current,const uint64_t &total) = 0;
        /// \brief show the file progression
        virtual void setFileProgression(const std::vector<Ultracopier::ProgressionItem> &progressionList) = 0;
        /// to get by file speed
        virtual void doneTime(const std::vector<std::pair<uint64_t,uint32_t> > &timeList) = 0;
    public:
        /// \brief get the widget for the copy engine
        virtual QWidget * getOptionsEngineWidget() = 0;
        /// \brief to set if the copy engine is found
        virtual void getOptionsEngineEnabled(const bool &isEnabled) = 0;
        /// \brief set the copyType -> file or folder
        virtual void setCopyType(const Ultracopier::CopyType&) = 0;
        /// \brief set the copyMove -> copy or move, to force in copy or move, else support both
        virtual void forceCopyMode(const Ultracopier::CopyMode&) = 0;
        /// \brief set if transfer list is exportable/importable
        virtual void setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation) = 0;
        /** \brief set if the order is external (like file manager copy)
         * to notify the interface, which can hide add folder/filer button */
        virtual void haveExternalOrder() = 0;
        /// \brief set if is in pause
        virtual void isInPause(const bool &isInPause) = 0;
        /// \brief set if is in pause
        virtual void havePause(const bool &havePause) = 0;
    // signal to implement
    signals:
        //set the transfer list
        void removeItems(const std::vector<uint64_t> &ids) const;
        void moveItemsOnTop(const std::vector<uint64_t> &ids) const;
        void moveItemsUp(const std::vector<uint64_t> &ids) const;
        void moveItemsDown(const std::vector<uint64_t> &ids) const;
        void moveItemsOnBottom(const std::vector<uint64_t> &ids) const;
        void exportTransferList() const;
        void importTransferList() const;
        void exportErrorIntoTransferList() const;
        //user ask ask to add folder (add it with interface ask source/destination)
        void userAddFolder(const Ultracopier::CopyMode &mode) const;
        void userAddFile(const Ultracopier::CopyMode &mode) const;
        void urlDropped(const std::vector<std::string> &urls) const;//absolute path, can start by file:/
        //action on the copy
        void pause() const;
        void resume() const;
        void skip(const uint64_t &id) const;
        void cancel() const;
        //edit the action
        void newSpeedLimitation(const uint64_t &speedLimitation) const;///< -1 if not able, 0 if disabled
};

/// \brief To define the interface for the factory to do themes instance
class PluginInterface_ThemesFactory : public QObject
{
    Q_OBJECT
    public:
        /// \brief to get one instance
        virtual PluginInterface_Themes * getInstance() = 0;
        /// \brief to set resources, writePath can be empty if read only mode
        virtual void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion) = 0;
        /// \brief to get the default options widget
        virtual QWidget * options() = 0;
        /// \brief to get a resource icon
        virtual QIcon getIcon(const std::string &fileName) const = 0;
    public slots:
        /// \brief to reset as default the local options
        virtual void resetOptions() = 0;
        /// \brief retranslate the language because the language have changed
        virtual void newLanguageLoaded() = 0;
    signals:
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
};

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
Q_DECLARE_INTERFACE(PluginInterface_ThemesFactory,"first-world.info.ultracopier.PluginInterface.ThemesFactory/2.0.0.0");
#endif

#endif // PLUGININTERFACE_THEMES_H
