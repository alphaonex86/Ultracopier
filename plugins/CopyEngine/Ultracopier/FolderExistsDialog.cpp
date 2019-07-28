#include "FolderExistsDialog.h"
#include "ui_folderExistsDialog.h"
#include "TransferThread.h"

#ifdef Q_OS_WIN32
#define CURRENTSEPARATOR "\\"
#else
#define CURRENTSEPARATOR "/"
#endif

#include <QMessageBox>
#include <QFileInfo>
#include <QMessageBox>

FolderExistsDialog::FolderExistsDialog(QWidget *parent, QFileInfo source, bool isSame, QFileInfo destination, std::string firstRenamingRule, std::string otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::folderExistsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    action=FolderExists_Cancel;
    oldName=TransferThread::resolvedName(destination);
    ui->lineEditNewName->setText(QString::fromStdString(oldName));
    ui->lineEditNewName->setPlaceholderText(QString::fromStdString(oldName));
    ui->label_content_source_modified->setText(source.lastModified().toString());
    ui->label_content_source_folder_name->setText(source.fileName());
    QString folder=source.absolutePath();
    if(folder.size()>80)
        folder=folder.mid(0,38)+"..."+folder.mid(folder.size()-38);
    ui->label_content_source_folder->setText(folder);
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
        QString folder=destination.absolutePath();
        if(folder.size()>80)
            folder=folder.mid(0,38)+"..."+folder.mid(folder.size()-38);
        ui->label_content_destination_folder->setText(folder);
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

std::string FolderExistsDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text().toStdString() || ui->checkBoxAlways->isChecked())
        return "";
    else
        return ui->lineEditNewName->text().toStdString();
}

void FolderExistsDialog::on_SuggestNewName_clicked()
{
    QFileInfo destinationInfo=this->destinationInfo;
    QString absolutePath=destinationInfo.absolutePath();
    QString fileName=QString::fromStdString(TransferThread::resolvedName(destinationInfo));
    QString suffix;
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
                newFileName=tr("%name% - copy");
            else
            {
                newFileName=QString::fromStdString(firstRenamingRule);
            }
        }
        else
        {
            if(otherRenamingRule.empty())
                newFileName=tr("%name% - copy (%number%)");
            else
                newFileName=QString::fromStdString(otherRenamingRule);
            newFileName.replace(QStringLiteral("%number%"),QString::number(num));
        }
        newFileName.replace(QStringLiteral("%name%"),fileName);
        destination=absolutePath+CURRENTSEPARATOR+newFileName+suffix;
        destinationInfo.setFile(destination);
        num++;
    }
    while(destinationInfo.exists());
    ui->lineEditNewName->setText(newFileName);
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
        QMessageBox::warning(this,tr("Error"),tr("Try rename with using special characters"));
}

void FolderExistsDialog::on_lineEditNewName_textChanged(const std::string &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FolderExistsDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text().toStdString() && !ui->lineEditNewName->text().isEmpty()));
}
