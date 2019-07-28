#include "RenamingRules.h"
#include "ui_RenamingRules.h"

#include <QMessageBox>

RenamingRules::RenamingRules(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::RenamingRules)
{
    ui->setupUi(this);
    connectUI();
    setRenamingRules("","");
}

RenamingRules::~RenamingRules()
{
    delete ui;
}

void RenamingRules::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::RejectRole)
        reject();
    if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::ResetRole)
    {
        setRenamingRules("","");
        emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
    }
}

void RenamingRules::setRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule)
{
    disconnectUI();
    if(firstRenamingRule.find("%name%")==std::string::npos || firstRenamingRule.find("%suffix%")==std::string::npos)
        firstRenamingRule.clear();
    if(otherRenamingRule.find("%name%")==std::string::npos || otherRenamingRule.find("%suffix%")==std::string::npos
             || otherRenamingRule.find("%number%")==std::string::npos)
        otherRenamingRule.clear();

    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;

    if(!firstRenamingRule.empty())
        ui->firstRenamingRule->setText(QString::fromStdString(firstRenamingRule));
    else
        ui->firstRenamingRule->setText(tr("%1 - copy%2").arg(QStringLiteral("%name%")).arg(QStringLiteral("%suffix%")));
    if(!otherRenamingRule.empty())
        ui->otherRenamingRule->setText(QString::fromStdString(otherRenamingRule));
    else
        ui->otherRenamingRule->setText(tr("%1 - copy (%2)%3").arg(QStringLiteral("%name%")).arg(QStringLiteral("%number%")).arg(QStringLiteral("%suffix%")));
    connectUI();
}

void RenamingRules::connectUI()
{
    connect(ui->firstRenamingRule,&QLineEdit::editingFinished,this,&RenamingRules::firstRenamingRule_haveChanged);
    connect(ui->otherRenamingRule,&QLineEdit::editingFinished,this,&RenamingRules::otherRenamingRule_haveChanged);
}

void RenamingRules::disconnectUI()
{
    disconnect(ui->firstRenamingRule,&QLineEdit::editingFinished,this,&RenamingRules::firstRenamingRule_haveChanged);
    disconnect(ui->otherRenamingRule,&QLineEdit::editingFinished,this,&RenamingRules::otherRenamingRule_haveChanged);
}

void RenamingRules::firstRenamingRule_haveChanged()
{
    QString newValue=ui->firstRenamingRule->text();
    if(!newValue.contains("%name%") || !newValue.contains("%suffix%"))
        newValue.clear();
    if(newValue==tr("%1 - copy%2").arg(QStringLiteral("%name%")).arg(QStringLiteral("%name%")))
        newValue=QStringLiteral("");
    if(newValue.toStdString()==firstRenamingRule)
        return;
    firstRenamingRule=newValue.toStdString();
    emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void RenamingRules::otherRenamingRule_haveChanged()
{
    QString newValue=ui->otherRenamingRule->text();
    if(!newValue.contains("%name%") || !newValue.contains("%suffix%") || !newValue.contains("%number%"))
        newValue.clear();
    if(newValue==tr("%1 - copy (%2)%3").arg(QStringLiteral("%name%")).arg(QStringLiteral("%number%")).arg(QStringLiteral("%name%")))
        newValue=QStringLiteral("");
    if(newValue.toStdString()==otherRenamingRule)
        return;
    otherRenamingRule=newValue.toStdString();
    emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void RenamingRules::newLanguageLoaded()
{
    ui->retranslateUi(this);
    setRenamingRules(firstRenamingRule,otherRenamingRule);
}
