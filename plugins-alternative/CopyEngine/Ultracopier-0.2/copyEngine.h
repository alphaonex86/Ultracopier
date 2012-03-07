/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include "../../../interface/PluginInterface_CopyEngine.h"
#include "scanFileOrFolder.h"
#include "readThread.h"
#include "fileErrorDialog.h"
#include "fileExistsDialog.h"
#include "fileIsSameDialog.h"
#include "folderExistsDialog.h"
#include "ui_options.h"
#include "Environment.h"

#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H

namespace Ui {
	class options;
}

/// \brief Define the copy engine
class copyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
	copyEngine();
	~copyEngine();
	/** \brief to send the options panel
	 * \return return false if have not the options
	 * \param tempWidget the widget to generate on it the options */
	bool getOptionsEngine(QWidget * tempWidget);
	/** \brief to have interface widget to do modal dialog
	 * \param interface to have the widget of the interface, useful for modal dialog */
	void setInterfacePointer(QWidget * interface);
	//return empty if multiple
	/** \brief compare the current sources of the copy, with the passed arguments
	 * \param sources the sources list to compares with the current sources list
	 * \return true if have same sources, else false (or empty) */
	bool haveSameSource(const QStringList &sources);
	/** \brief compare the current destination of the copy, with the passed arguments
	 * \param destination the destination to compares with the current destination
	 * \return true if have same destination, else false (or empty) */
	bool haveSameDestination(const QString &destination);
	//external soft like file browser have send copy/move list to do
	/** \brief send copy without destination, ask the destination
	 * \param sources the sources list to copy
	 * \return true if the copy have been accepted */
	bool newCopy(const QStringList &sources);
	/** \brief send copy with destination
	 * \param sources the sources list to copy
	 * \param destination the destination to copy
	 * \return true if the copy have been accepted */
	bool newCopy(const QStringList &sources,const QString &destination);
	/** \brief send move without destination, ask the destination
	 * \param sources the sources list to move
	 * \return true if the move have been accepted */
	bool newMove(const QStringList &sources);
	/** \brief send move without destination, ask the destination
	 * \param sources the sources list to move
	 * \param destination the destination to move
	 * \return true if the move have been accepted */
	bool newMove(const QStringList &sources,const QString &destination);
	//get information about the copy
	/** \brief to get the general progression
	 * first = current transfered byte, second = byte to transfer */
	QPair<quint64,quint64> getGeneralProgression();
	/** \brief to get the progression for a specific file
	 * \param id the id of the transfer, id send during population the transfer list
	 * first = current transfered byte, second = byte to transfer */
	returnSpecificFileProgression getFileProgression(const quint64 &id);
	/** \brief to get byte read, use by Ultracopier for the speed calculation
	 * real size transfered to right speed calculation */
	quint64 realByteTransfered();
	//edit the transfer list
	/** \brief to get the action in waiting on the transfer list */
	QList<returnActionOnCopyList> getActionOnList();
	//speed limitation
	/** \brief get the speed limitation
	 * < -1 if not able, 0 if disabled */
	qint64 getSpeedLimitation();
	//get collision action
	/** \brief get the collision action list */
	QList<QPair<QString,QString> > getCollisionAction();
	/** \brief get the collision error list */
	QList<QPair<QString,QString> > getErrorAction();
	//transfer list
	/** \brief to get the transfer list
	 * Used when the interface is changed, useful to minimize the memory size */
	QList<ItemOfCopyList> getTransferList();
	/** \brief to get one transfer info
	 * Used by the interface which show multiple transfer */
	ItemOfCopyList getTransferListEntry(const quint64 &id);
	
	/** \brief to set drives detected
	 * specific to this copy engine */
	void setDrive(const QStringList &drives);
