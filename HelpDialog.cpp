/** \file HelpDialog.cpp
\brief Define the help dialog
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "HelpDialog.h"
#include "ProductKey.h"
#include "FacilityEngine.h"

#include <QTreeWidgetItem>
#include <QApplication>
#ifdef Q_OS_WIN32
#include <windows.h>
#include <QDir>
#endif

/// \brief Construct the object
HelpDialog::HelpDialog() :
    ui(new Ui::HelpDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ui->setupUi(this);
    reloadTextValue();
    #ifdef ULTRACOPIER_DEBUG
    ui->debugView->setModel(DebugModel::debugModel);
    connect(ui->pushButtonSaveBugReport,&QPushButton::clicked,DebugEngine::debugEngine,&DebugEngine::saveBugReport);
    #else // ULTRACOPIER_DEBUG
    ui->lineEditInsertDebug->hide();
    ui->debugView->hide();
    ui->pushButtonSaveBugReport->hide();
    ui->pushButtonCrash->hide();
    this->setMaximumSize(QSize(500,128));
    /*timeToSetText.setInterval(250);
    timeToSetText.setSingleShot(true);
    connect(&timeToSetText,QTimer::timeout,this,&DebugEngine::showDebugText);*/
    ui->pushButtonClose->hide();
    #endif // ULTRACOPIER_DEBUG
    //connect the about Qt
    connect(ui->pushButtonAboutQt,&QPushButton::toggled,&QApplication::aboutQt);
    setWindowTitle(tr("About Ultracopier"));
    #ifndef ULTRACOPIER_INTERNET_SUPPORT
    ui->checkUpdate->hide();
    #endif
}

/// \brief Destruct the object
HelpDialog::~HelpDialog()
{
    delete ui;
}

/// \brief To re-translate the ui
void HelpDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        ui->retranslateUi(this);
        reloadTextValue();
        break;
    default:
        break;
    }
}

/// \brief To reload the text value
void HelpDialog::reloadTextValue()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QString text=ui->label_ultracopier->text();
    if(ProductKey::productKey->isUltimate())
        text=text.replace(QStringLiteral("%1"),QStringLiteral("Ultimate %1").arg(QString::fromStdString(FacilityEngine::version())));
    else
        text=text.replace(QStringLiteral("%1"),QString::fromStdString(FacilityEngine::version()));
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
    text=text.replace(QStringLiteral("Ultracopier"),QStringLiteral("Supercopier"),Qt::CaseInsensitive);
    #endif
    ui->label_ultracopier->setText(text);

    text=ui->label_description->text();
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_VERSION_PORTABLEAPPS
            text=text.replace(QStringLiteral("%1"),tr("For http://portableapps.com/"));
        #else
            #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
                text=text.replace(QStringLiteral("%1"),tr("Portable and all in one version"));
            #else
                text=text.replace(QStringLiteral("%1"),tr("Portable version"));
            #endif
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            text=text.replace(QStringLiteral("%1"),tr("All in one version"));
        #else
            text=text.replace(QStringLiteral("%1"),tr("Normal version"));
        #endif
    #endif
    ui->label_description->setText(text);

    text=ui->label_site->text();
    //: This site need be the official site of ultracopier, into the right languages, english if not exists
    text=text.replace("%1",QString::fromStdString(getWebSite()));
    ui->label_site->setText(text);

    text=ui->label_platform->text();
    text=text.replace(QStringLiteral("%1"),ULTRACOPIER_PLATFORM_NAME);

    #ifdef Q_OS_WIN32
        #if defined(_M_X64)//64Bits
        #else//32Bits
            char *arch=getenv("windir");
            if(arch!=NULL)
            {
                QDir dir;
                if(dir.exists(QString(arch)+"\\SysWOW64\\"))
                    text+=tr(" on Windows 64Bits");
            }
        #endif
    #endif

    #ifdef __clang_patchlevel__
    text+=tr(", gcc %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
    #else
        #ifdef __GNUC_PATCHLEVEL__
        text+=tr(", gcc %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
        #endif
    #endif
    #ifdef QT_VERSION
    text+=tr(", Qt %1").arg(QT_VERSION_STR);
    #endif
    ui->label_platform->setText(text);
}

std::string HelpDialog::getWebSite()
{
    return tr("https://ultracopier.herman-brule.com/").toStdString();
}


std::string HelpDialog::getUpdateUrl()
{
    return tr("https://ultracopier.herman-brule.com/#download").toStdString();
}

#ifdef ULTRACOPIER_DEBUG
void HelpDialog::on_lineEditInsertDebug_returnPressed()
{
    DebugEngine::addDebugNote(ui->lineEditInsertDebug->text().toStdString());
    ui->lineEditInsertDebug->clear();
    ui->debugView->scrollToBottom();
}
#endif // ULTRACOPIER_DEBUG

void HelpDialog::on_pushButtonAboutQt_clicked()
{
    QApplication::aboutQt();
}

void HelpDialog::on_pushButtonCrash_clicked()
{
    int a=0;
    int *b=NULL;
    *b=3/a;
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void HelpDialog::on_checkUpdate_clicked()
{
    ui->status->setText(tr("Update checking..."));
    emit checkUpdate();
}

void HelpDialog::newUpdate(const std::string &version) const
{
    ui->status->setText(tr("Update: %1").arg(QString::fromStdString(version)));
}

void HelpDialog::errorUpdate(const std::string &errorString) const
{
    ui->status->setText(tr("Update: %1").arg(QString::fromStdString(errorString)));
}

void HelpDialog::noNewUpdate() const
{
    if(!ui->status->text().isEmpty())
        ui->status->setText(tr("No update"));
}
#endif
