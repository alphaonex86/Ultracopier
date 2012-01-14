#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include "Environment.h"

#ifndef FILEEXISTSDIALOG_H
#define FILEEXISTSDIALOG_H

namespace Ui {
	class fileExistsDialog;
}

class fileExistsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit fileExistsDialog();
	~fileExistsDialog();
	void setInfo(QFileInfo source,QFileInfo destination);
	bool getAlways();
	FileExistsAction getAction();
	QString getNewName();
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_SuggestNewName_clicked();
	void on_Rename_clicked();
	void on_Overwrite_clicked();
	void on_Skip_clicked();
	void on_Cancel_clicked();
	void on_actionOverwrite_if_newer_triggered();
	void on_actionOverwrite_if_not_same_modification_date_triggered();
private:
	Ui::fileExistsDialog *ui;
	FileExistsAction action;
	QString oldName;
};

#endif // FILEEXISTSDIALOG_H
