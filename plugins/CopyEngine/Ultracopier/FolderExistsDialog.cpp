#include "FolderExistsDialog.h"
#include "ui_folderExistsDialog.h"
#include "TransferThread.h"

#include <QMessageBox>
#include <QFileInfo>
#include <QMessageBox>

FolderExistsDialog::FolderExistsDialog(QWidget *parent,QFileInfo source,bool isSame,QFileInfo destination,QString firstRenamingRule,QString otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::folderExistsDialog)
{
    ui->setupUi(this);
    action=FolderExists_Cancel;
    oldName=TransferThread::resolvedName(destination);
    ui->lineEditNewName->setText(oldName);
    ui->lineEditNewName->setPlaceholderText(oldName);
    ui->label_content_source_modified->setText(source.lastModified().toString());
    ui->label_content_source_folder_name->setText(source.fileName());
    ui->label_content_source_folder->setText(source.absolutePath());
    if(ui->label_content_source_folder_name->text().isEmpty())
    {
        ui->label_source_folder_name->hide();
        ui->label_content_source_folder_name->hide();
    }
    if(isSame)
    {
        this->destinationInfo=source;
        ui->label_source->hide();
        ui->label_destination->hide();
        ui->label_destination_modified->hide();
        ui->label_destination_folder_name->hide();
        ui->label_destination_folder->hide();
        ui->label_content_destination_modified->hide();
        ui->label_content_destination_folder_name->hide();
        ui->label_content_destination_folder->hide();
    }
    else
    {
        this->destinationInfo=destination;
        this->setWindowTitle(tr("Folder already exists"));
        ui->label_content_destination_modified->setText(destination.lastModified().toString());
        ui->label_content_destination_folder_name->setText(destination.fileName());
        ui->label_content_destination_folder->setText(destination.absolutePath());
        if(ui->label_content_destination_folder_name->text().isEmpty())
        {
            ui->label_destination_folder_name->hide();
            ui->label_content_destination_folder_name->hide();
        }
    }
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    on_SuggestNewName_clicked();
}

FolderExistsDialog::~FolderExistsDialog()
{
    delete ui;
}

void FolderExistsDialog::changeEvent(QEvent *e)
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

QString FolderExistsDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
        return "";
    else
        return ui->lineEditNewName->text();
}

void FolderExistsDialog::on_SuggestNewName_clicked()
{
    QFileInfo destinationInfo=this->destinationInfo;
    QString absolutePath=destinationInfo.absolutePath();
    QString fileName=TransferThread::resolvedName(destinationInfo);
    QString suffix="";
    QString destination;
    QString newFileName;
    //resolv the suffix
    if(fileName.contains(QRegularExpression("^(.*)(\\.[a-z0-9]+)$")))
    {
        suffix=fileName;
        suffix.replace(QRegularExpression("^(.*)(\\.[a-z0-9]+)$"),"\\2");
        fileName.replace(QRegularExpression("^(.*)(\\.[a-z0-9]+)$"),"\\1");
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
        num++;
    }
    while(destinationInfo.exists());
    ui->lineEditNewName->setText(newFileName+suffix);
}

void FolderExistsDialog::on_Rename_clicked()
{
    action=FolderExists_Rename;
    this->close();
}

void FolderExistsDialog::on_Skip_clicked()
{
    action=FolderExists_Skip;
    this->close();
}

void FolderExistsDialog::on_Cancel_clicked()
{
    action=FolderExists_Cancel;
    this->close();
}

FolderExistsAction FolderExistsDialog::getAction()
{
    return action;
}

bool FolderExistsDialog::getAlways()
{
    return ui->checkBoxAlways->isChecked();
}

void FolderExistsDialog::on_Merge_clicked()
{
    action=FolderExists_Merge;
    this->close();
}

void FolderExistsDialog::on_lineEditNewName_editingFinished()
{
    updateRenameButton();
}

void FolderExistsDialog::on_lineEditNewName_returnPressed()
{
    updateRenameButton();
    if(ui->Rename->isEnabled())
        on_Rename_clicked();
    else
        QMessageBox::warning(this,tr("Error"),tr("Try rename with unauthorized characters"));
}

void FolderExistsDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FolderExistsDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text() && !ui->lineEditNewName->text().isEmpty()));
}
