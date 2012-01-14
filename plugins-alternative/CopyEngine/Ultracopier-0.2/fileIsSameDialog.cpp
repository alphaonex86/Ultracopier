#include "fileIsSameDialog.h"
#include "ui_fileIsSameDialog.h"

fileIsSameDialog::fileIsSameDialog() :
	ui(new Ui::fileIsSameDialog)
{
	ui->setupUi(this);
	action=FileExists_Cancel;
	setWindowFlags((windowFlags()|Qt::WindowStaysOnTopHint));
}

fileIsSameDialog::~fileIsSameDialog()
{
	delete ui;
}

void fileIsSameDialog::setInfo(QFileInfo fileInfo)
{
	action=FileExists_Cancel;
	oldName=fileInfo.fileName();
	ui->lineEditNewName->setText(oldName);
	ui->lineEditNewName->setPlaceholderText(oldName);
	ui->label_content_size->setText(QString::number(fileInfo.size()));
	ui->label_content_modified->setText(fileInfo.lastModified().toString());
	ui->label_content_file_name->setText(fileInfo.fileName());
}

void fileIsSameDialog::changeEvent(QEvent *e)
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

QString fileIsSameDialog::getNewName()
{
	if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
		return "";
	else
		return ui->lineEditNewName->text();
}

void fileIsSameDialog::on_SuggestNewName_clicked()
{
	ui->lineEditNewName->setText(tr("Copy of ")+oldName);
}

void fileIsSameDialog::on_Rename_clicked()
{
	action=FileExists_Rename;
	this->close();
}

void fileIsSameDialog::on_Skip_clicked()
{
	action=FileExists_Skip;
	this->close();
}

void fileIsSameDialog::on_Cancel_clicked()
{
	action=FileExists_Cancel;
	this->close();
}

FileExistsAction fileIsSameDialog::getAction()
{
	return action;
}

bool fileIsSameDialog::getAlways()
{
	return ui->checkBoxAlways->isChecked();
}
