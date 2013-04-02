#include "OSSpecific.h"
#include "ui_OSSpecific.h"

OSSpecific::OSSpecific(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OSSpecific)
{
    ui->setupUi(this);
    #if defined(Q_OS_LINUX)
    ui->label->setText(tr("The replacement of default copy/move system is not supported by the file manager (Dolphin, Nautilus, ...).<br />Ask the developer to support it.<br />You need do the copy/move manually."));
    #elif defined(Q_OS_WIN32)
    ui->label->setText(tr("Reboot the system if previously had similar software installed (like Teracopy, Supercopier or an earlier version of Ultracopier)."));
    #elif defined(Q_OS_MAC)
    ui->label->setText(tr("The replacement of default copy/move system is not supported and blocked by finder of Mac OS X.<br />You need do the copy/move manually by right clicking on the system tray icon near the clock (not the dock icon)."));
    #else
    ui->label->setText(tr("The replacement of default copy/move system should be not supported by the file manager.<br />Ask to the developer to support it.<br />You need do the copy/move manually."));
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
