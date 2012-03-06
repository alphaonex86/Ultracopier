/** \file MkPath.h
\brief Make the path given as queued mkpath
\author alpha_one_x86
\version 0.3
\date 2011 */

#ifndef MKPATH_H
#define MKPATH_H

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QSemaphore>
#include <QStringList>
#include <QDir>

/// \brief Make the path given as queued mkpath
class MkPath : public QThread
{
	Q_OBJECT
public:
	explicit MkPath();
	~MkPath();
	/// \brief add path to make
	void addPath(QString path);
signals:
	void errorOnFolder(QFileInfo,QString);
	void firstFolderFinish();
	void internalStartAddPath(QString path);
	void internalStartDoThisPath();
	void internalStartSkip();
	void internalStartRetry();
public slots:
	/// \brief skip after creation error
	void skip();
	/// \brief retry after creation error
	void retry();
private:
	void run();
	bool waitAction;
	bool stopIt;
	bool skipIt;
	QStringList pathList;
	void checkIfCanDoTheNext();
	QDir dir;
private slots:
	void internalDoThisPath();
	void internalAddPath(QString path);
	void internalSkip();
	void internalRetry();
};

#endif // MKPATH_H
