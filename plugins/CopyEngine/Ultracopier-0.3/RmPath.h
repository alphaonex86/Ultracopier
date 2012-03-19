/** \file RmPath.h
\brief Remove the path given as queued rmpath
\author alpha_one_x86
\version 0.3
\date 2011 */

#ifndef RMPATH_H
#define RMPATH_H

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QSemaphore>
#include <QStringList>
#include <QDir>

#include "Environment.h"

/// \brief Remove the path given as queued rmpath
class RmPath : public QThread
{
	Q_OBJECT
public:
	explicit RmPath();
	~RmPath();
	/// \brief add new path to remove
	void addPath(const QString &path);
signals:
	void errorOnFolder(const QFileInfo &,const QString &);
	void firstFolderFinish();
	void internalStartAddPath(const QString &path);
	void internalStartDoThisPath();
	void internalStartSkip();
	void internalStartRetry();
	void debugInformation(const DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
public slots:
	void skip();
	void retry();
private:
	void run();
	bool waitAction;
	bool stopIt;
	bool skipIt;
	QStringList pathList;
	void checkIfCanDoTheNext();
	QDir dir;
	bool rmpath(const QDir &dir);
private slots:
	void internalDoThisPath();
	void internalAddPath(const QString &path);
	void internalSkip();
	void internalRetry();
};


#endif // RMPATH_H
