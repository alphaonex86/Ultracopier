#include "FilterRules.h"
#include "ui_FilterRules.h"

FilterRules::FilterRules(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::FilterRules)
{
	ui->setupUi(this);
}

FilterRules::~FilterRules()
{
	delete ui;
}
