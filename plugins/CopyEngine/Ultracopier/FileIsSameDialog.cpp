#include "FileIsSameDialog.h"
#include "ui_fileIsSameDialog.h"
#include "TransferThread.h"
#include "FacilityEngine.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QMessageBox>

FileIsSameDialog::FileIsSameDialog(QWidget *parent, QFileInfo fileInfo, std::string firstRenamingRule, std::string otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::fileIsSameDialog)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    action=FileExists_Cancel;
    oldName=TransferThread::resolvedName(fileInfo);
    destinationInfo=fileInfo;
    ui->lineEditNewName->setText(QString::fromStdString(oldName));
    ui->lineEditNewName->setPlaceholderText(QString::fromStdString(oldName));
    ui->label_content_size->setText(QString::number(fileInfo.size()));
    ui->label_content_modified->setText(fileInfo.lastModified().toString());
    ui->label_content_file_name->setText(QString::fromStdString(TransferThread::resolvedName(fileInfo)));
    QString folder=fileInfo.absolutePath();
    if(folder.size()>80)
        folder=folder.mid(0,38)+"..."+folder.mid(folder.size()-38);
    ui->label_content_folder->setText(folder);
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

std::string FileIsSameDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text().toStdString() || ui->checkBoxAlways->isChecked())
        return oldName;
    else
        return ui->lineEditNewName->text().toStdString();
}

void FileIsSameDialog::on_SuggestNewName_clicked()
{
    QFileInfo destinationInfo=this->destinationInfo;
    QString absolutePath=destinationInfo.absolutePath();
    QString fileName=QString::fromStdString(TransferThread::resolvedName(destinationInfo));
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
            if(firstRenamingRule.empty())
                newFileName=tr("%1 - copy").arg(fileName);
            else
            {
                newFileName=QString::fromStdString(firstRenamingRule);
                newFileName.replace(QStringLiteral("%name%"),fileName);
            }
        }
        else
        {
            if(otherRenamingRule.empty())
                newFileName=tr("%1 - copy (%2)").arg(fileName).arg(num);
            else
            {
                newFileName=QString::fromStdString(otherRenamingRule);
                newFileName.replace(QStringLiteral("%name%"),fileName);
                newFileName.replace(QStringLiteral("%number%"),QString::number(num));
            }
        }
        destination=absolutePath+QString::fromStdString(FacilityEngine::separator())+newFileName+suffix;
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
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text().toStdString() && !ui->lineEditNewName->text().isEmpty()));
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
        QMessageBox::warning(this,tr("Error"),tr("Try rename with using special characters"));
}

void FileIsSameDialog::on_lineEditNewName_editingFinished()
{
    updateRenameButton();
}
