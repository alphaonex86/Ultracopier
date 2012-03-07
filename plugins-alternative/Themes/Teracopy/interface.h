/** \file interface.h
\brief Define the interface test
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef INTERFACE_TEST_H
#define INTERFACE_TEST_H

#include <QObject>
#include <QWidget>
#include <QCloseEvent>
#include <QMenu>

#include "../../../interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "Environment.h"

namespace Ui {
	class interfaceCopy;
}

/// \brief Define the interface
class InterfacePlugin : public PluginInterface_Themes
{
	Q_OBJECT
private:
	struct ItemOfCopyListWithMoreInformations
	{
		quint64 currentProgression;
		ItemOfCopyList generalData;
	};
	struct graphicItem
	{
		quint64 id;
		QTreeWidgetItem * item;
	};
	Ui::interfaceCopy *ui;
	quint64 currentFile;
	quint64 totalFile;
	quint64 currentSize;
	quint64 totalSize;
	void updateOverallInformation();
	void updateCurrentFileInformation();
	EngineActionInProgress action;
	void closeEvent(QCloseEvent *event);
	QList<ItemOfCopyListWithMoreInformations> currentProgressList;
	QList<graphicItem> graphicItemList;
	QString speedString;
	bool storeIsInPause;
	bool modeIsForced;
	CopyType type;
	CopyMode mode;
	void updateModeAndType();
	bool haveStarted;
	QWidget optionEngineWidget;
	void updateTitle();
	QMenu menu;
	FacilityInterface * facilityEngine;
	int loop_size,loop_sub_size,indexAction,index;
public:
	//send information about the copy
	/// \brief to set the action in progress
	void actionInProgess(EngineActionInProgress);
	/// \brief new transfer have started
	void newTransferStart(const ItemOfCopyList &item);
	/** \brief one transfer have been stopped
	 * is stopped, example: because error have occurred, and try later, don't remove the item! */
	void newTransferStop(const quint64 &id);
	/// \brief the new folder is listing
	void newFolderListing(const QString &path);
	/** \brief show the detected speed
	 * in byte per seconds */
	void detectedSpeed(const quint64 &speed);
	/** \brief show the remaining time
	 * time in seconds */
	void remainingTime(const int &remainingSeconds);
	/// \brief set the current collision action
	void newCollisionAction(const QString &action);
	/// \brief set the current error action
	void newErrorAction(const QString &action);
	/// \brief set one error is detected
	void errorDetected();
	//speed limitation
	/** \brief the max speed used
	 * in byte per seconds, -1 if not able, 0 if disabled */
	bool setSpeedLimitation(const qint64 &speedLimitation);
	InterfacePlugin(FacilityInterface * facilityEngine);
	~InterfacePlugin();
	//get information about the copy
	/// \brief show the general progression
	void setGeneralProgression(const quint64 &current,const quint64 &total);
	/// \brief show the file progression
	void setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total);
	/// \brief set collision action
	void setCollisionAction(const QList<QPair<QString,QString> > &);
	/// \brief set error action
	void setErrorAction(const QList<QPair<QString,QString> > &);
	/// \brief set the copyType -> file or folder
	void setCopyType(CopyType);
	/// \brief set the copyMove -> copy or move, to force in copy or move, else support both
	void forceCopyMode(CopyMode);
	/// \brief set if transfer list is exportable/importable
	void setTransferListOperation(TransferListOperation transferListOperation);
	//edit the transfer list
	/// \brief get action on the transfer list (add/move/remove)
	void getActionOnList(const QList<returnActionOnCopyList> &returnActions);
	/** \brief set if the order is external (like file manager copy)
	 * to notify the interface, which can hide add folder/filer button */
	void haveExternalOrder();
	/// \brief set if is in pause
	void isInPause(bool);
	/// \brief get the widget for the copy engine
	QWidget * getOptionsEngineWidget();
	/// \brief to set if the copy engine is found
	void getOptionsEngineEnabled(bool isEnabled);
private slots:
	void on_cancelButton_clicked();
	void on_pauseButton_clicked();
	void on_skipButton_clicked();
	void forcedModeAddFile();
	void forcedModeAddFolder();
	void forcedModeAddFileToCopy();
	void forcedModeAddFolderToCopy();
	void forcedModeAddFileToMove();
	void forcedModeAddFolderToMove();
signals:
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
	//set the transfer list
	void removeItems(QList<int> ids);
	void moveItemsOnTop(QList<int> ids);
	void moveItemsUp(QList<int> ids);
	void moveItemsDown(QList<int> ids);
	void moveItemsOnBottom(QList<int> ids);
	void exportTransferList();
	void importTransferList();
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
	void newSpeedLimitation(qint64);
public slots:
	//set the translate
	void newLanguageLoaded();
};

#endif // INTERFACE_TEST_H
