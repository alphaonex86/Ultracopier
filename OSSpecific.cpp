#include "OSSpecific.h"
#include "ui_OSSpecific.h"

OSSpecific::OSSpecific(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OSSpecific)
{
    ui->setupUi(this);
#ifndef Q_OS_WIN32
    #if defined(Q_OS_LINUX)
    ui->label->setText(tr("The replacement of default copy/move system is not supported by the file manager (Dolphin, Nautilus, ...).<br />Ask to the developer to support it."));
    #elif defined(Q_OS_WIN32)
    ui->label->setText(tr("Don't forget to reboot if previously was installed other similar software<br />(like: Supercopier, Ultracopier in previous version, ...)"));
    #elif defined(Q_OS_MAC)
    ui->label->setText(tr("The replacement of default copy/move system is not supported and blocked by finder of Mac OS X."));
    #else
    ui->label->setText(tr("The replacement of default copy/move system should be not supported by the file manager.<br />Ask to the developer to support it."));
    #endif
#endif
    if(!QIcon::fromTheme("dialog-warning").isNull())
        setWindowIcon(QIcon::fromTheme("dialog-warning"));
}

OSSpecific::~OSSpecific()
{
    delete ui;
}

bool OSSpecific::dontShowAgain()
{
    return ui->dontShowAgain->isChecked();
}

void OSSpecific::on_pushButton_clicked()
{
    close();
}
