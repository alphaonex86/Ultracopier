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

class folderExistsDialog : public QDialog
{
    Q_OBJECT

public:
	explicit folderExistsDialog();
	~folderExistsDialog();
	void setInfo(QFileInfo source,bool isSame,QFileInfo destination);
	bool getAlways();
	FolderExistsAction getAction();
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
