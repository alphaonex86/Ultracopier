#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include "Environment.h"

#ifndef FILEERRORDIALOG_H
#define FILEERRORDIALOG_H

namespace Ui {
	class fileErrorDialog;
}

class fileErrorDialog : public QDialog
{
	Q_OBJECT
public:
	explicit fileErrorDialog();
	~fileErrorDialog();
	void setInfo(QFileInfo fileInfo,QString errorString,bool canPutAtTheEnd);
	bool getAlways();
	FileErrorAction getAction();
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_PutToBottom_clicked();
	void on_Retry_clicked();
	void on_Skip_clicked();
	void on_Cancel_clicked();
private:
	Ui::fileErrorDialog *ui;
	FileErrorAction action;
};

#endif // FILEERRORDIALOG_H
