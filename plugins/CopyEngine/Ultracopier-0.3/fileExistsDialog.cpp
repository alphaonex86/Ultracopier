#include "fileExistsDialog.h"
#include "ui_fileExistsDialog.h"

#include <QDebug>

fileExistsDialog::fileExistsDialog(QWidget *parent,QFileInfo source,QFileInfo destination) :
	QDialog(parent),
	ui(new Ui::fileExistsDialog)
{
	ui->setupUi(this);
	action=FileExists_Cancel;
	destinationInfo=destination;
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
	QDateTime maxTime(QDate(1990,1,1));
	if(maxTime<source.lastModified())
	{
		ui->label_source_modified->setVisible(true);
		ui->label_content_source_modified->setVisible(true);
		ui->label_content_source_modified->setText(source.lastModified().toString());
	}
	else
	{
		ui->label_source_modified->setVisible(false);
		ui->label_content_source_modified->setVisible(false);
	}
	if(maxTime<destination.lastModified())
	{
		ui->label_destination_modified->setVisible(true);
		ui->label_content_destination_modified->setVisible(true);
		ui->label_content_destination_modified->setText(destination.lastModified().toString());
	}
	else
	{
		ui->label_destination_modified->setVisible(false);
		ui->label_content_destination_modified->setVisible(false);
	}
}

fileExistsDialog::~fileExistsDialog()
{
	delete ui;
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
		qDebug() << "return the old name: "+oldName;
	else
		qDebug() << "return the new name: "+ui->lineEditNewName->text();
	if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
		return oldName;
	else
		return ui->lineEditNewName->text();
}

void fileExistsDialog::on_SuggestNewName_clicked()
{
	QFileInfo destinationInfo=this->destinationInfo;
	QString absolutePath=destinationInfo.absolutePath();
	QString fileName=destinationInfo.fileName();
	QString suffix="";
	QString destination;
	if(fileName.contains(QRegExp("^(.*)(\\.[a-z0-9]+)$")))
	{
		suffix=fileName;
		suffix.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\2");
		fileName.replace(QRegExp("^(.*)(\\.[a-z0-9]+)$"),"\\1");
	}
	do
	{
		if(!fileName.startsWith(tr("Copy of ")))
			fileName=tr("Copy of ")+fileName;
		else
		{
			if(fileName.contains(QRegExp("_[0-9]+$")))
			{
				QString number=fileName;
				number.replace(QRegExp("^.*_([0-9]+)$"),"\\1");
				int num=number.toInt()+1;
				fileName.remove(QRegExp("[0-9]+$"));
				fileName+=QString::number(num);
			}
			else
				fileName+="_2";
		}
		destination=absolutePath+QDir::separator()+fileName+suffix;
		destinationInfo.setFile(destination);
	}
	while(destinationInfo.exists());
	ui->lineEditNewName->setText(fileName+suffix);
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

void fileExistsDialog::updateRenameButton()
{
	ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (oldName!=ui->lineEditNewName->text() && !ui->lineEditNewName->text().isEmpty()));
}

void fileExistsDialog::on_checkBoxAlways_toggled(bool checked)
{
	Q_UNUSED(checked);
	updateRenameButton();
}

void fileExistsDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
	Q_UNUSED(arg1);
	updateRenameButton();
}
