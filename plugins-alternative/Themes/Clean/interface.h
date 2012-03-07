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

#include "../../../interface/PluginInterface_Themes.h"

namespace Ui {
	class interface;
}

/// \brief Define the interface
class InterfacePlugin : public PluginInterface_Themes
{
	Q_OBJECT
public slots:
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
	//set the translate
	void newLanguageLoaded();
public:
	QWidget * getOptionsEngineWidget();
	void getOptionsEngineEnabled(bool isEnabled);
	//edit the transfer list
	void getActionOnList(const QList<returnActionOnCopyList> &);
	//get information about the copy
	void setGeneralProgression(const quint64 &current,const quint64 &total);
	void setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total);
	void setCollisionAction(const QList<QPair<QString,QString> > &);
	void setErrorAction(const QList<QPair<QString,QString> > &);
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
