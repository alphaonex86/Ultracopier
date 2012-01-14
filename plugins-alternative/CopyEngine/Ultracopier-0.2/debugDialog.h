#ifndef DEBUGDAILOG_H
#define DEBUGDAILOG_H

#include "Environment.h"

#include <QWidget>

#ifdef DEBUG_WINDOW

namespace Ui {
    class debugDialog;
}

class debugDialog : public QWidget
{
    Q_OBJECT

public:
    explicit debugDialog(QWidget *parent = 0);
    ~debugDialog();
	void setTransferList(QStringList list);
	void setWriteList(QList<DebugWriteThread>);
	void setReadStatus(bool isRunning);
	void setReadBlocking(bool isBlocked);
private:
    Ui::debugDialog *ui;
};

#else
class debugDialog : public QObject {Q_OBJECT};
#endif // DEBUG_WINDOW

#endif // DEBUGDAILOG_H
