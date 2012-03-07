/** \file debugDialog.h
\brief Define the dialog to have debug information
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef DEBUGDAILOG_H
#define DEBUGDAILOG_H

#include "Environment.h"

#include <QWidget>

#ifdef DEBUG_WINDOW

namespace Ui {
    class debugDialog;
}

/// \brief class to the dialog to have debug information
class debugDialog : public QWidget
{
	Q_OBJECT
public:
	explicit debugDialog(QWidget *parent = 0);
	~debugDialog();
	/// \brief to set the transfer list, limited in result to not slow down the application
	void setTransferList(QStringList list);
	/// \brief show the transfer thread, it show be a thread pool in normal time
	void setWriteList(QList<DebugWriteThread>);
	/// \brief show how many transfer is active
	void setReadStatus(bool isRunning);
	/// \brief show many many inode is manipulated
	void setReadBlocking(bool isBlocked);
private:
	Ui::debugDialog *ui;
};

#else
/// \brief class to the dialog to have debug information
class debugDialog : public QObject {Q_OBJECT};
#endif // DEBUG_WINDOW

#endif // DEBUGDAILOG_H
