/** \file fileIsSameDialog.h
\brief Define the dialog when file is same
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <string>
#include "Environment.h"
#include "../../../interface/FacilityInterface.h"

#ifndef FILEISSAMEDIALOG_H
#define FILEISSAMEDIALOG_H

namespace Ui {
    class fileIsSameDialog;
}

/// \brief to show file is same dialog, and ask what do
class FileIsSameDialog : public QDialog
{
    Q_OBJECT
public:
    /// \brief create the object and pass all the informations to it
    explicit FileIsSameDialog(QWidget *parent, std::string fileInfo, std::string firstRenamingRule, std::string otherRenamingRule, FacilityInterface *facilityEngine);
    ~FileIsSameDialog();
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
    void on_Skip_clicked();
    void on_Cancel_clicked();
    void updateRenameButton();
    void on_lineEditNewName_textChanged(const QString &arg1);
    void on_checkBoxAlways_toggled(bool checked);
    void on_lineEditNewName_returnPressed();
    void on_lineEditNewName_editingFinished();
private:
    Ui::fileIsSameDialog *ui;
    FileExistsAction action;
    std::string oldName;
    std::string destinationInfo;
    std::string firstRenamingRule;
    std::string otherRenamingRule;

};

#endif // FILEISSAMEDIALOG_H
