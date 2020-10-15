#include "FolderExistsDialog.h"
#include "ui_folderExistsDialog.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"
#include <cstring>

#ifdef Q_OS_WIN32
#define CURRENTSEPARATOR "\\"
#else
#define CURRENTSEPARATOR "/"
#endif

#include <QMessageBox>

FolderExistsDialog::FolderExistsDialog(QWidget *parent, INTERNALTYPEPATH source, bool isSame, INTERNALTYPEPATH destination,
                                       std::string firstRenamingRule, std::string otherRenamingRule) :
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
    oldName=TransferThread::resolvedName(TransferThread::internalStringTostring(destination));
    ui->lineEditNewName->setText(QString::fromStdString(oldName));
    ui->lineEditNewName->setPlaceholderText(QString::fromStdString(oldName));
#ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfoW;
    if(GetFileAttributesExW(source.c_str(),GetFileExInfoStandard,&fileInfoW))
    {
        uint64_t mdate=fileInfoW.ftLastWriteTime.dwHighDateTime;
        mdate<<=32;
        mdate|=fileInfoW.ftLastWriteTime.dwLowDateTime;
        uint64_t size=fileInfoW.nFileSizeHigh;
        size<<=32;
        size|=fileInfoW.nFileSizeLow;
#else
    struct stat source_statbuf;
    memset(&source_statbuf,0,sizeof(source_statbuf));
    if (lstat(TransferThread::internalStringTostring(source).c_str(), &source_statbuf) < 0)
    {
        #ifdef Q_OS_UNIX
            #ifdef Q_OS_MAC
            const uint64_t mdate=source_statbuf.st_mtimespec.tv_sec;
            #else
            const uint64_t mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtim);
            #endif
        #else
        const uint64_t mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtime);
        #endif
#endif
        ui->label_content_source_modified->setText(QDateTime::fromMSecsSinceEpoch(mdate*1000).toString());
    }
    else
        ui->label_content_source_modified->hide();
    ui->label_content_source_folder_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(source))));
    std::string folder=TransferThread::internalStringTostring(FSabsolutePath(source));
    if(folder.size()>80)
        folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
    ui->label_content_source_folder->setText(QString::fromStdString(folder));
    if(ui->label_content_source_folder_name->text().isEmpty())
    {
        ui->label_source_folder_name->hide();
        ui->label_content_source_folder_name->hide();
    }
    if(isSame)
    {
        this->destinationInfo=TransferThread::internalStringTostring(source);
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
        this->destinationInfo=TransferThread::internalStringTostring(destination);
        this->setWindowTitle(tr("Folder already exists"));
        struct stat destination_statbuf;
        memset(&destination_statbuf,0,sizeof(destination_statbuf));
        if(TransferThread::exists(destination))
        {
            #ifdef Q_OS_UNIX
                #ifdef Q_OS_MAC
                const uint64_t mdate=destination_statbuf.st_mtimespec.tv_sec;
                #else
                const uint64_t mdate=*reinterpret_cast<int64_t*>(&destination_statbuf.st_mtim);
                #endif
            #else
            const uint64_t mdate=*reinterpret_cast<int64_t*>(&destination_statbuf.st_mtime);
            #endif
            ui->label_content_destination_modified->setText(QDateTime::fromMSecsSinceEpoch(mdate*1000).toString());
        }
        else
            ui->label_content_destination_modified->hide();
        ui->label_content_destination_folder_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(destination))));
        std::string folder=TransferThread::internalStringTostring(FSabsolutePath(destination));
        if(folder.size()>80)
            folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
        ui->label_content_destination_folder->setText(QString::fromStdString(folder));
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
    std::string destinationInfo=this->destinationInfo;
    QString absolutePath=QString::fromStdString(FSabsolutePath(destinationInfo));
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
                newFileName=tr("%name% - copy%suffix%");
            else
            {
                newFileName=QString::fromStdString(firstRenamingRule);
            }
        }
        else
        {
            if(otherRenamingRule.empty())
                newFileName=tr("%name% - copy (%number%)%suffix%");
            else
                newFileName=QString::fromStdString(otherRenamingRule);
            newFileName.replace(QStringLiteral("%number%"),QString::number(num));
        }
        newFileName.replace(QStringLiteral("%name%"),fileName);
        newFileName.replace(QStringLiteral("%suffix%"),suffix);
        destination=absolutePath;
        if(!destination.endsWith('/')
            #ifdef Q_OS_WIN32
                && !destination.endsWith('\\')
            #endif
                )
            destination+=CURRENTSEPARATOR;
        destination+=newFileName;
        destinationInfo=destination.toStdString();
        num++;
    }
    while(TransferThread::exists(destinationInfo.c_str()));
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

void FolderExistsDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FolderExistsDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text().toStdString() && !ui->lineEditNewName->text().isEmpty()));
}
