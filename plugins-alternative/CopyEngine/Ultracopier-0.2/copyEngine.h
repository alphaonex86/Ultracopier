/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include "interface/PluginInterface_CopyEngine.h"
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

class copyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
	copyEngine();
	~copyEngine();
	//to get engine custom panel
	bool getOptionsEngine(QWidget *tempWidget);
	void setInterfacePointer(QWidget * interface);
	//duplication copy detection
	bool haveSameSource(QStringList sources);
	bool haveSameDestination(QString destination);
	//external soft like file browser have send copy/move list to do
	bool newCopy(QStringList sources);
	bool newCopy(QStringList sources,QString destination);
	bool newMove(QStringList sources);
	bool newMove(QStringList sources,QString destination);
	//get information about the copy
	QPair<quint64,quint64> getGeneralProgression();
	returnSpecificFileProgression getFileProgression(quint64 id);
	QList<returnActionOnCopyList> getActionOnList();
	quint64 realByteTransfered();		//real size transfered to right speed calculation
	//speed limitation
	qint64 getSpeedLimitation();
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
	void start();
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
	bool setSpeedLimitation(qint64 speedLimitation);
	//get collision action
	void setCollisionAction(QString action);
	void setErrorAction(QString action);
	//set the copy info and options before running
	void setRightTransfer(const bool doRightTransfer);
	//set keep date
	void setKeepDate(const bool keepDate);
	//set the current max speed in KB/s
	void setMaxSpeed(int maxSpeed);
	//set preallocate file size
	void setPreallocateFileSize(const bool prealloc);
	//set block size in KB
	void setBlockSize(const int blockSize);
	//set auto start
	void setAutoStart(const bool autoStart);
	//set if need check the destination folder exists
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
	//set the translate
	void newLanguageLoaded();
	//reset widget
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
};

#endif // COPY_ENGINE_H
