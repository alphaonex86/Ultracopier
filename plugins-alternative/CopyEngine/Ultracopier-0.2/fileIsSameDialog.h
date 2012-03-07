/** \file fileIsSameDialog.h
\brief Define the dialog when file is same
\author alpha_one_x86
\version 0.3
\date 2010 */

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

/// \brief Define the dialog when file is same
class fileIsSameDialog : public QDialog
{
	Q_OBJECT
public:
	explicit fileIsSameDialog();
	~fileIsSameDialog();
	/// \brief set the file informations
	void setInfo(QFileInfo fileInfo);
	/// \brief return the the always checkbox is checked
	bool getAlways();
	/// \brief return the action clicked
	FileExistsAction getAction();
	/// \brief get the new name
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
