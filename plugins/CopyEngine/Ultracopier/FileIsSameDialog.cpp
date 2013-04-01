#include "FileIsSameDialog.h"
#include "ui_fileIsSameDialog.h"
#include "TransferThread.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QMessageBox>

FileIsSameDialog::FileIsSameDialog(QWidget *parent,QFileInfo fileInfo,QString firstRenamingRule,QString otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::fileIsSameDialog)
{
    ui->setupUi(this);
    action=FileExists_Cancel;
    oldName=TransferThread::resolvedName(fileInfo);
    destinationInfo=fileInfo;
    ui->lineEditNewName->setText(oldName);
    ui->lineEditNewName->setPlaceholderText(oldName);
    ui->label_content_size->setText(QString::number(fileInfo.size()));
    ui->label_content_modified->setText(fileInfo.lastModified().toString());
    ui->label_content_file_name->setText(TransferThread::resolvedName(fileInfo));
    ui->label_content_folder->setText(fileInfo.absolutePath());
    updateRenameButton();
    QDateTime maxTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
    if(maxTime<fileInfo.lastModified())
    {
        ui->label_modified->setVisible(true);
        ui->label_content_modified->setVisible(true);
        ui->label_content_modified->setText(fileInfo.lastModified().toString());
    }
    else
    {
        ui->label_modified->setVisible(false);
        ui->label_content_modified->setVisible(false);
    }
    if(!fileInfo.exists())
    {
        ui->label_content_size->setVisible(false);
        ui->label_size->setVisible(false);
        ui->label_modified->setVisible(false);
        ui->label_content_modified->setVisible(false);
    }
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    on_SuggestNewName_clicked();
}

FileIsSameDialog::~FileIsSameDialog()
{
    delete ui;
}

void FileIsSameDialog::changeEvent(QEvent *e)
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

QString FileIsSameDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
        return oldName;
    else
        return ui->lineEditNewName->text();
}

void FileIsSameDialog::on_SuggestNewName_clicked()
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

void FileIsSameDialog::on_Rename_clicked()
{
    action=FileExists_Rename;
    this->close();
}

void FileIsSameDialog::on_Skip_clicked()
{
    action=FileExists_Skip;
    this->close();
}

void FileIsSameDialog::on_Cancel_clicked()
{
    action=FileExists_Cancel;
    this->close();
}

FileExistsAction FileIsSameDialog::getAction()
{
    return action;
}

bool FileIsSameDialog::getAlways()
{
    return ui->checkBoxAlways->isChecked();
}

void FileIsSameDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text() && !ui->lineEditNewName->text().isEmpty()));
}

void FileIsSameDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FileIsSameDialog::on_checkBoxAlways_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateRenameButton();
}

void FileIsSameDialog::on_lineEditNewName_returnPressed()
{
    updateRenameButton();
    if(ui->Rename->isEnabled())
        on_Rename_clicked();
    else
        QMessageBox::warning(this,tr("Error"),tr("Try rename with unauthorized characters"));
}

void FileIsSameDialog::on_lineEditNewName_editingFinished()
{
    updateRenameButton();
}
