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

/// \brief Remove the path given as queued rmpath
class RmPath : public QThread
{
	Q_OBJECT
public:
	explicit RmPath();
	~RmPath();
	/// \brief add new path to remove
	void addPath(QString path);
signals:
	void errorOnFolder(QFileInfo,QString);
	void firstFolderFinish();
	void internalStartAddPath(QString path);
	void internalStartDoThisPath();
	void internalStartSkip();
	void internalStartRetry();
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
private slots:
	void internalDoThisPath();
	void internalAddPath(QString path);
	void internalSkip();
	void internalRetry();
};


#endif // RMPATH_H
