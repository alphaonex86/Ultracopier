#include "OptionsWidget.h"
#include "ui_OptionsWidget.h"

OptionsWidget::OptionsWidget(QWidget *parent) :
        QWidget(parent),
    ui(new Ui::OptionsWidget)
{
    ui->setupUi(this);
}

OptionsWidget::~OptionsWidget()
{
    //delete ui;//attached to the main program, then it's the main program responsive the delete
}

void OptionsWidget::setAllDllIsImportant(bool allDllIsImportant)
{
    ui->allDllIsImportant->setChecked(allDllIsImportant);
}

void OptionsWidget::setAllUserIsImportant(bool allDllIsImportant)
{
    ui->allDllIsImportant->setChecked(allDllIsImportant);
}

void OptionsWidget::setDebug(bool Debug)
{
    ui->Debug->setChecked(Debug);
}

void OptionsWidget::on_allDllIsImportant_toggled(bool checked)
{
    emit sendAllDllIsImportant(checked);
}

void OptionsWidget::on_allUserIsImportant_toggled(bool checked)
{
    emit sendAllUserIsImportant(checked);
}

void OptionsWidget::retranslate()
{
    ui->retranslateUi(this);
}

void OptionsWidget::on_Debug_toggled(bool checked)
{
    emit sendDebug(checked);
}

void OptionsWidget::on_atstartup_toggled(bool checked)
{
    emit sendAtStartup(checked);
}

void OptionsWidget::on_notunload_toggled(bool checked)
{
    emit sendNotUnload(checked);
}
