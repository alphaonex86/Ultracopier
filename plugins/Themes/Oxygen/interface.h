/** \file interface.h
\brief Define the interface test
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef INTERFACE_TEST_H
#define INTERFACE_TEST_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>
#include <QShortcut>

#include "../../../interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "ui_options.h"
#include "Environment.h"

// for windows progress bar
#ifndef __GNUC__
#    include <shobjidl.h>
#endif

namespace Ui {
	class interfaceCopy;
}

class InterfacePlugin : public PluginInterface_Themes
{
	Q_OBJECT
public:
	InterfacePlugin(bool checkBoxShowSpeed,FacilityInterface * facilityEngine);
	~InterfacePlugin();
	//send information about the copy
	void actionInProgess(EngineActionInProgress);
	void newTransferStart(const ItemOfCopyList &item);
	void newTransferStop(const quint64 &id);//is stopped, example: because error have occurred, and try later, don't remove the item!
	void newFolderListing(const QString &path);
	void detectedSpeed(const quint64 &speed);//in byte per seconds
	void remainingTime(const int &remainingSeconds);
	void newCollisionAction(const QString &action);
	void newErrorAction(const QString &action);
	void errorDetected();
	//speed limitation
	bool setSpeedLimitation(const qint64 &speedLimitation);///< -1 if not able, 0 if disabled
	//get information about the copy
	void setGeneralProgression(const quint64 &current,const quint64 &total);
	void setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total);
	void setCollisionAction(const QList<QPair<QString,QString> > &);
	void setErrorAction(const QList<QPair<QString,QString> > &);
	void setCopyType(CopyType);
	void forceCopyMode(CopyMode);
	void setTransferListOperation(TransferListOperation transferListOperation);
	//edit the transfer list
	void getActionOnList(const QList<returnActionOnCopyList> &returnActions);
	void haveExternalOrder();
	void isInPause(bool);
	QWidget * getOptionsEngineWidget();
	void getOptionsEngineEnabled(bool isEnabled);
public slots:
	//set the translate
	void newLanguageLoaded();
private slots:
	void on_putOnTop_clicked();
	void on_pushUp_clicked();
	void on_pushDown_clicked();
	void on_putOnBottom_clicked();
	void on_del_clicked();
	void on_cancelButton_clicked();
	void on_checkBoxShowSpeed_toggled(bool checked);
	void on_SliderSpeed_valueChanged(int value);
	void on_limitSpeed_valueChanged(int );
	void on_checkBox_limitSpeed_clicked();
	void on_pauseButton_clicked();
	void on_skipButton_clicked();
	void forcedModeAddFile();
	void forcedModeAddFolder();
	void forcedModeAddFileToCopy();
	void forcedModeAddFolderToCopy();
	void forcedModeAddFileToMove();
	void forcedModeAddFolderToMove();
	void uiUpdateSpeed();
	void on_pushButtonCloseSearch_clicked();
	//close the search box
	void closeTheSearchBox();
	//search box shortcut
	void searchBoxShortcut();
	//hilight the search
	void hilightTheSearch();
	//auto connect
	void on_pushButtonSearchPrev_clicked();
	void on_pushButtonSearchNext_clicked();
	void on_lineEditSearch_returnPressed();
	void on_lineEditSearch_textChanged(QString text);
	void on_moreButton_toggled(bool checked);
	void on_comboBox_copyErrors_currentIndexChanged(int index);
	void on_comboBox_fileCollisions_currentIndexChanged(int index);
	void on_searchButton_toggled(bool checked);
	void on_exportTransferList_clicked();
	void on_importTransferList_clicked();
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
	QMenu *menu;
	EngineActionInProgress action;
	void closeEvent(QCloseEvent *event);
	QList<ItemOfCopyListWithMoreInformations> currentProgressList;
	QList<graphicItem> graphicItemList;
	qint64 currentSpeed;
	void updateSpeed();
	bool storeIsInPause;
	bool modeIsForced;
	CopyType type;
	CopyMode mode;
	void updateModeAndType();
	bool haveStarted;
	bool haveError;
	QWidget optionEngineWidget;
	QShortcut *searchShortcut;
	QShortcut *searchShortcut2;
	QShortcut *searchShortcut3;
	QTimer *TimerForSearch;
	int currentIndexSearch;		///< Current index search in starting at the end
	FacilityInterface * facilityEngine;
	QIcon player_play,player_pause;
	//temp variables
	int loop_size,loop_sub_size,index,indexAction;
	QList<QTreeWidgetItem *> selectedItems;
	QList<int> ids;
	/** \brief drag event processing

	need setAcceptDrops(true); into the constructor
	need implementation to accept the drop:
	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dragLeaveEvent(QDragLeaveEvent* event);
	*/
	void dropEvent(QDropEvent *event);
	/** \brief accept all event to allow the drag and drop
	  \see dropEvent() */
	void dragEnterEvent(QDragEnterEvent* event);
	/** \brief accept all event to allow the drag and drop
	  \see dropEvent() */
	void dragMoveEvent(QDragMoveEvent* event);
	/** \brief accept all event to allow the drag and drop
	  \see dropEvent() */
	void dragLeaveEvent(QDragLeaveEvent* event);
	//have functionality
	bool shutdown;
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
};

#endif // INTERFACE_TEST_H
