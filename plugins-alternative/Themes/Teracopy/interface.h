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

#include "interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "Environment.h"

namespace Ui {
	class interfaceCopy;
}

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
	void actionInProgess(EngineActionInProgress);
	void newTransferStart(const ItemOfCopyList &item);
	void newTransferStop(const quint64 &id);
	void newFolderListing(const QString &path);
	void detectedSpeed(quint64 speed);//in byte per seconds
	void remainingTime(int remainingSeconds);//can't be more than 23h59m59s, if -1 is infinity
	void newCollisionAction(QString action);
	void newErrorAction(QString action);
	void errorDetected();
	//speed limitation
	bool setSpeedLimitation(qint64 speedLimitation);///< -1 if not able, 0 if disabled
	void speed(qint64);
	InterfacePlugin(FacilityInterface * facilityEngine);
	~InterfacePlugin();
	//get information about the copy
	void setGeneralProgression(quint64 current,quint64 total);
	void setFileProgression(quint64 id,quint64 current,quint64 total);
	void setCollisionAction(QList<QPair<QString,QString> >);
	void setErrorAction(QList<QPair<QString,QString> >);
	void setCopyType(CopyType);
	void forceCopyMode(CopyMode);
	//edit the transfer list
	void getActionOnList(QList<returnActionOnCopyList> returnActions);
	void haveExternalOrder();
	void isInPause(bool);
	QWidget * getOptionsEngineWidget();
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
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
public slots:
	//set the translate
	void newLanguageLoaded();
};

#endif // INTERFACE_TEST_H
