#include "folderExistsDialog.h"
#include "ui_folderExistsDialog.h"

#include <QMessageBox>

folderExistsDialog::folderExistsDialog(QWidget *parent,QFileInfo source,bool isSame,QFileInfo destination,QString firstRenamingRule,QString otherRenamingRule) :
	QDialog(parent),
	ui(new Ui::folderExistsDialog)
{
	ui->setupUi(this);
	action=FolderExists_Cancel;
	oldName=source.fileName();
	this->destinationInfo=destinationInfo;
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
	this->firstRenamingRule=firstRenamingRule;
	this->otherRenamingRule=otherRenamingRule;
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
	QFileInfo destinationInfo=this->destinationInfo;
	QString absolutePath=destinationInfo.absolutePath();
	QString fileName=destinationInfo.fileName();
	QString suffix="";
	QString destination;
	QString newFileName;
	//resolv the suffix
	if(fileName.contains(QRegExp("^(.*)(\\.[a-z0-9]+)$")))
	{
		suffix=fileName;
		suffix.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\2");
		fileName.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\1");
	}
	//resolv the new name
	int num=1;
	do
	{
		if(num==1)
		{
			if(firstRenamingRule=="")
				newFileName=tr("%1 - copy").arg(fileName);
			else
			{
				newFileName=firstRenamingRule;
				newFileName.replace("%name%",fileName);
			}
		}
		else
		{
			if(otherRenamingRule=="")
				newFileName=tr("%1 - copy (%2)").arg(fileName).arg(num);
			else
			{
				newFileName=otherRenamingRule;
				newFileName.replace("%name%",fileName);
				newFileName.replace("%number%",QString::number(num));
			}
		}
		destination=absolutePath+QDir::separator()+newFileName+suffix;
		destinationInfo.setFile(destination);
	}
	while(destinationInfo.exists());
	ui->lineEditNewName->setText(fileName+suffix);
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
