/** \file fileErrorDialog.h
\brief Define the dialog error on the file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

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

/// \brief to show error dialog, and ask what do
class fileErrorDialog : public QDialog
{
	Q_OBJECT
public:
	/// \brief create the object and pass all the informations to it
	explicit fileErrorDialog(QWidget *parent,QFileInfo fileInfo,QString errorString,bool havePutAtTheEndButton=true);
	~fileErrorDialog();
	/// \brief return the the always checkbox is checked
	bool getAlways();
	/// \brief return the action clicked
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
