#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
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
	explicit fileIsSameDialog(QWidget *parent,QFileInfo fileInfo);
	~fileIsSameDialog();
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
	void updateRenameButton();
	void on_lineEditNewName_textChanged(const QString &arg1);
	void on_checkBoxAlways_toggled(bool checked);
private:
	Ui::fileIsSameDialog *ui;
	FileExistsAction action;
	QString oldName;
	QFileInfo destinationInfo;
};

#endif // FILEISSAMEDIALOG_H
