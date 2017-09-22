/** \file debugDialog.cpp
\brief Define the dialog to have debug information
\author alpha_one_x86 */

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

void DebugDialog::setTransferList(const std::vector<std::string> &list)
{
    ui->tranferList->clear();
    ui->tranferList->addItems(list);
}

void DebugDialog::setActiveTransfer(const int &activeTransfer)
{
    ui->spinBoxActiveTransfer->setValue(activeTransfer);
}

void DebugDialog::setInodeUsage(const int &inodeUsage)
{
    ui->spinBoxNumberOfInode->setValue(inodeUsage);
}

void DebugDialog::setTransferThreadList(const std::vector<std::string> &list)
{
    ui->transferThreadList->clear();
    ui->transferThreadList->addItems(list);
}

#endif
