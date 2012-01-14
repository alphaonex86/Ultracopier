#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include "Environment.h"

#ifndef FILEISSAMEDIALOG_H
#define FILEISSAMEDIALOG_H

namespace Ui {
	class fileIsSameDialog;
}

class fileIsSameDialog : public QDialog
{
	Q_OBJECT
public:
	explicit fileIsSameDialog();
	~fileIsSameDialog();
	void setInfo(QFileInfo fileInfo);
	bool getAlways();
	FileExistsAction getAction();
	QString getNewName();
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_SuggestNewName_clicked();
	void on_Rename_clicked();
	void on_Skip_clicked();
	void on_Cancel_clicked();
private:
	Ui::fileIsSameDialog *ui;
	FileExistsAction action;
	QString oldName;
};

#endif // FILEISSAMEDIALOG_H
