#include "ProductKey.h"
#include "ui_ProductKey.h"
#include "DebugEngine.h"
#include "OptionEngine.h"
#include "SystrayIcon.h"
#include "FacilityEngine.h"
#if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
#include "EventDispatcher.h"
#endif
#include <QMessageBox>
#include <QCryptographicHash>

ProductKey::ProductKey(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProductKey)
{
    ui->setupUi(this);
    parseKey();

    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    connect(&newUpdateTimer,&QTimer::timeout,this,&ProductKey::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&ProductKey::downloadFile);
    newUpdateTimer.start(1000*3600*71);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(1000*15);
    reply=NULL;
    qnam=new QNetworkAccessManager();
    #endif
}

ProductKey::~ProductKey()
{
    delete ui;

    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    if(reply!=NULL)
    {
        delete reply;
        reply=NULL;
    }
    delete qnam;
    #endif
}

bool ProductKey::parseKey(QString orgkey)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultimate key");
    orgkey.replace(" ","");
    orgkey.replace("\t","");
    orgkey.replace("\n","");
    orgkey.replace("\r","");
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
            #ifdef ULTRACOPIER_INTERNET_SUPPORT
            if(bannedList.contains(lastKey) && bannedList.size()>5 && !lastKey.isEmpty())
            {
                ultimate=false;
                bannedKey();
            }
            else
            #endif
            {
                if(!orgkey.isEmpty())
                    OptionEngine::optionEngine->setOptionValue("Ultracopier","key",key.toStdString());
                ultimate=true;
                #ifdef ULTRACOPIER_INTERNET_SUPPORT
                lastKey=key;
                #endif
            }
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

bool ProductKey::isIllegal() const
{
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    if(bannedList.contains(lastKey) && bannedList.size()>5 && !lastKey.isEmpty())
        return true;
    else
        return false;
    #else
    return false;
    #endif
}

void ProductKey::on_buttonBox_accepted()
{
    if(!ProductKey::parseKey(ui->productkey->text()))
    {
        QString extra;
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        if(bannedList.contains(ui->productkey->text()) && bannedList.size()>5 && !lastKey.isEmpty())
            extra+=tr("You have illegal key, we will start legal procedure shortly");
        else
        #endif
        {
            if(ui->productkey->text().startsWith("UC1"))
                extra+=tr("The major update is not free to support the project.<br />For years I had to update the site and software, fight against hacking attempts, provide user support (and correct bugs in less than 2 hours).<br />The version 1.x was free update during more than 5 years and have years old.<br />The version 2.x was free update during more than 7 years and have years old. (and lot of have stole this version)<br />This is the only way to be able to have time for this project that I am the only one to maintain and without help.<br />You can use the old version and disable update detection<br /><a href=\"https://cdn.confiared.com/ultracopier.herman-brule.com/files/2.2.7.1/ultracopier-windows-x86_64-2.2.7.1-setup.exe\">ultracopier-windows-x86_64-2.2.7.1-setup.exe</a>");
            else if(ui->productkey->text().startsWith("UC2"))
                extra+=tr("The major update is not free to support the project.<br />For years I had to update the site and software, fight against hacking attempts, provide user support (and correct bugs in less than 2 hours).<br />The version 1.x was free update during more than 5 years and have years old.<br />The version 2.x was free update during more than 7 years and have years old. (and lot of have stole this version)<br />This is the only way to be able to have time for this project that I am the only one to maintain and without help.<br />You can use the old version and disable update detection<br /><a href=\"https://cdn.confiared.com/ultracopier.herman-brule.com/files/2.2.7.1/ultracopier-windows-x86_64-2.2.7.1-setup.exe\">ultracopier-windows-x86_64-2.2.7.1-setup.exe</a>");
            else if(ui->productkey->text().startsWith("UC3"))
                extra+=tr("This kind of key should work, check if you have copied extra characters");
            else if(ui->productkey->text().startsWith("UC4") || ui->productkey->text().startsWith("UC5") || ui->productkey->text().startsWith("UC6"))
                extra+=tr("This key is for version %1, then try upgrade this version for this version. Go to <a href=\"https://ultracopier.herman-brule.com/#download\">https://ultracopier.herman-brule.com/#download</a>").arg(4);
            else
                extra+=tr("Your product key \"%1\" was rejected for this version %2.<br />If you buy key, unmark check your spam and unmark the mail as spam").arg(ui->productkey->text()).arg(QString::fromStdString(FacilityEngine::version()));
            extra+=tr("<br />If you have not buy your key, go to <a href=\"https://ultracopier.herman-brule.com/#ultimate\">https://ultracopier.herman-brule.com/#ultimate</a>");
        }
        QMessageBox::critical(this,tr("Error"),"<br />"+extra);
    }
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

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void ProductKey::checkUpdate()
{
    downloadFileInternal();
}

void ProductKey::downloadFile()
{
    downloadFileInternal();
}

void ProductKey::downloadFileInternal()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
         std::string name="Supercopier";
     #else
         std::string name="Ultracopier";
     #endif
    std::string ultracopierVersion;
    if(ProductKey::productKey!=NULL && ProductKey::productKey->isUltimate())
        ultracopierVersion=name+" Ultimate/"+FacilityEngine::version();
    else
        ultracopierVersion=name+"/"+FacilityEngine::version();
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
             ultracopierVersion+=" portable/all-in-one";
        #else
             ultracopierVersion+=" portable";
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            ultracopierVersion+=" all-in-one";
        #endif
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"before OS detection");
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    ultracopierVersion+=" (OS: "+EventDispatcher::GetOSDisplayString()+")";
    #endif
    #ifndef ULTRACOPIER_VERSION_ULTIMATE
    std::string key=OptionEngine::optionEngine->getOptionValue("Ultracopier","key");
    if(!key.empty())
        ultracopierVersion+=" "+key;
    #endif
    ultracopierVersion+=" "+std::string(ULTRACOPIER_PLATFORM_CODE);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"after OS detection");
    QNetworkRequest networkRequest(QStringLiteral(ULTRACOPIER_BAN_URL));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,QString::fromStdString(ultracopierVersion));
    networkRequest.setRawHeader("Connection", "Close");
    reply = qnam->get(networkRequest);
    if(reply==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"null get");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"before connect");
    connect(reply, &QNetworkReply::finished, this, &ProductKey::httpFinished);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"after connect");
}

void ProductKey::httpFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(reply==NULL)
        return;
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"get the new update failed: not finished");
        reply->deleteLater();
        reply=NULL;
        return;
    }
    else if (!redirectionTarget.isNull())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"redirection denied from "+std::string(ULTRACOPIER_UPDATER_URL)+" to: "+redirectionTarget.toUrl().toString().toStdString());
        reply->deleteLater();
        reply=NULL;
        return;
    }
    else if (reply->error())
    {
        newUpdateTimer.stop();
        newUpdateTimer.start(1000*3600*24);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"get the new update failed: "+reply->errorString().toStdString()+" QNetworkReply::NetworkError: "+std::to_string((int)reply->error()));
        reply->deleteLater();
        reply=NULL;
        return;
    }
    bannedList=QString::fromUtf8(reply->readAll());
    if(bannedList.contains(lastKey) && bannedList.size()>5)
    {
        ultimate=false;
        changeToUltimate();
    }
    newUpdateTimer.stop();
    reply->deleteLater();
    reply=NULL;
    //regen to force close the connection
    delete qnam;
    qnam=new QNetworkAccessManager();
}
#endif
