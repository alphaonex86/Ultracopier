#include "fileExistsDialog.h"
#include "ui_fileExistsDialog.h"

fileExistsDialog::fileExistsDialog() :
	ui(new Ui::fileExistsDialog)
{
	ui->setupUi(this);
	action=FileExists_Cancel;
	setWindowFlags((windowFlags()|Qt::WindowStaysOnTopHint));
}

fileExistsDialog::~fileExistsDialog()
{
	delete ui;
}

void fileExistsDialog::setInfo(QFileInfo source, QFileInfo destination)
{
	action=FileExists_Cancel;
	oldName=destination.fileName();
	ui->lineEditNewName->setText(oldName);
	ui->lineEditNewName->setPlaceholderText(oldName);
	ui->Overwrite->addAction(ui->actionOverwrite_if_newer);
	ui->Overwrite->addAction(ui->actionOverwrite_if_not_same_modification_date);
	ui->label_content_source_size->setText(QString::number(source.size()));
	ui->label_content_source_modified->setText(source.lastModified().toString());
	ui->label_content_source_file_name->setText(source.fileName());
	ui->label_content_destination_size->setText(QString::number(destination.size()));
	ui->label_content_destination_modified->setText(destination.lastModified().toString());
	ui->label_content_destination_file_name->setText(destination.fileName());
}

void fileExistsDialog::changeEvent(QEvent *e)
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

QString fileExistsDialog::getNewName()
{
	if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
		return "";
	else
		return ui->lineEditNewName->text();
}

void fileExistsDialog::on_SuggestNewName_clicked()
{
	ui->lineEditNewName->setText(tr("Copy of ")+oldName);
}

void fileExistsDialog::on_Rename_clicked()
{
	action=FileExists_Rename;
	this->close();
}

void fileExistsDialog::on_Overwrite_clicked()
{
	action=FileExists_Overwrite;
	this->close();
}

void fileExistsDialog::on_Skip_clicked()
{
	action=FileExists_Skip;
	this->close();
}

void fileExistsDialog::on_Cancel_clicked()
{
	action=FileExists_Cancel;
	this->close();
}

void fileExistsDialog::on_actionOverwrite_if_newer_triggered()
{
	action=FileExists_OverwriteIfNewer;
	this->close();
}

void fileExistsDialog::on_actionOverwrite_if_not_same_modification_date_triggered()
{
	action=FileExists_OverwriteIfNotSameModificationDate;
	this->close();
}

FileExistsAction fileExistsDialog::getAction()
{
	return action;
}

bool fileExistsDialog::getAlways()
{
	return ui->checkBoxAlways->isChecked();
}

