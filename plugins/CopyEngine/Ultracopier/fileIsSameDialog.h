/** \file fileIsSameDialog.h
\brief Define the dialog when file is same
\author alpha_one_x86
\licence GPL3, see the file COPYING */

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

/// \brief to show file is same dialog, and ask what do
class fileIsSameDialog : public QDialog
{
	Q_OBJECT
public:
	/// \brief create the object and pass all the informations to it
	explicit fileIsSameDialog(QWidget *parent,QFileInfo fileInfo,QString firstRenamingRule,QString otherRenamingRule);
	~fileIsSameDialog();
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
	QString firstRenamingRule;
	QString otherRenamingRule;

};

#endif // FILEISSAMEDIALOG_H
