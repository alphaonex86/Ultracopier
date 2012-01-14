#ifndef DEBUGDAILOG_H
#define DEBUGDAILOG_H

#include "Environment.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#include <QWidget>

namespace Ui {
    class debugDialog;
}

class debugDialog : public QWidget
{
    Q_OBJECT

public:
	explicit debugDialog(QWidget *parent = 0);
	~debugDialog();
	void setTransferList(const QStringList &list);
	void setTransferThreadList(const QStringList &list);
	void setActiveTransfer(int activeTransfer);
	void setInodeUsage(int inodeUsage);
private:
	Ui::debugDialog *ui;
};

#endif // ULTRACOPIER_PLUGIN_DEBUG_WINDOW

#endif // DEBUGDAILOG_H
