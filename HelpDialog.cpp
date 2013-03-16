/** \file HelpDialog.cpp
\brief Define the help dialog
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "HelpDialog.h"

#include <QTreeWidgetItem>
#include <QApplication>

/// \brief Construct the object
HelpDialog::HelpDialog() :
    ui(new Ui::HelpDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ui->setupUi(this);
    reloadTextValue();
    #ifdef ULTRACOPIER_DEBUG
    ui->debugView->setModel(&DebugModel::debugModel);
    connect(ui->pushButtonSaveBugReport,&QPushButton::clicked,debug_engine_instance,&DebugEngine::saveBugReport);
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
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    text=text.replace("%1",QString("Ultimate %1").arg(ULTRACOPIER_VERSION));
    #else
    text=text.replace("%1",ULTRACOPIER_VERSION);
    #endif
    ui->label_ultracopier->setText(text);

    text=ui->label_description->text();
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_VERSION_PORTABLEAPPS
            text=text.replace("%1",tr("For http://portableapps.com/"));
        #else
             text=text.replace("%1",tr("Portable version"));
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            text=text.replace("%1",tr("All in one version"));
        #else
            text=text.replace("%1",tr("Normal version"));
        #endif
    #endif
    ui->label_description->setText(text);

    text=ui->label_site->text();
    //: This site need be the official site of ultracopier, into the right languages, english if not exists
    text=text.replace("%1",tr("http://ultracopier.first-world.info/"));
    ui->label_site->setText(text);

    text=ui->label_platform->text();
    text=text.replace("%1",ULTRACOPIER_PLATFORM_NAME);
    ui->label_platform->setText(text);
}

#ifdef ULTRACOPIER_DEBUG
void HelpDialog::on_lineEditInsertDebug_returnPressed()
{
    DebugEngine::addDebugNote(ui->lineEditInsertDebug->text());
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
