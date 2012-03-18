/** \file scanFileOrFolder.h
\brief Thread changed to list recursively the folder
\author alpha_one_x86
\version 0.3
\date 2011 */

#include <QThread>
#include <QStringList>
#include <QString>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include <QSemaphore>

#include "Environment.h"

#ifndef SCANFILEORFOLDER_H
#define SCANFILEORFOLDER_H

/// \brief Thread changed to list recursively the folder
class scanFileOrFolder : public QThread
{
	Q_OBJECT
public:
	explicit scanFileOrFolder(CopyMode mode);
	~scanFileOrFolder();
	/// \brief to the a folder listing
	void stop();
	/// \brief to get if is finished
	bool isFinished();
	/// \brief set action if Folder are same or exists
	void setFolderExistsAction(FolderExistsAction action,QString newName="");
	/// \brief set action if error
	void setFolderErrorAction(FileErrorAction action);
	/// \brief set if need check if the destination exists
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
signals:
	void fileTransfer(const QFileInfo &source,const QFileInfo &destination,const CopyMode &mode);
	/// \brief To debug source
	void debugInformation(const DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
	void folderAlreadyExists(const QFileInfo &source,const QFileInfo &destination,const bool &isSame);
	void errorOnFolder(const QFileInfo &fileInfo,const QString &errorString);
	void finishedTheListing();

	void newFolderListing(const QString &path);
	void addToMkPath(const QString& folder);
	void addToRmPath(const QString& folder,const int& inodeToRemove);
public slots:
	void addToList(const QStringList& sources,const QString& destination);
protected:
	void run();
private:
	QStringList		sources;
	QString			destination;
	volatile bool		stopIt;
        void			listFolder(const QString& source,const QString& destination,const QString& sourceSuffixPath,QString destinationSuffixPath);
	volatile bool		stopped;
	QSemaphore		waitOneAction;
	FolderExistsAction	folderExistsAction;
	FileErrorAction		fileErrorAction;
	volatile bool		checkDestinationExists;
	QString			newName;
	QRegExp			folder_isolation;
	QString			prefix;
	QString			suffix;
	CopyMode		mode;
};

#endif // SCANFILEORFOLDER_H
