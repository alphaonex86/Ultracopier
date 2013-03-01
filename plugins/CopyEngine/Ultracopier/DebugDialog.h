/** \file debugDialog.h
\brief Define the dialog to have debug information
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef DEBUGDAILOG_H
#define DEBUGDAILOG_H

#include "Environment.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#include <QWidget>

namespace Ui {
    class debugDialog;
}

/// \brief class to the dialog to have debug information
class DebugDialog : public QWidget
{
    Q_OBJECT
public:
    explicit DebugDialog(QWidget *parent = 0);
    ~DebugDialog();
    /// \brief to set the transfer list, limited in result to not slow down the application
    void setTransferList(const QStringList &list);
    /// \brief show the transfer thread, it show be a thread pool in normal time
    void setTransferThreadList(const QStringList &list);
    /// \brief show how many transfer is active
    void setActiveTransfer(int activeTransfer);
    /// \brief show many many inode is manipulated
    void setInodeUsage(int inodeUsage);
private:
    Ui::debugDialog *ui;
};

#endif // ULTRACOPIER_PLUGIN_DEBUG_WINDOW

#endif // DEBUGDAILOG_H
