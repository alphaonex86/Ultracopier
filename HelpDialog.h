/** \file HelpDialog.h
\brief Define the help dialog
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>
#include <QTimer>
#include <QColor>
#include <QBrush>

#include "ui_HelpDialog.h"
#include "Environment.h"

namespace Ui {
    class HelpDialog;
}

/** \brief Help dialog, and some user oriented repport/debug function */
class HelpDialog : public QDialog {
    Q_OBJECT
    public:
        /// \brief Construct the object
        HelpDialog();
        /// \brief Destruct the object
        ~HelpDialog();
        static std::string getWebSite();
        static std::string getUpdateUrl();

        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        void newUpdate(const std::string &version) const;
        void errorUpdate(const std::string &errorString) const;
        void noNewUpdate() const;
        #endif
    protected:
        /// \brief To re-translate the ui
        void changeEvent(QEvent *e);
    private:
        Ui::HelpDialog *ui;
        /// \brief To reload the text value
        void reloadTextValue();
    private slots:
        #ifdef ULTRACOPIER_DEBUG
        /// \brief Add debug text
        void on_lineEditInsertDebug_returnPressed();
        #endif // ULTRACOPIER_DEBUG
        void on_pushButtonAboutQt_clicked();
        void on_pushButtonCrash_clicked();
        void on_checkUpdate_clicked();
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
    signals:
        void checkUpdate();
        #endif
};

#endif // DIALOG_H
