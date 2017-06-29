/** \file PluginInterface_CopyEngine.h
\brief Define the interface of the plugin of type: copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_COPYENGINE_H
#define PLUGININTERFACE_COPYENGINE_H

#include <QObject>
#include <QWidget>
#include <string>
#include <vector>

#include "OptionInterface.h"
#include "FacilityInterface.h"
#include "../StructEnumDefinition.h"

/** \brief To define the interface between Ultracopier and the copy engine
 * This interface support:
 * - Multiple transfer and multiple transfer progression
 * - Only the copy mode (source as read only)
 * - Only manipulation of folder, file, file + folder
 * - Change on live file size
 * - Speed is calculated by ultracopier in using the size \see getGeneralProgression()
 * - Support speed limitation if the engine provide it, else inform Ultracopier
 * - File/Folder can be added interactively or not
 * - The collision and error is managed by the plug-in to have useful information (like preview, size, date, ...)
 * **/
class PluginInterface_CopyEngine : public QObject
{
    Q_OBJECT
    public:
        /** \brief to send the options panel
         * \return return false if have not the options
         * \param tempWidget the widget to generate on it the options */
        virtual bool getOptionsEngine(QWidget * tempWidget) = 0;
        /** \brief to have interface widget to do modal dialog
         * \param interface to have the widget of the interface, useful for modal dialog */
        virtual void setInterfacePointer(QWidget * interface) = 0;
        /** \brief compare the current sources of the copy, with the passed arguments
         * \param sources the sources list to compares with the current sources list
         * \return true if have same sources, else false (or empty) */
        virtual bool haveSameSource(const std::vector<std::string> &sources) = 0;
        /** \brief compare the current destination of the copy, with the passed arguments
         * \param destination the destination to compares with the current destination
         * \return true if have same destination, else false (or empty) */
        virtual bool haveSameDestination(const std::string &destination) = 0;


        //external soft like file browser have send copy/move list to do
        /** \brief send copy without destination, ask the destination
         * \param sources the sources list to copy
         * \return true if the copy have been accepted */
        virtual bool newCopy(const std::vector<std::string> &sources) = 0;
        /** \brief send copy with destination
         * \param sources the sources list to copy
         * \param destination the destination to copy
         * \return true if the copy have been accepted */
        virtual bool newCopy(const std::vector<std::string> &sources,const std::string &destination) = 0;
        /** \brief send move without destination, ask the destination
         * \param sources the sources list to move
         * \return true if the move have been accepted */
        virtual bool newMove(const std::vector<std::string> &sources) = 0;
        /** \brief send move without destination, ask the destination
         * \param sources the sources list to move
         * \param destination the destination to move
         * \return true if the move have been accepted */
        virtual bool newMove(const std::vector<std::string> &sources,const std::string &destination) = 0;
        /** \brief send the new transfer list
         * \param file the transfer list */
        virtual void newTransferList(const std::string &file) = 0;


        /** \brief to get byte read, use by Ultracopier for the speed calculation
         * real size transfered to right speed calculation */
        virtual uint64_t realByteTransfered() = 0;


        /** \brief support speed limitation */
        virtual bool supportSpeedLimitation() const = 0;

        //transfer list
        /** \brief to sync the transfer list
         * Used when the interface is changed, useful to minimize the memory size */
        virtual void syncTransferList() = 0;
    public slots:
        //user ask ask to add folder (add it with interface ask source/destination)
        /** \brief add folder called on the interface
         * Used by manual adding */
        virtual bool userAddFolder(const Ultracopier::CopyMode &mode) = 0;
        /** \brief add file called on the interface
         * Used by manual adding */
        virtual bool userAddFile(const Ultracopier::CopyMode &mode) = 0;


        //action on the transfer
        /// \brief put the transfer in pause
        virtual void pause() = 0;
        /// \brief resume the transfer
        virtual void resume() = 0;
        /** \brief skip one transfer entry
         * \param id id of the file to remove */
        virtual void skip(const quint64 &id) = 0;
        /// \brief cancel all the transfer
        virtual void cancel() = 0;