public slots:
	/** \brief add folder called on the interface
	 * Used by manual adding */
	bool userAddFolder(const CopyMode &mode);
	/** \brief add file called on the interface
	 * Used by manual adding */
	bool userAddFile(const CopyMode &mode);
	//action on the copy
	/// \brief put the transfer in pause
	void pause();
	/// \brief resume the transfer
	void resume();
	/** \brief skip one transfer entry
	 * \param id id of the file to remove */
	void skip(const quint64 &id);
	/// \brief cancel all the transfer
	void cancel();
	//edit the transfer list
	/** \brief remove the selected item
	 * \param ids ids is the id list of the selected items */
	void removeItems(const QList<int> &ids);
	/** \brief move on top of the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsOnTop(const QList<int> &ids);
	/** \brief move up the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsUp(const QList<int> &ids);
	/** \brief move down the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsDown(const QList<int> &ids);
	/** \brief move on bottom of the list the selected item
	 * \param ids ids is the id list of the selected items */
	void moveItemsOnBottom(const QList<int> &ids);
	/// \brief export the transfer list into a file
	void exportTransferList();
	/// \brief import the transfer list into a file
	void importTransferList();
	/** \brief to set the speed limitation
	 * -1 if not able, 0 if disabled */
	bool setSpeedLimitation(const qint64 &speedLimitation);
	//action
	/// \brief to set the collision action
	void setCollisionAction(const QString &action);
	/// \brief to set the error action
	void setErrorAction(const QString &action);
	
	/// \brief set the copy info and options before running
	void setRightTransfer(const bool doRightTransfer);
	/// \brief set keep date
	void setKeepDate(const bool keepDate);
	/// \brief set the current max speed in KB/s
	void setMaxSpeed(int maxSpeed);
	/// \brief set preallocate file size
	void setPreallocateFileSize(const bool prealloc);
	/// \brief set block size in KB
	void setBlockSize(const int blockSize);
	/// \brief set auto start
	void setAutoStart(const bool autoStart);
	/// \brief set if need check the destination folder exists
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
	/// \brief set the translate
	void newLanguageLoaded();
	/// \brief reset widget
	void resetTempWidget();
	//other
	void on_comboBoxFolderColision_currentIndexChanged(int index);
	void on_comboBoxFolderError_currentIndexChanged(int index);
private:
	QWidget *tempWidget;
	Ui::options *ui;
	QString sourceDrive;
	bool sourceDriveMultiple;
	QString destinationDrive;
	bool destinationDriveMultiple;
	struct scanFileOrFolderThread
	{
		scanFileOrFolder * thread;
		CopyMode mode;
	};
	QList<scanFileOrFolderThread> scanFileOrFolderThreadsPool;
	scanFileOrFolder * newScanThread(CopyMode mode);
	QWidget *interface;
	quint64 bytesToTransfer;
	quint64 bytesTransfered;
	readThread threadOfTheTransfer;
	bool transferIsStarted_internalVar;
	FileExistsAction alwaysDoThisActionForFileExists;
	FolderExistsAction alwaysDoThisActionForFolderExists;
	FileErrorAction alwaysDoThisActionForFileError;
	FileErrorAction alwaysDoThisActionForFolderError;
	bool uiIsInstalled;
	int maxSpeed;
	bool doRightTransfer;
	bool keepDate;
	bool prealloc;
	int blockSize;
	bool autoStart;
	bool checkDestinationFolderExists;
	bool stopIt;
	fileIsSameDialog fileIsSameDialogObject;
	fileExistsDialog fileExistsDialogObject;
	fileErrorDialog fileErrorDialogObject;
	folderExistsDialog folderExistsDialogObject;
private slots:
	void scanThreadHaveFinish(bool skipFirstRemove=false);
	void updateTheStatus();
	void transferIsStarted(bool);
	void folderTransfer(QString source,QString destination,int numberOfItem);
	void fileTransfer(QFileInfo sourceFileInfo,QFileInfo destinationFileInfo);
	//dialog message
	void fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame);
	void errorOnFile(QFileInfo fileInfo,QString errorString,bool canPutAtTheEnd);
        void errorOnFileInWriteThread(QFileInfo fileInfo,QString errorString,int indexOfWriteThread);
	void folderAlreadyExists(QFileInfo source,bool isSame,QFileInfo destination);
	void errorOnFolder(QFileInfo fileInfo,QString errorString);
	void setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox=true);
	void setComboBoxFolderError(FileErrorAction action,bool changeComboBox=true);
signals:
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
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
	void mkPath(QString path);
};

#endif // COPY_ENGINE_H
