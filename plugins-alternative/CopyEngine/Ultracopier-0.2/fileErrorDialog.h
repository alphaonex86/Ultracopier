/** \file fileErrorDialog.h
\brief Define the dialog error on the file
\author alpha_one_x86
\version 0.3
\date 2010 */

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
	explicit fileErrorDialog();
	~fileErrorDialog();
	/// \brief set the file information
	void setInfo(QFileInfo fileInfo,QString errorString,bool canPutAtTheEnd);
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
