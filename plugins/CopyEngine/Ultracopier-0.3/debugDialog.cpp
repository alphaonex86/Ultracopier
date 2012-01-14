#include "debugDialog.h"
#include "ui_debugDialog.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW

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

void debugDialog::setTransferList(const QStringList &list)
{
	if(list.size()==ui->tranferList->count())
	{
		int index=0;
		while(index<list.size())
		{
			ui->tranferList->item(index)->setText(list.at(index));
			index++;
		}
	}
	else
	{
		ui->tranferList->clear();
		ui->tranferList->addItems(list);
	}
}

void debugDialog::setActiveTransfer(int activeTransfer)
{
	ui->spinBoxActiveTransfer->setValue(activeTransfer);
}

void debugDialog::setInodeUsage(int inodeUsage)
{
	ui->spinBoxNumberOfInode->setValue(inodeUsage);
}

void debugDialog::setTransferThreadList(const QStringList &list)
{
	if(list.size()==ui->transferThreadList->count())
	{
		int index=0;
		while(index<list.size())
		{
			ui->transferThreadList->item(index)->setText(list.at(index));
			index++;
		}
	}
	else
	{
		ui->transferThreadList->clear();
		ui->transferThreadList->addItems(list);
	}
}

#endif
