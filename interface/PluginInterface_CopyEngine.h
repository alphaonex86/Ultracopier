/** \file PluginInterface_CopyEngine.h
\brief Define the interface of the plugin of type: copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef PLUGININTERFACE_COPYENGINE_H
#define PLUGININTERFACE_COPYENGINE_H

#include <QStringList>
#include <QString>
#include <QObject>
#include <QList>
#include <QPair>
#include <QWidget>
#include <QDateTime>

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
		virtual bool haveSameSource(const QStringList &sources) = 0;
		/** \brief compare the current destination of the copy, with the passed arguments
		 * \param destination the destination to compares with the current destination
		 * \return true if have same destination, else false (or empty) */
		virtual bool haveSameDestination(const QString &destination) = 0;
		
		
		//external soft like file browser have send copy/move list to do
		/** \brief send copy without destination, ask the destination
		 * \param sources the sources list to copy
		 * \return true if the copy have been accepted */
		virtual bool newCopy(const QStringList &sources) = 0;
		/** \brief send copy with destination
		 * \param sources the sources list to copy
		 * \param destination the destination to copy
		 * \return true if the copy have been accepted */
		virtual bool newCopy(const QStringList &sources,const QString &destination) = 0;
		/** \brief send move without destination, ask the destination
		 * \param sources the sources list to move
		 * \return true if the move have been accepted */
		virtual bool newMove(const QStringList &sources) = 0;
		/** \brief send move without destination, ask the destination
		 * \param sources the sources list to move
		 * \param destination the destination to move
		 * \return true if the move have been accepted */
		virtual bool newMove(const QStringList &sources,const QString &destination) = 0;
		/** \brief send the new transfer list
		 * \param file the transfer list */
		virtual void newTransferList(const QString &file) = 0;
		
		
		/** \brief to get byte read, use by Ultracopier for the speed calculation
		 * real size transfered to right speed calculation */
		virtual quint64 realByteTransfered() = 0;
		
		
		/** \brief get the speed limitation
		 * < -1 if not able, 0 if disabled */
		virtual qint64 getSpeedLimitation() = 0;
		
		
		//get user action
		/** \brief get the collision action list */
		virtual QList<QPair<QString,QString> > getCollisionAction() = 0;
		/** \brief get the collision error list */
		virtual QList<QPair<QString,QString> > getErrorAction() = 0;
		
		
		//transfer list
		/** \brief to sync the transfer list
		 * Used when the interface is changed, useful to minimize the memory size */
		virtual void syncTransferList() = 0;
	public slots:
		//user ask ask to add folder (add it with interface ask source/destination)
		/** \brief add folder called on the interface
		 * Used by manual adding */
		virtual bool userAddFolder(const CopyMode &mode) = 0;
		/** \brief add file called on the interface
		 * Used by manual adding */
		virtual bool userAddFile(const CopyMode &mode) = 0;
		
		
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
		virtual void removeItems(const QList<int> &ids) = 0;
		/** \brief move on top of the list the selected item
		 * \param ids ids is the id list of the selected items */
		virtual void moveItemsOnTop(const QList<int> &ids) = 0;
		/** \brief move up the list the selected item
		 * \param ids ids is the id list of the selected items */
		virtual void moveItemsUp(const QList<int> &ids) = 0;
		/** \brief move down the list the selected item
		 * \param ids ids is the id list of the selected items */
		virtual void moveItemsDown(const QList<int> &ids) = 0;
		/** \brief move on bottom of the list the selected item
		 * \param ids ids is the id list of the selected items */
		virtual void moveItemsOnBottom(const QList<int> &ids) = 0;


		/** \brief give the forced mode, to export/import transfer list */
		virtual void forceMode(const CopyMode &mode) = 0;
		/// \brief export the transfer list into a file
		virtual void exportTransferList() = 0;
		/// \brief import the transfer list into a file
		virtual void importTransferList() = 0;
		
		
		/** \brief to set the speed limitation
		 * -1 if not able, 0 if disabled */
		virtual bool setSpeedLimitation(const qint64 &speedLimitation) = 0;
		
		
		//action
		/// \brief to set the collision action
		virtual void setCollisionAction(const QString &action) = 0;
		/// \brief to set the error action
		virtual void setErrorAction(const QString &action) = 0;
	// signal to implement
	signals:
		//send information about the copy
		void actionInProgess(const EngineActionInProgress &engineActionInProgress);	//should update interface information on this event
		
		void newFolderListing(const QString &path);
		void newCollisionAction(const QString &action);
		void newErrorAction(const QString &action);
		void isInPause(const bool &isInPause);
		
		void newActionOnList(const QList<returnActionOnCopyList>&);///very important, need be temporized to group the modification to do and not flood the interface
		void syncReady();

		/** \brief to get the progression for a specific file
		 * \param id the id of the transfer, id send during population the transfer list
		 * first = current transfered byte, second = byte to transfer */
		void pushFileProgression(const QList<ProgressionItem> &progressionList);
		//get information about the copy
		/** \brief to get the general progression
		 * first = current transfered byte, second = byte to transfer */
		void pushGeneralProgression(const quint64 &,const quint64 &);
		
		//when the cancel is clicked on copy engine dialog
		void cancelAll();

		//when can be deleted
		void canBeDeleted();
		
		//send error occurred
		void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error);
		//for the extra logging
		void rmPath(const QString &path);
		void mkPath(const QString &path);
};

/// \brief To define the interface for the factory to do copy engine instance
class PluginInterface_CopyEngineFactory : public QObject
{
	Q_OBJECT
	public:
		/// \brief to get one instance
		virtual PluginInterface_CopyEngine * getInstance() = 0;
		/// \brief to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion) = 0;
		//get mode allowed
		/// \brief define if can copy file, folder or both
		virtual CopyType getCopyType() = 0;
		/// \brief define if can import/export or nothing
		virtual TransferListOperation getTransferListOperation() = 0;
		/// \brief define if can only copy, or copy and move
		virtual bool canDoOnlyCopy() = 0;
		/// \brief to get the supported protocols for the source
		virtual QStringList supportedProtocolsForTheSource() = 0;
		/// \brief to get the supported protocols for the destination
		virtual QStringList supportedProtocolsForTheDestination() = 0;
		/// \brief to get the options of the copy engine
		virtual QWidget * options() = 0;
	public slots:
		/// \brief to reset the options
		virtual void resetOptions() = 0;
		/// \brief to reload the translation, because the new language have been loaded
		virtual void newLanguageLoaded() = 0;
	signals:
		/// \brief To debug source
		void debugInformation(const DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
};

Q_DECLARE_INTERFACE(PluginInterface_CopyEngineFactory,"first-world.info.ultracopier.PluginInterface.CopyEngineFactory/0.4.0.0");

#endif // PLUGININTERFACE_COPYENGINE_H
