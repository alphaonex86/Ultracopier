#include "RenamingRules.h"
#include "ui_RenamingRules.h"

#include <QMessageBox>

RenamingRules::RenamingRules(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::RenamingRules)
{
    ui->setupUi(this);
    connectUI();
    setRenamingRules(QStringLiteral(""),QStringLiteral(""));
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
        setRenamingRules(QStringLiteral(""),QStringLiteral(""));
        emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
    }
}

void RenamingRules::setRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule)
{
    disconnectUI();
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    if(!firstRenamingRule.isEmpty())
        ui->firstRenamingRule->setText(firstRenamingRule);
    else
        ui->firstRenamingRule->setText(tr("%1 - copy").arg(QStringLiteral("%name%")));
    if(!otherRenamingRule.isEmpty())
        ui->otherRenamingRule->setText(otherRenamingRule);
    else
        ui->otherRenamingRule->setText(tr("%1 - copy (%2)").arg(QStringLiteral("%name%")).arg(QStringLiteral("%number%")));
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
    if(newValue==tr("%1 - copy").arg(QStringLiteral("%name%")))
        newValue=QStringLiteral("");
    if(newValue==firstRenamingRule)
        return;
    firstRenamingRule=newValue;
    emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void RenamingRules::otherRenamingRule_haveChanged()
{
    QString newValue=ui->otherRenamingRule->text();
    if(newValue==tr("%1 - copy (%2)").arg(QStringLiteral("%name%")).arg(QStringLiteral("%number%")))
        newValue=QStringLiteral("");
    if(newValue==otherRenamingRule)
        return;
    otherRenamingRule=newValue;
    emit sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void RenamingRules::newLanguageLoaded()
{
    ui->retranslateUi(this);
    setRenamingRules(firstRenamingRule,otherRenamingRule);
}
