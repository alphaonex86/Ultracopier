#ifndef MKPATH_H
#define MKPATH_H

#include <QThread>
#include <QFileInfo>
#include <QString>
#include <QSemaphore>
#include <QStringList>
#include <QDir>

class MkPath : public QThread
{
	Q_OBJECT
public:
	explicit MkPath();
	~MkPath();
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

#endif // MKPATH_H