        //edit the transfer list
        /** \brief remove the selected item
         * \param ids ids is the id list of the selected items */
        virtual void removeItems(const std::vector<int> &ids) = 0;
        /** \brief move on top of the list the selected item
         * \param ids ids is the id list of the selected items */
        virtual void moveItemsOnTop(const std::vector<int> &ids) = 0;
        /** \brief move up the list the selected item
         * \param ids ids is the id list of the selected items */
        virtual void moveItemsUp(const std::vector<int> &ids) = 0;
        /** \brief move down the list the selected item
         * \param ids ids is the id list of the selected items */
        virtual void moveItemsDown(const std::vector<int> &ids) = 0;
        /** \brief move on bottom of the list the selected item
         * \param ids ids is the id list of the selected items */
        virtual void moveItemsOnBottom(const std::vector<int> &ids) = 0;


        /** \brief give the forced mode, to export/import transfer list */
        virtual void forceMode(const Ultracopier::CopyMode &mode) = 0;
        /// \brief export the transfer list into a file
        virtual void exportTransferList() = 0;
        /// \brief export the transfer list into a file
        virtual void exportErrorIntoTransferList() = 0;
        /// \brief import the transfer list into a file
        virtual void importTransferList() = 0;


        /** \brief to set the speed limitation
         * -1 if not able, 0 if disabled */
        virtual bool setSpeedLimitation(const int64_t &speedLimitation) = 0;
    // signal to implement
    signals:
        //send information about the copy
        void actionInProgess(const Ultracopier::EngineActionInProgress &engineActionInProgress) const;	//should update interface information on this event

        void newFolderListing(const std::string &path) const;
        void isInPause(const bool &isInPause) const;

        void newActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList>&) const;///very important, need be temporized to group the modification to do and not flood the interface
        void doneTime(const std::vector<std::pair<uint64_t,uint32_t> >&) const;
        void syncReady() const;

        /** \brief to get the progression for a specific file
         * \param id the id of the transfer, id send during population the transfer list
         * first = current transfered byte, second = byte to transfer */
        void pushFileProgression(const std::vector<Ultracopier::ProgressionItem> &progressionList) const;
        //get information about the copy
        /** \brief to get the general progression
         * first = current transfered byte, second = byte to transfer */
        void pushGeneralProgression(const uint64_t &,const uint64_t &) const;

        //when the cancel is clicked on copy engine dialog
        void cancelAll() const;

        //when can be deleted
        void canBeDeleted() const;

        //send error occurred
        void error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error) const;
        void errorToRetry(const std::string &source,const std::string &destination,const std::string &error) const;
        //for the extra logging
        void rmPath(const std::string &path) const;
        void mkPath(const std::string &path) const;
};

/// \brief To define the interface for the factory to do copy engine instance
class PluginInterface_CopyEngineFactory : public QObject
{
    Q_OBJECT
    public:
        /// \brief to get one instance
        virtual PluginInterface_CopyEngine * getInstance() = 0;
        /// \brief to set resources, writePath can be empty if read only mode
        virtual void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion) = 0;
        //get mode allowed
        /// \brief define if can copy file, folder or both
        virtual Ultracopier::CopyType getCopyType() = 0;
        /// \brief define if can import/export or nothing
        virtual Ultracopier::TransferListOperation getTransferListOperation() = 0;
        /// \brief define if can only copy, or copy and move
        virtual bool canDoOnlyCopy() const = 0;
        /// \brief to get the supported protocols for the source
        virtual std::vector<std::string> supportedProtocolsForTheSource() const = 0;
        /// \brief to get the supported protocols for the destination
        virtual std::vector<std::string> supportedProtocolsForTheDestination() const = 0;
        /// \brief to get the options of the copy engine
        virtual QWidget * options() = 0;
    public slots:
        /// \brief to reset the options
        virtual void resetOptions() = 0;
        /// \brief to reload the translation, because the new language have been loaded
        virtual void newLanguageLoaded() = 0;
    signals:
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
};

Q_DECLARE_INTERFACE(PluginInterface_CopyEngineFactory,"first-world.info.ultracopier.PluginInterface.CopyEngineFactory/1.2.4.0");

#endif // PLUGININTERFACE_COPYENGINE_H
