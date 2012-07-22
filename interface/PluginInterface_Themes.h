/** \file PluginInterface_Themes.h
\brief Define the interface of the plugin of type: copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef PLUGININTERFACE_THEMES_H
#define PLUGININTERFACE_THEMES_H

#include <QStringList>
#include <QString>
#include <QObject>
#include <QWidget>
#include <QList>
#include <QPair>
#include <QUrl>
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
		virtual void actionInProgess(const EngineActionInProgress&) = 0;

		/// \brief the new folder is listing
		virtual void newFolderListing(const QString &path) = 0;
		/** \brief show the detected speed
		 * in byte per seconds */
		virtual void detectedSpeed(const quint64 &speed) = 0;
		/** \brief show the remaining time
		 * time in seconds */
		virtual void remainingTime(const int &remainingSeconds) = 0;
		/// \brief set the current collision action
		virtual void newCollisionAction(const QString &action) = 0;
		/// \brief set the current error action
		virtual void newErrorAction(const QString &action) = 0;
		/// \brief set one error is detected
		virtual void errorDetected() = 0;
		/** \brief the max speed used
		 * in byte per seconds, -1 if not able, 0 if disabled */
		virtual bool setSpeedLimitation(const qint64 &speedLimitation) = 0;
		/// \brief get action on the transfer list (add/move/remove)
		virtual void getActionOnList(const QList<returnActionOnCopyList> &returnActions) = 0;
		/// \brief show the general progression
		virtual void setGeneralProgression(const quint64 &current,const quint64 &total) = 0;
		/// \brief show the file progression
		virtual void setFileProgression(const QList<ProgressionItem> &progressionList) = 0;
	public:
		/// \brief get the widget for the copy engine
		virtual QWidget * getOptionsEngineWidget() = 0;
		/// \brief to set if the copy engine is found
		virtual void getOptionsEngineEnabled(const bool &isEnabled) = 0;
		/// \brief set collision action
		virtual void setCollisionAction(const QList<QPair<QString,QString> > &collisionActionList) = 0;
		/// \brief set error action
		virtual void setErrorAction(const QList<QPair<QString,QString> > &errorActionList) = 0;
		/// \brief set the copyType -> file or folder
		virtual void setCopyType(const CopyType&) = 0;
		/// \brief set the copyMove -> copy or move, to force in copy or move, else support both
		virtual void forceCopyMode(const CopyMode&) = 0;
		/// \brief set if transfer list is exportable/importable
		virtual void setTransferListOperation(const TransferListOperation &transferListOperation) = 0;
		/** \brief set if the order is external (like file manager copy)
		 * to notify the interface, which can hide add folder/filer button */
		virtual void haveExternalOrder() = 0;
		/// \brief set if is in pause
		virtual void isInPause(const bool &isInPause) = 0;
	// signal to implement
	signals:
		//set the transfer list
		void removeItems(const QList<int> &ids);
		void moveItemsOnTop(const QList<int> &ids);
		void moveItemsUp(const QList<int> &ids);
		void moveItemsDown(const QList<int> &ids);
		void moveItemsOnBottom(const QList<int> &ids);
		void exportTransferList();
		void importTransferList();
		//user ask ask to add folder (add it with interface ask source/destination)
		void userAddFolder(const CopyMode &mode);
		void userAddFile(const CopyMode &mode);
		void urlDropped(const QList<QUrl> &urls);
		//action on the copy
		void pause();
		void resume();
		void skip(const quint64 &id);
		void cancel();
		//edit the action
		void sendCollisionAction(const QString &action);
		void sendErrorAction(const QString &action);
		void newSpeedLimitation(const qint64 &speedLimitation);///< -1 if not able, 0 if disabled
};

/// \brief To define the interface for the factory to do themes instance
class PluginInterface_ThemesFactory : public QObject
{
	Q_OBJECT
	public:
		/// \brief to get one instance
		virtual PluginInterface_Themes * getInstance() = 0;
		/// \brief to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion) = 0;
		/// \brief to get the default options widget
		virtual QWidget * options() = 0;
		/// \brief to get a resource icon
		virtual QIcon getIcon(const QString &fileName) = 0;
	public slots:
		/// \brief to reset as default the local options
		virtual void resetOptions() = 0;
		/// \brief retranslate the language because the language have changed
		virtual void newLanguageLoaded() = 0;
	signals:
		/// \brief To debug source
		void debugInformation(const DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
};

Q_DECLARE_INTERFACE(PluginInterface_ThemesFactory,"first-world.info.ultracopier.PluginInterface.ThemesFactory/0.4.0.0");

#endif // PLUGININTERFACE_THEMES_H
