#include "fileErrorDialog.h"
#include "ui_fileErrorDialog.h"

fileErrorDialog::fileErrorDialog() :
	ui(new Ui::fileErrorDialog)
{
	ui->setupUi(this);
	action=FileError_Cancel;
	setWindowFlags((windowFlags()|Qt::WindowStaysOnTopHint));
}

fileErrorDialog::~fileErrorDialog()
{
	delete ui;
}

void fileErrorDialog::setInfo(QFileInfo fileInfo,QString errorString,bool canPutAtTheEnd)
{
	action=FileError_Cancel;
	ui->PutToBottom->setEnabled(canPutAtTheEnd);
	ui->label_error->setText(errorString);
	ui->label_content_file_name->setText(fileInfo.fileName());
	ui->label_content_size->setText(QString::number(fileInfo.size()));
	ui->label_content_modified->setText(fileInfo.lastModified().toString());
}

void fileErrorDialog::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void fileErrorDialog::on_PutToBottom_clicked()
{
	action=FileError_PutToEndOfTheList;
	this->close();
}

void fileErrorDialog::on_Retry_clicked()
{
	action=FileError_Retry;
	this->close();
}

void fileErrorDialog::on_Skip_clicked()
{
	action=FileError_Skip;
	this->close();
}

void fileErrorDialog::on_Cancel_clicked()
{
	action=FileError_Cancel;
	this->close();
}

bool fileErrorDialog::getAlways()
{
	return ui->checkBoxAlways->isChecked();
}

FileErrorAction fileErrorDialog::getAction()
{
	return action;
}
