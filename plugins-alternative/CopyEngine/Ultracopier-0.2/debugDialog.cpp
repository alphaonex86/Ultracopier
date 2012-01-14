#include "debugDialog.h"

#ifdef DEBUG_WINDOW

#include "ui_debugDialog.h"

debugDialog::debugDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::debugDialog)
{
    ui->setupUi(this);
}

debugDialog::~debugDialog()
{
    delete ui;
}

void debugDialog::setTransferList(QStringList list)
{
	ui->tranferList->clear();
	ui->tranferList->addItems(list);
}

void debugDialog::setWriteList(QList<DebugWriteThread> list)
{
	while(list.size()<(ui->writeThreadList->count()))
		delete ui->writeThreadList->item(ui->writeThreadList->count()-1);
	while(list.size()>(ui->writeThreadList->count()))
		ui->writeThreadList->addItem("");
	int count=ui->writeThreadList->count();
	int index=0;
	while(index<list.size())
	{
		if(index>=count)
			break;
		switch(list.at(index))
		{
			case Running_Blocked:
				ui->writeThreadList->item(index)->setBackground(QBrush(QColor(255,0,0)));
				ui->writeThreadList->item(index)->setText("Running_Blocked");
			break;
			case Running:
				ui->writeThreadList->item(index)->setBackground(QBrush(QColor(0,255,0)));
				ui->writeThreadList->item(index)->setText("Running");
			break;
			case Not_Running:
				ui->writeThreadList->item(index)->setBackground(QBrush(QColor(128,128,128)));
				ui->writeThreadList->item(index)->setText("Not_Running");
			break;
		}
		index++;
	}
}

void debugDialog::setReadStatus(bool isRunning)
{
	ui->readStatus->setChecked(isRunning);
}

void debugDialog::setReadBlocking(bool isBlocked)
{
	ui->readBlocking->setChecked(isBlocked);
}

#endif // DEBUG_WINDOW
