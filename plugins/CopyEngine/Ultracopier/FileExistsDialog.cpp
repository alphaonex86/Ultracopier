#include "FileExistsDialog.h"
#include "ui_fileExistsDialog.h"
#include "TransferThread.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QMessageBox>

FileExistsDialog::FileExistsDialog(QWidget *parent,QFileInfo source,QFileInfo destination,QString firstRenamingRule,QString otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::fileExistsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    action=FileExists_Cancel;
    destinationInfo=destination;
    oldName=TransferThread::resolvedName(destination);
    ui->lineEditNewName->setText(oldName);
    ui->lineEditNewName->setPlaceholderText(oldName);
    ui->Overwrite->addAction(ui->actionOverwrite_if_newer);
    ui->Overwrite->addAction(ui->actionOverwrite_if_not_same_modification_date);
    ui->label_content_source_size->setText(QString::number(source.size()));
    ui->label_content_source_modified->setText(source.lastModified().toString());
    ui->label_content_source_file_name->setText(TransferThread::resolvedName(source));
    QString folder=source.absolutePath();
    if(folder.size()>80)
        folder=folder.mid(0,38)+"..."+folder.mid(folder.size()-38);
    ui->label_content_source_folder->setText(folder);
    ui->label_content_destination_size->setText(QString::number(destination.size()));
    ui->label_content_destination_modified->setText(destination.lastModified().toString());
    ui->label_content_destination_file_name->setText(TransferThread::resolvedName(destination));
    folder=destination.absolutePath();
    if(folder.size()>80)
        folder=folder.mid(0,38)+"..."+folder.mid(folder.size()-38);
    ui->label_content_destination_folder->setText(folder);
    QDateTime maxTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
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
    if(!source.exists())
    {
        ui->label_content_source_size->setVisible(false);
        ui->label_source_size->setVisible(false);
        ui->label_source_modified->setVisible(false);
        ui->label_content_source_modified->setVisible(false);
    }
    if(!destination.exists())
    {
        ui->label_content_destination_size->setVisible(false);
        ui->label_destination_size->setVisible(false);
        ui->label_destination_modified->setVisible(false);
        ui->label_content_destination_modified->setVisible(false);
    }
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    on_SuggestNewName_clicked();
}

FileExistsDialog::~FileExistsDialog()
{
    delete ui;
}

void FileExistsDialog::changeEvent(QEvent *e)
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

QString FileExistsDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text() || ui->checkBoxAlways->isChecked())
        return oldName;
    else
        return ui->lineEditNewName->text();
}

void FileExistsDialog::on_SuggestNewName_clicked()
{
    QFileInfo destinationInfo=this->destinationInfo;
    QString absolutePath=destinationInfo.absolutePath();
    QString fileName=TransferThread::resolvedName(destinationInfo);
    QString suffix="";
    QString destination;
    QString newFileName;
    //resolv the suffix
    if(fileName.contains(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$"))))
    {
        suffix=fileName;
        suffix.replace(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$")),QStringLiteral("\\2"));
        fileName.replace(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$")),QStringLiteral("\\1"));
    }
    //resolv the new name
    int num=1;
    do
    {
        if(num==1)
        {
            if(firstRenamingRule==QStringLiteral(""))
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
                newFileName.replace(QStringLiteral("%name%"),fileName);
                newFileName.replace(QStringLiteral("%number%"),QString::number(num));
            }
        }
        destination=absolutePath+FacilityEngine::separator()+newFileName+suffix;
        destinationInfo.setFile(destination);
        num++;
    }
    while(destinationInfo.exists());
    ui->lineEditNewName->setText(newFileName+suffix);
}

void FileExistsDialog::on_Rename_clicked()
{
    action=FileExists_Rename;
    this->close();
}

void FileExistsDialog::on_Overwrite_clicked()
{
    action=FileExists_Overwrite;
    this->close();
}

void FileExistsDialog::on_Skip_clicked()
{
    action=FileExists_Skip;
    this->close();
}

void FileExistsDialog::on_Cancel_clicked()
{
    action=FileExists_Cancel;
    this->close();
}

void FileExistsDialog::on_actionOverwrite_if_newer_triggered()
{
    action=FileExists_OverwriteIfNewer;
    this->close();
}

void FileExistsDialog::on_actionOverwrite_if_not_same_modification_date_triggered()
{
    action=FileExists_OverwriteIfNotSame;
    this->close();
}

FileExistsAction FileExistsDialog::getAction()
{
    return action;
}

bool FileExistsDialog::getAlways()
{
    return ui->checkBoxAlways->isChecked();
}

void FileExistsDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text() && !ui->lineEditNewName->text().isEmpty()));
}

void FileExistsDialog::on_checkBoxAlways_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateRenameButton();
}

void FileExistsDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FileExistsDialog::on_lineEditNewName_returnPressed()
{
    updateRenameButton();
    if(ui->Rename->isEnabled())
        on_Rename_clicked();
    else
        QMessageBox::warning(this,tr("Error"),tr("Try rename with using special characters"));
}

void FileExistsDialog::on_actionOverwrite_if_older_triggered()
{
    action=FileExists_OverwriteIfOlder;
    this->close();
}

void FileExistsDialog::on_lineEditNewName_editingFinished()
{
    updateRenameButton();
}
