/** \file fileErrorDialog.h
\brief Define the dialog error on the file
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QDateTime>
#include <string>
#include "Environment.h"
#include "../../../interface/FacilityInterface.h"

#ifndef FILEERRORDIALOG_H
#define FILEERRORDIALOG_H

#ifdef WIDESTRING
#define INTERNALTYPEPATH std::wstring
#else
#define INTERNALTYPEPATH std::string
#endif

namespace Ui {
    class fileErrorDialog;
}

/// \brief to show error dialog, and ask what do
class FileErrorDialog : public QDialog
{
    Q_OBJECT
public:
    /// \brief create the object and pass all the informations to it
    explicit FileErrorDialog(QWidget *parent, INTERNALTYPEPATH fileInfo, std::string errorString, const ErrorType &errorType, FacilityInterface *facilityEngine);
    virtual ~FileErrorDialog();
    /// \brief return the the always checkbox is checked
    virtual bool getAlways();
    /// \brief return the action clicked.
    /// VIRTUAL on purpose: a headless test build subclasses this dialog and returns a scripted action
    /// without ever showing the GUI (it also overrides QDialog::exec() to return immediately). This is
    /// the clean seam for testing the error dialog -- NO #ifdef / env / test logic lives in the engine.
    virtual FileErrorAction getAction();
    /// \brief create a FileErrorDialog. The engine creates EVERY fileError dialog through this so a test
    /// can substitute a subclass. overrideFactory is nullptr in production (-> a normal dialog); only a
    /// test build's static initializer sets it. (Plain function pointer + .cpp definition keeps this
    /// C++11/mingw-4.9.2 safe -- no C++17 inline static.)
    static FileErrorDialog *createInstance(QWidget *parent, INTERNALTYPEPATH fileInfo, std::string errorString, const ErrorType &errorType, FacilityInterface *facilityEngine);
    static FileErrorDialog *(*overrideFactory)(QWidget *, INTERNALTYPEPATH, std::string, const ErrorType &, FacilityInterface *);
protected:
    void changeEvent(QEvent *e);
    static bool isInAdmin;
private slots:
    void on_PutToBottom_clicked();
    void on_Retry_clicked();
    void on_Skip_clicked();
    void on_Cancel_clicked();
    void on_checkBoxAlways_clicked();
    #ifdef ULTRACOPIER_PLUGIN_RIGHTS
    void on_Rights_clicked();
    #endif
private:
    Ui::fileErrorDialog *ui;
    FileErrorAction action;
};

#endif // FILEERRORDIALOG_H
