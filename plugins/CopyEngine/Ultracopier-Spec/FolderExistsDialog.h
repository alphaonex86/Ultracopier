/** \file folderExistsDialog.h
\brief Define the dialog when file exists
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef FOLDERISSAMEDIALOG_H
#define FOLDERISSAMEDIALOG_H

#include <QDialog>
#include <string>

#include "Environment.h"

namespace Ui {
    class folderExistsDialog;
}

/// \brief to show file exists dialog, and ask what do
class FolderExistsDialog : public QDialog
{
    Q_OBJECT

public:
    /// \brief create the object and pass all the informations to it
    explicit FolderExistsDialog(QWidget *parent,std::string source,bool isSame,std::string destination,std::string firstRenamingRule,std::string otherRenamingRule);
    ~FolderExistsDialog();
    /// \brief return the the always checkbox is checked
    bool getAlways();
    /// \brief return the action clicked
    FolderExistsAction getAction();
    /// \brief return the new rename is case in manual renaming
    std::string getNewName();
protected:
    void changeEvent(QEvent *e);
private slots:
    void updateRenameButton();
    void on_SuggestNewName_clicked();
    void on_Rename_clicked();
    void on_Skip_clicked();
    void on_Cancel_clicked();
    void on_Merge_clicked();
    void on_lineEditNewName_editingFinished();
    void on_lineEditNewName_returnPressed();
    void on_lineEditNewName_textChanged(const QString &arg1);
private:
    Ui::folderExistsDialog *ui;
    FolderExistsAction action;
    std::string oldName;
    std::string firstRenamingRule;
    std::string otherRenamingRule;
    std::string destinationInfo;
};

#endif // FOLDERISSAMEDIALOG_H
