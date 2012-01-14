#include "folderExistsDialog.h"
#include "ui_folderExistsDialog.h"

#include <QMessageBox>

folderExistsDialog::folderExistsDialog(QWidget *parent,QFileInfo source,bool isSame,QFileInfo destination) :
	QDialog(parent),
	ui(new Ui::folderExistsDialog)
{
	ui->setupUi(this);
	action=FolderExists_Cancel;
	oldName=source.fileName();
	ui->lineEditNewName->setText(oldName);
	ui->lineEditNewName->setPlaceholderText(oldName);
	ui->label_content_source_modified->setText(source.lastModified().toString());
	ui->label_content_source_folder_name->setText(source.fileName());
	if(isSame)
	{
		ui->label_source->hide();
		ui->label_destination->hide();
		ui->label_destination_modified->hide();
		ui->label_destination_folder_name->hide();
		ui->label_content_destination_modified->hide();
		ui->label_content_destination_folder_name->hide();
	}
	else
	{
		ui->label_message->hide();
		ui->label_content_destination_modified->setText(destination.lastModified().toString());
		ui->label_content_destination_folder_name->setText(destination.fileName());
	}
}

folderExistsDialog::~folderExistsDialog()
{
	delete ui;
}

void folderExistsDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
		break;
		default:
		break;
	}
}

QString folderExistsDialog::getNewName()
{
	if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
		return "";
	else
		return ui->lineEditNewName->text();
}

void folderExistsDialog::on_SuggestNewName_clicked()
{
	ui->lineEditNewName->setText(tr("Copy of ")+oldName);
}

void folderExistsDialog::on_Rename_clicked()
{
	action=FolderExists_Rename;
	this->close();
}

void folderExistsDialog::on_Skip_clicked()
{
	action=FolderExists_Skip;
	this->close();
}

void folderExistsDialog::on_Cancel_clicked()
{
	action=FolderExists_Cancel;
	this->close();
}

FolderExistsAction folderExistsDialog::getAction()
{
	return action;
}

bool folderExistsDialog::getAlways()
{
	return ui->checkBoxAlways->isChecked();
}

void folderExistsDialog::on_Merge_clicked()
{
	action=FolderExists_Merge;
	this->close();
}
