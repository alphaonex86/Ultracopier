/** \file fileExistsDialog.h
\brief Define the dialog when file already exists
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include "Environment.h"

#ifndef FILEEXISTSDIALOG_H
#define FILEEXISTSDIALOG_H

namespace Ui {
	class fileExistsDialog;
}

/// \brief to show file exists dialog, and ask what do
class fileExistsDialog : public QDialog
{
	Q_OBJECT
public:
	/// \brief create the object and pass all the informations to it
	explicit fileExistsDialog(QWidget *parent,QFileInfo source,QFileInfo destination,QString firstRenamingRule,QString otherRenamingRule);
	~fileExistsDialog();
	/// \brief return the the always checkbox is checked
	bool getAlways();
	/// \brief return the action clicked
	FileExistsAction getAction();
	/// \brief return the new rename is case in manual renaming
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
	void updateRenameButton();
	void on_checkBoxAlways_toggled(bool checked);
	void on_lineEditNewName_textChanged(const QString &arg1);
private:
	Ui::fileExistsDialog *ui;
	FileExistsAction action;
	QString oldName;
	QFileInfo destinationInfo;
	QString firstRenamingRule;
	QString otherRenamingRule;
};

#endif // FILEEXISTSDIALOG_H
