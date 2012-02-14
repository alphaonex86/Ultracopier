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

#include "interface/OptionInterface.h"
#include "interface/FacilityInterface.h"
#include "../StructEnumDefinition.h"

class PluginInterface_Themes : public QWidget
{
	Q_OBJECT
	public slots:
		//send information about the copy
		virtual void actionInProgess(EngineActionInProgress) = 0;
		virtual void newTransferStart(const ItemOfCopyList &item) = 0;
		virtual void newTransferStop(const quint64 &id) = 0;//is stopped, example: because error have occurred, and try later, don't remove the item!
		virtual void newFolderListing(const QString &path) = 0;
		virtual void detectedSpeed(quint64 speed) = 0;//in byte per seconds
		virtual void speed(qint64) = 0;
		virtual void remainingTime(int remainingSeconds) = 0;
		virtual void newCollisionAction(QString action) = 0;
		virtual void newErrorAction(QString action) = 0;
		virtual void errorDetected() = 0;
		//speed limitation
		virtual bool setSpeedLimitation(qint64 speedLimitation) = 0;///< -1 if not able, 0 if disabled
	public:
		virtual QWidget * getOptionsEngineWidget() = 0;
		virtual void getOptionsEngineEnabled(bool isEnabled) = 0;
		//edit the transfer list
		virtual void getActionOnList(QList<returnActionOnCopyList>) = 0;
		//get information about the copy
		virtual void setGeneralProgression(quint64 current,quint64 total) = 0;
		virtual void setFileProgression(quint64 id,quint64 current,quint64 total) = 0;
		virtual void setCollisionAction(QList<QPair<QString,QString> >) = 0;
		virtual void setErrorAction(QList<QPair<QString,QString> >) = 0;
		virtual void setCopyType(CopyType) = 0;
		virtual void forceCopyMode(CopyMode) = 0;//to force in copy or move, else support both
		virtual void haveExternalOrder() = 0;//to notify the interface, which can hide add folder/filer button
		virtual void isInPause(bool) = 0;
	/* signal to implement
	signals:
		//set the transfer list
		void removeItems(QList<int> ids);
		void moveItemsOnTop(QList<int> ids);
		void moveItemsUp(QList<int> ids);
		void moveItemsDown(QList<int> ids);
		void moveItemsOnBottom(QList<int> ids);
		//user ask ask to add folder (add it with interface ask source/destination)
		void userAddFolder(CopyMode);
		void userAddFile(CopyMode);
		void urlDropped(QList<QUrl> urls);
		//action on the copy
		void pause();
		void resume();
		void skip(quint64 id);
		void cancel();
		//edit the action
		void sendCollisionAction(QString action);
		void sendErrorAction(QString action);
		void newSpeedLimitation(qint64);///< -1 if not able, 0 if disabled*/
};

class PluginInterface_ThemesFactory : public QObject
{
	Q_OBJECT
	public:
		virtual PluginInterface_Themes * getInstance() = 0;
		//to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface *,QString writePath,QString pluginPath,FacilityInterface * facilityInterface,bool portableVersion) = 0;
		virtual QWidget * options() = 0;
	public slots:
		virtual void resetOptions() = 0;
		virtual void newLanguageLoaded() = 0;
};

Q_DECLARE_INTERFACE(PluginInterface_ThemesFactory,"first-world.info.ultracopier.PluginInterface.ThemesFactory/0.3.0.2");

#endif // PLUGININTERFACE_THEMES_H
