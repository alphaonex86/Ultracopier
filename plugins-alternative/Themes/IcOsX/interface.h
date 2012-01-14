/** \file interface.h
\brief Define the interface
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef INTERFACE_TEST_H
#define INTERFACE_TEST_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>

#include "interface/PluginInterface_Themes.h"

namespace Ui {
	class interface;
}

class InterfacePlugin : public PluginInterface_Themes
{
	Q_OBJECT
public slots:
	//send information about the copy
	void actionInProgess(EngineActionInProgress);
	void newTransferStart(const ItemOfCopyList &item);
	void newTransferStop(const quint64 &id);//is stopped, example: because error have occurred, and try later, don't remove the item!
	void newFolderListing(const QString &path);
	void detectedSpeed(quint64 speed);//in byte per seconds
	void speed(qint64);
	void remainingTime(int remainingSeconds);
	void newCollisionAction(QString action);
	void newErrorAction(QString action);
	void errorDetected();
	//speed limitation
	bool setSpeedLimitation(qint64 speedLimitation);///< -1 if not able, 0 if disabled
	//set the translate
	void newLanguageLoaded();
public:
	QWidget * getOptionsEngineWidget();
	void getOptionsEngineEnabled(bool isEnabled);
	//edit the transfer list
	void getActionOnList(QList<returnActionOnCopyList>);
	//get information about the copy
	void setGeneralProgression(quint64 current,quint64 total);
	void setFileProgression(quint64 id,quint64 current,quint64 total);
	void setCollisionAction(QList<QPair<QString,QString> >);
	void setErrorAction(QList<QPair<QString,QString> >);
	void setCopyType(CopyType);
	void forceCopyMode(CopyMode);//to force in copy or move, else support both
	void haveExternalOrder();//to notify the interface, which can hide add folder/filer button
	void isInPause(bool);
signals:
	//set the transfer list
	void setActionOnList(ActionOnCopyList action);
	//user ask ask to add folder (add it with interface ask source/destination)
	void userAddFolder(CopyMode);
	void userAddFile(CopyMode);
	//action on the copy
	void pause();
	void resume();
	void skip(quint64 id);
	void cancel();
	//edit the action
	void setCollisionAction(QString action);
	void setErrorAction(QString action);
	void newSpeedLimitation(qint64);///< -1 if not able, 0 if disabled
public:
	//constructor and destructor
	InterfacePlugin();
	~InterfacePlugin();
private:
	Ui::interface *ui;
	quint64 currentFile;
	quint64 totalFile;
	quint64 currentSize;
	quint64 totalSize;
	void updateTitle();
	QMenu *menu;
	EngineActionInProgress action;
	void closeEvent(QCloseEvent *event);
	void updateModeAndType();
	bool modeIsForced;
	CopyType type;
	CopyMode mode;
	bool haveStarted;
private slots:
	void forcedModeAddFile();
	void forcedModeAddFolder();
	void forcedModeAddFileToCopy();
	void forcedModeAddFolderToCopy();
	void forcedModeAddFileToMove();
	void forcedModeAddFolderToMove();
};

#endif // INTERFACE_TEST_H
