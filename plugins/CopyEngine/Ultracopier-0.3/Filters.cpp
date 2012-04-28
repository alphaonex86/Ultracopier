#include "Filters.h"
#include "ui_Filters.h"

Filters::Filters(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::Filters)
{
	ui->setupUi(this);
}

Filters::~Filters()
{
	delete ui;
}
