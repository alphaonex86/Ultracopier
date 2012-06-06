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
	delete ui;
}

void OptionsWidget::setAllDllIsImportant(bool allDllIsImportant)
{
	ui->allDllIsImportant->setChecked(allDllIsImportant);
}

void OptionsWidget::on_allDllIsImportant_toggled(bool checked)
{
	emit sendAllDllIsImportant(ui->allDllIsImportant->isChecked());
}

void OptionsWidget::retranslate()
{
	ui->retranslateUi(this);
}
