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

class scanFileOrFolder : public QThread
{
	Q_OBJECT
public:
	explicit scanFileOrFolder();
	~scanFileOrFolder();
	void stop();
	bool isFinished();
	//set action if Folder are same or exists
	void setFolderExistsAction(FolderExistsAction action,QString newName="");
	//set action if error
	void setFolderErrorAction(FileErrorAction action);
	//set if need check if the destination exists
	void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
signals:
	void folderTransfer(QString source,QString destination,int numberOfItem);
	void fileTransfer(QFileInfo source,QFileInfo destination);
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	void folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame);
	void errorOnFolder(QFileInfo fileInfo,QString errorString);
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
};

#endif // SCANFILEORFOLDER_H
