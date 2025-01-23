#include "ProductKey.h"
#include "ui_ProductKey.h"
#include "DebugEngine.h"
#include "OptionEngine.h"
#include "SystrayIcon.h"
#include "FacilityEngine.h"
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
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(QStringLiteral("TxUd5cp4dwAqHAHZUhH84FuT").toUtf8());//a salt
        hash.addData(key.toUtf8());
        const QByteArray &result=hash.result();
        if(!result.isEmpty() &&
            result.at(0)==0x00 && result.at(1)==0x00 && result.at(2)==0x21 && (result.at(3)&0x0f)==0x00
            )
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
#ifdef ULTRACOPIER_VERSION_ULTIMATE
    return true;
#else
    return ultimate;
#endif
}

void ProductKey::on_buttonBox_accepted()
{
    if(!ProductKey::parseKey(ui->productkey->text()))
        QMessageBox::critical(this,tr("Error"),"<br />"+tr("Your product key was rejected for this version %1.<br /><b>If you have just buy the key, try download the last version.</b><br />If you buy key, unmark check your spam and unmark the mail as spam<br />If you have not buy your key, go to <a href=\"https://ultracopier.herman-brule.com/#ultimate\">https://ultracopier.herman-brule.com/#ultimate</a>").arg(QString::fromStdString(FacilityEngine::version())));
    else
    {
        changeToUltimate();
        hide();
        QString extra
            #ifdef Q_OS_WIN32
            =tr("<br />Restart to enable the option high performance into option -> Copy Engine -> Ultracopier Spec -> Performance -> OS native copy")
            #endif
            ;
        QMessageBox::information(this,tr("Informations"),"<br />"+tr("You have correctly enabled your key.")+extra);
    }
}

void ProductKey::on_buttonBox_rejected()
{
    hide();
}
