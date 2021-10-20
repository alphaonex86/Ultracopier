#include "FileExistsDialog.h"
#include "ui_fileExistsDialog.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
#define CURRENTSEPARATOR "\\"
#else
#define CURRENTSEPARATOR "/"
#endif

#include <QMessageBox>

FileExistsDialog::FileExistsDialog(QWidget *parent, INTERNALTYPEPATH source,
                                   INTERNALTYPEPATH destination, std::string firstRenamingRule,
                                   std::string otherRenamingRule,FacilityInterface * facilityEngine) :
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
    destinationInfo=TransferThread::internalStringTostring(destination);
    oldName=TransferThread::resolvedName(TransferThread::internalStringTostring(destination));
    ui->lineEditNewName->setText(QString::fromStdString(oldName));
    ui->lineEditNewName->setPlaceholderText(QString::fromStdString(oldName));

    ui->Overwrite->addAction(ui->actionOverwrite_if_newer);
    ui->Overwrite->addAction(ui->actionOverwrite_if_older);
    ui->Overwrite->addAction(ui->actionOverwrite_if_not_same_modification_date);
    ui->Overwrite->addAction(ui->actionOverwrite_if_not_same_size);
    ui->Overwrite->addAction(ui->actionOverwrite_if_not_same_size_and_date);

    ui->label_content_source_file_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(source))));
    std::string folder=TransferThread::internalStringTostring(FSabsolutePath(source));
    if(folder.size()>80)
        folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
    ui->label_content_source_folder->setText(QString::fromStdString(folder));
    ui->label_content_destination_file_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(destination))));
    folder=TransferThread::internalStringTostring(FSabsolutePath(destination));
    if(folder.size()>80)
        folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
    ui->label_content_destination_folder->setText(QString::fromStdString(folder));
    //QDateTime maxTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
#ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA sourceW;
    if(GetFileAttributesExW(source.c_str(),GetFileExInfoStandard,&sourceW))
    {
        LARGE_INTEGER li;
        li.LowPart  = sourceW.ftLastWriteTime.dwLowDateTime;
        li.HighPart = sourceW.ftLastWriteTime.dwHighDateTime;
        const uint64_t mdate=(li.QuadPart - 0x019DB1DED53E8000) / 10000000;
        /*uint64_t mdate=sourceW.ftLastWriteTime.dwHighDateTime;
        mdate<<=32;
        mdate|=sourceW.ftLastWriteTime.dwLowDateTime;*/
        uint64_t size=sourceW.nFileSizeHigh;
        size<<=32;
        size|=sourceW.nFileSizeLow;
#else
    struct stat source_statbuf;
    memset(&source_statbuf,0,sizeof(source_statbuf));
    #ifdef Q_OS_UNIX
    if(lstat(TransferThread::internalStringTostring(source).c_str(), &source_statbuf)==0)
    #else
    if(stat(TransferThread::internalStringTostring(source).c_str(), &source_statbuf)==0)
    #endif
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
        const uint64_t size=source_statbuf.st_size;
#endif
        ui->label_source_modified->setVisible(true);
        ui->label_content_source_size->setVisible(true);
        ui->label_content_source_size->setText(QString::fromStdString(facilityEngine->sizeToString(size)));
        ui->label_content_source_modified->setVisible(true);
        ui->label_content_source_modified->setText(QDateTime::fromMSecsSinceEpoch(mdate*1000).toString());
    }
    else
    {
        ui->label_content_source_size->setVisible(false);
        ui->label_source_size->setVisible(false);
        ui->label_source_modified->setVisible(false);
        ui->label_content_source_modified->setVisible(false);
        ui->label_source_modified->setVisible(false);
        ui->label_content_source_modified->setVisible(false);
    }
#ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA destinationW;
    if(GetFileAttributesExW(destination.c_str(),GetFileExInfoStandard,&destinationW))
    {
        LARGE_INTEGER li;
        li.LowPart  = destinationW.ftLastWriteTime.dwLowDateTime;
        li.HighPart = destinationW.ftLastWriteTime.dwHighDateTime;
        const uint64_t mdate=(li.QuadPart - 0x019DB1DED53E8000) / 10000000;
        /*uint64_t mdate=destinationW.ftLastWriteTime.dwHighDateTime;
        mdate<<=32;
        mdate|=destinationW.ftLastWriteTime.dwLowDateTime;*/
        uint64_t size=destinationW.nFileSizeHigh;
        size<<=32;
        size|=destinationW.nFileSizeLow;
#else
    struct stat destination_statbuf;
    memset(&destination_statbuf,0,sizeof(destination_statbuf));
    #ifdef Q_OS_UNIX
    if(lstat(TransferThread::internalStringTostring(destination).c_str(), &destination_statbuf)==0)
    #else
    if(stat(TransferThread::internalStringTostring(destination).c_str(), &destination_statbuf)==0)
    #endif
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
        const uint64_t size=destination_statbuf.st_size;
#endif
        ui->label_destination_modified->setVisible(true);
        ui->label_content_destination_size->setVisible(true);
        ui->label_content_destination_size->setText(QString::fromStdString(facilityEngine->sizeToString(size)));
        ui->label_content_destination_modified->setVisible(true);
        ui->label_content_destination_modified->setText(QDateTime::fromMSecsSinceEpoch(mdate*1000).toString());
    }
    else
    {
        ui->label_destination_modified->setVisible(false);
        ui->label_content_destination_modified->setVisible(false);
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

std::string FileExistsDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text().toStdString() || ui->checkBoxAlways->isChecked())
        return oldName;
    else
        return ui->lineEditNewName->text().toStdString();
}

void FileExistsDialog::on_SuggestNewName_clicked()
{
    std::string destinationInfo=this->destinationInfo;
    QString absolutePath=QString::fromStdString(FSabsolutePath(destinationInfo));
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
                newFileName=tr("%name% - copy%suffix%");
            else
                newFileName=QString::fromStdString(firstRenamingRule);
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
    action=FileExists_OverwriteIfNotSameMdate;
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
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text().toStdString() && !ui->lineEditNewName->text().isEmpty()));
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

void FileExistsDialog::on_actionOverwrite_if_not_same_size_triggered()
{
    action=FileExists_OverwriteIfNotSameSize;
    this->close();
}

void FileExistsDialog::on_actionOverwrite_if_not_same_size_and_date_triggered()
{
    action=FileExists_OverwriteIfNotSameSizeAndDate;
    this->close();
}
