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

/** This interface support:
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
		virtual bool getOptionsEngine(QWidget * tempWidget) = 0;				//to send the options panel
		virtual void setInterfacePointer(QWidget * interface) = 0;		//to have interface widget to do modal dialog
		//return empty if multiple
		virtual bool haveSameSource(QStringList sources) = 0;
		virtual bool haveSameDestination(QString destination) = 0;
		//external soft like file browser have send copy/move list to do
		virtual bool newCopy(QStringList sources) = 0;
		virtual bool newCopy(QStringList sources,QString destination) = 0;
		virtual bool newMove(QStringList sources) = 0;
		virtual bool newMove(QStringList sources,QString destination) = 0;
		//get information about the copy
		virtual QPair<quint64,quint64> getGeneralProgression() = 0;			//first = current transfered byte, second = byte to transfer
		virtual returnSpecificFileProgression getFileProgression(quint64 id) = 0;	//first = current transfered byte, second = byte to transfer
		virtual quint64 realByteTransfered() = 0;					//real size transfered to right speed calculation
		//edit the transfer list
		virtual QList<returnActionOnCopyList> getActionOnList() = 0;
		//speed limitation
		virtual qint64 getSpeedLimitation() = 0;///< -1 if not able, 0 if disabled
		//get collision action
		virtual QList<QPair<QString,QString> > getCollisionAction() = 0;
		virtual QList<QPair<QString,QString> > getErrorAction() = 0;
		//transfer list
		virtual QList<ItemOfCopyList> getTransferList() = 0;
		virtual ItemOfCopyList getTransferListEntry(quint64 id) = 0;
	public slots:
		//user ask ask to add folder (add it with interface ask source/destination)
		virtual bool userAddFolder(CopyMode mode) = 0;
		virtual bool userAddFile(CopyMode mode) = 0;
		//action on the copy
		virtual void pause() = 0;
		virtual void resume() = 0;
		virtual void skip(quint64 id) = 0;
		virtual void cancel() = 0;
		//edit the transfer list
		virtual void removeItems(QList<int> ids) = 0;
		virtual void moveItemsOnTop(QList<int> ids) = 0;
		virtual void moveItemsUp(QList<int> ids) = 0;
		virtual void moveItemsDown(QList<int> ids) = 0;
		virtual void moveItemsOnBottom(QList<int> ids) = 0;
		//speed limitation
		virtual bool setSpeedLimitation(qint64 speedLimitation) = 0;///< -1 if not able, 0 if disabled
		//action
		virtual void setCollisionAction(QString action) = 0;
		virtual void setErrorAction(QString action) = 0;
	/* signal to implement
	signals:
		//send information about the copy
		void actionInProgess(EngineActionInProgress engineActionInProgress);	//should update interface information on this event
		
		void newTransferStart(ItemOfCopyList itemOfCopyList);		//should update interface information on this event
		void newTransferStop(quint64 id);		//should update interface information on this event, is stopped, example: because error have occurred, and try later, don't remove the item!
		
		void newFolderListing(QString path);
		void newCollisionAction(QString action);
		void newErrorAction(QString action);
		void isInPause(bool isInPause);
		
		void newActionOnList();
		void cancelAll();
		
		//send error occurred
		void error(QString path,quint64 size,QDateTime mtime,QString error);
		//for the extra logging
		void rmPath(QString path);
		void mkPath(QString path);*/
};

class PluginInterface_CopyEngineFactory : public QObject
{
	Q_OBJECT
	public:
		virtual PluginInterface_CopyEngine * getInstance() = 0;
		//to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface * options,QString writePath,QString pluginPath,FacilityInterface * facilityInterface,bool portableVersion) = 0;
		//get mode allowed
		/// \brief define if can copy file, folder or both
		virtual CopyType getCopyType() = 0;
		virtual bool canDoOnlyCopy() = 0;
		virtual QStringList supportedProtocolsForTheSource() = 0;
		virtual QStringList supportedProtocolsForTheDestination() = 0;
		virtual QWidget * options() = 0;
	public slots:
		virtual void resetOptions() = 0;
		virtual void newLanguageLoaded() = 0;

};

Q_DECLARE_INTERFACE(PluginInterface_CopyEngineFactory,"first-world.info.ultracopier.PluginInterface.CopyEngineFactory/0.3.0.2");

#endif // PLUGININTERFACE_COPYENGINE_H
