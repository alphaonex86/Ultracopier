/** \file folderExistsDialog.h
\brief Define the dialog when file exists
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FOLDERISSAMEDIALOG_H
#define FOLDERISSAMEDIALOG_H

#include <QDialog>
#include <QFileInfo>
#include <QString>
#include <QDateTime>

#include "Environment.h"

namespace Ui {
    class folderExistsDialog;
}

/// \brief Define the dialog when file exists
class folderExistsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit folderExistsDialog();
	~folderExistsDialog();
	/// \brief set file information
	void setInfo(QFileInfo source,bool isSame,QFileInfo destination);
	/// \brief return the the always checkbox is checked
	bool getAlways();
	/// \brief return the action clicked
	FolderExistsAction getAction();
	/// \brief get the new name
	QString getNewName();
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_SuggestNewName_clicked();
	void on_Rename_clicked();
	void on_Skip_clicked();
	void on_Cancel_clicked();
	void on_Merge_clicked();
private:
	Ui::folderExistsDialog *ui;
	FolderExistsAction action;
	QString oldName;
};

#endif // FOLDERISSAMEDIALOG_H
