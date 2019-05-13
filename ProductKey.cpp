#include "ProductKey.h"
#include "ui_ProductKey.h"
#include "DebugEngine.h"
#include "OptionEngine.h"
#include "SystrayIcon.h"
#include <QMessageBox>
#include <QCryptographicHash>

ProductKey::ProductKey(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProductKey)
{
    ui->setupUi(this);
    parseKey();
}

ProductKey::~ProductKey()
{
    delete ui;
}

bool ProductKey::parseKey(QString orgkey)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultimate key");
    QString key=orgkey;
    if(orgkey.isEmpty())
        key=QString::fromStdString(OptionEngine::optionEngine->getOptionValue("Ultracopier","key"));
    if(!key.isEmpty())
    {
        QCryptographicHash hash(QCryptographicHash::Sha224);
        hash.addData(QStringLiteral("mQcLvEg1HW8JuRXY3BawjSpe").toUtf8());
        hash.addData(key.toUtf8());
        const QByteArray &result=hash.result();
        if(!result.isEmpty() && result.at(0)==0x00 && result.at(1)==0x00)
        {
            if(!orgkey.isEmpty())
            OptionEngine::optionEngine->setOptionValue("Ultracopier","key",key.toStdString());
            ultimate=true;
        }
        else
            ultimate=false;
    }
    else
        ultimate=false;
    return ultimate;
}

bool ProductKey::isUltimate() const
{
    return ultimate;
}

void ProductKey::on_buttonBox_accepted()
{
    if(!ProductKey::parseKey(ui->productkey->text()))
        QMessageBox::critical(this,tr("Error"),"<br />"+tr("Your product key was rejected.<br />If you buy key, unmark check your spam and unmark the mail as spam<br />If you have not buy your key, go to <a href=\"https://shop.first-world.info/\">https://shop.first-world.info/</a>"));
    else
    {
        changeToUltimate();
        hide();
    }
}

void ProductKey::on_buttonBox_rejected()
{
    hide();
}
