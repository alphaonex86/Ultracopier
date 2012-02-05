/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QFileDialog>

#include "interface/PluginInterface_CopyEngine.h"
#include "fileErrorDialog.h"
#include "fileExistsDialog.h"
#include "fileIsSameDialog.h"
#include "ui_options.h"
#include "Environment.h"
#include "ListThread.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#include "debugDialog.h"
#include <QTimer>
#endif

#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H

namespace Ui {
	class options;
}

/// \brief the implementation of copy engine plugin, manage directly few stuff, else pass to ListThread class.
class copyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
	copyEngine();
	~copyEngine();
private:
	ListThread *listThread;
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	debugDialog debugDialogWindow;
	#endif
	QWidget *			tempWidget;
	Ui::options *			ui;
	bool				uiIsInstalled;
	QWidget *			interface;
	int				maxSpeed;
	bool				doRightTransfer;
	bool				keepDate;
	int				blockSize;
	bool				autoStart;
	bool				checkDestinationFolderExists;
	FileExistsAction		alwaysDoThisActionForFileExists;
	FileErrorAction			alwaysDoThisActionForFileError;
	FileErrorAction			alwaysDoThisActionForFolderError;
	FolderExistsAction		alwaysDoThisActionForFolderExists;
	bool				dialogIsOpen;
	volatile bool			stopIt;
	//error queue
	struct errorQueueItem
	{
		TransferThread * transfer;	// NULL if send by scan thread
		scanFileOrFolder * scan;	// NULL if send by transfer thread
		bool mkPath;
		bool rmPath;
		QFileInfo inode;
		QString errorString;
	};
	QList<errorQueueItem> errorQueue;
	//already exists queue
	struct alreadyExistsQueueItem
	{
		TransferThread * transfer;	// NULL if send by scan thread
		scanFileOrFolder * scan;	// NULL if send by transfer thread
		QFileInfo source;
		QFileInfo destination;
		bool isSame;
	};
	QList<alreadyExistsQueueItem> alreadyExistsQueue;
	//temp variable
	int error_index,loop_size;
	FileErrorAction tempFileErrorAction;
	FolderExistsAction tempFolderExistsAction;
	FileExistsAction tempFileExistsAction;
private slots:
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	void updateTheDebugInfo(QStringList,QStringList,int);
	#endif
	//dialog message
	/// \note Can be call without queue because all call will be serialized
	void fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog=false);
	/// \note Can be call without queue because all call will be serialized
	void errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread,bool isCalledByShowOneNewDialog=false);
	/// \note Can be call without queue because all call will be serialized
	void folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
	/// \note Can be call without queue because all call will be serialized
	void errorOnFolder(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
	//mkpath event
	void mkPathErrorOnFolder(QFileInfo,QString,bool isCalledByShowOneNewDialog=false);
	//rmpath event
	void rmPathErrorOnFolder(QFileInfo,QString,bool isCalledByShowOneNewDialog=false);
	//show one new dialog if needed
	void showOneNewDialog();
public:
	bool getOptionsEngine(QWidget * tempWidget);				//to send the options panel
	void setInterfacePointer(QWidget * interface);		//to have interface widget to do modal dialog
	//return empty if multiple
	bool haveSameSource(QStringList sources);
	bool haveSameDestination(QString destination);
	//external soft like file browser have send copy/move list to do
	bool newCopy(QStringList sources);
	bool newCopy(QStringList sources,QString destination);
	bool newMove(QStringList sources);
	bool newMove(QStringList sources,QString destination);
	//get information about the copy
	QPair<quint64,quint64> getGeneralProgression();			//first = current transfered byte, second = byte to transfer
	returnSpecificFileProgression getFileProgression(quint64 id);	//first = current transfered byte, second = byte to transfer
	quint64 realByteTransfered();					//real size transfered to right speed calculation
	//edit the transfer list
	QList<returnActionOnCopyList> getActionOnList();
	//speed limitation
	qint64 getSpeedLimitation();///< -1 if not able, 0 if disabled
	//get collision action
	QList<QPair<QString,QString> > getCollisionAction();
	QList<QPair<QString,QString> > getErrorAction();
	//transfer list
	QList<ItemOfCopyList> getTransferList();
	ItemOfCopyList getTransferListEntry(quint64 id);
	void setDrive(QStringList drives);
public slots:
	//user ask ask to add folder (add it with interface ask source/destination)
	bool userAddFolder(CopyMode mode);
	bool userAddFile(CopyMode mode);
	//action on the copy
	void pause();
	void resume();
	void skip(quint64 id);
	void cancel();
	//edit the transfer list
	void removeItems(QList<int> ids);
	void moveItemsOnTop(QList<int> ids);
	void moveItemsUp(QList<int> ids);
	void moveItemsDown(QList<int> ids);
	void moveItemsOnBottom(QList<int> ids);
	//speed limitation
	bool setSpeedLimitation(qint64 speedLimitation);///< -1 if not able, 0 if disabled
	//action
	void setCollisionAction(QString action);
	void setErrorAction(QString action);
	//set the copy info and options before runing
	void setRightTransfer(const bool doRightTransfer);
	//set keep date
	void setKeepDate(const bool keepDate);
	//set block size in KB
	void setBlockSize(const int blockSize);
	//set auto start
	void setAutoStart(const bool autoStart);
	//set check destination folder
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
	//reset widget
	void resetTempWidget();
	//autoconnect
	void on_comboBoxFolderColision_currentIndexChanged(int index);
	void on_comboBoxFolderError_currentIndexChanged(int index);
	//set the translate
	void newLanguageLoaded();
private slots:
	void setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox=true);
	void setComboBoxFolderError(FileErrorAction action,bool changeComboBox=true);
signals:
	//send information about the copy
	void actionInProgess(EngineActionInProgress);	//should update interface information on this event

	void newTransferStart(ItemOfCopyList);		//should update interface information on this event
	void newTransferStop(quint64 id);		//should update interface information on this event, is stopped, example: because error have occurred, and try later, don't remove the item!

	void newFolderListing(QString path);
	void newCollisionAction(QString action);
	void newErrorAction(QString action);
	void isInPause(bool);

	void newActionOnList();
	void cancelAll();

	//send error occurred
	void error(QString path,quint64 size,QDateTime mtime,QString error);
	//for the extra logging
	void rmPath(QString path);
	void mkPath(QString path);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif

	//other signals
	void queryOneNewDialog();
};

#endif // COPY_ENGINE_H
