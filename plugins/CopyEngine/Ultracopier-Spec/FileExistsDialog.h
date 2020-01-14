/** \file fileExistsDialog.h
\brief Define the dialog when file already exists
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <string>
#include "Environment.h"
#include "../../../interface/FacilityInterface.h"

#ifndef FILEEXISTSDIALOG_H
#define FILEEXISTSDIALOG_H

#ifdef WIDESTRING
#define INTERNALTYPEPATH std::wstring
#else
#define INTERNALTYPEPATH std::string
#endif

namespace Ui {
    class fileExistsDialog;
}

/// \brief to show file exists dialog, and ask what do
class FileExistsDialog : public QDialog
{
    Q_OBJECT
public:
    /// \brief create the object and pass all the informations to it
    explicit FileExistsDialog(QWidget *parent, INTERNALTYPEPATH source, INTERNALTYPEPATH destination, std::string firstRenamingRule, std::string otherRenamingRule, FacilityInterface *facilityEngine);
    ~FileExistsDialog();
    /// \brief return the the always checkbox is checked
    bool getAlways();
    /// \brief return the action clicked
    FileExistsAction getAction();
    /// \brief return the new rename is case in manual renaming
    std::string getNewName();
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
    void on_lineEditNewName_returnPressed();
    void on_actionOverwrite_if_older_triggered();
    void on_lineEditNewName_editingFinished();
    void on_actionOverwrite_if_not_same_size_triggered();
    void on_actionOverwrite_if_not_same_size_and_date_triggered();
private:
    Ui::fileExistsDialog *ui;
    FileExistsAction action;
    std::string oldName;
    std::string destinationInfo;
    std::string firstRenamingRule;
    std::string otherRenamingRule;
};

#endif // FILEEXISTSDIALOG_H
