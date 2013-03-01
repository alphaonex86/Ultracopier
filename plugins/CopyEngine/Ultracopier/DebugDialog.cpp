/** \file debugDialog.cpp
\brief Define the dialog to have debug information
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "DebugDialog.h"
#include "ui_debugDialog.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW

DebugDialog::DebugDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::debugDialog)
{
    ui->setupUi(this);
}

DebugDialog::~DebugDialog()
{
    delete ui;
}

void DebugDialog::setTransferList(const QStringList &list)
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

void DebugDialog::setActiveTransfer(int activeTransfer)
{
    ui->spinBoxActiveTransfer->setValue(activeTransfer);
}

void DebugDialog::setInodeUsage(int inodeUsage)
{
    ui->spinBoxNumberOfInode->setValue(inodeUsage);
}

void DebugDialog::setTransferThreadList(const QStringList &list)
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
