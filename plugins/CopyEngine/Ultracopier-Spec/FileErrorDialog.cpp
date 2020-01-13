#include "FileErrorDialog.h"
#include "ui_fileErrorDialog.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#include <QString>

bool FileErrorDialog::isInAdmin=false;

FileErrorDialog::FileErrorDialog(QWidget *parent, INTERNALTYPEPATH fileInfo, std::string errorString, const ErrorType &errorType,FacilityInterface * facilityEngine) :
    QDialog(parent),
    ui(new Ui::fileErrorDialog)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    action=FileError_Cancel;
    ui->label_error->setText(QString::fromStdString(errorString));
#ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfoW;
    if(GetFileAttributesExW(fileInfo.c_str(),GetFileExInfoStandard,&fileInfoW))
    {
        uint64_t mdate=fileInfoW.ftLastWriteTime.dwHighDateTime;
        mdate<<=32;
        mdate|=fileInfoW.ftLastWriteTime.dwLowDateTime;
        uint64_t size=fileInfoW.nFileSizeHigh;
        size<<=32;
        size|=fileInfoW.nFileSizeLow;
#else
    struct stat p_statbuf;
    if(stat(TransferThread::internalStringTostring(fileInfo).c_str(), &p_statbuf)==0)
    {
        #ifdef Q_OS_UNIX
            #ifdef Q_OS_MAC
            uint64_t mdate=p_statbuf.st_mtimespec.tv_sec;
            #else
            uint64_t mdate=*reinterpret_cast<int64_t*>(&p_statbuf.st_mtim);
            #endif
        #else
        uint64_t mdate=*reinterpret_cast<int64_t*>(&p_statbuf.st_mtime);
        #endif
        const uint64_t size=p_statbuf.st_size;
#endif
        ui->label_content_file_name->setText(
                    QString::fromStdString(
                        TransferThread::resolvedName(
                            TransferThread::internalStringTostring(fileInfo)
                            )
                        )
                    );
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(QString::fromStdString(TransferThread::internalStringTostring(fileInfo)));
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
        {
            std::string folder=TransferThread::internalStringTostring(fileInfo);
            if(folder.size()>80)
                folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
            ui->label_content_folder->setText(QString::fromStdString(FSabsolutePath(TransferThread::internalStringTostring(fileInfo))));
        }
        ui->label_content_size->setText(QString::fromStdString(facilityEngine->sizeToString(size)));
        if(ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS<mdate)
        {
            ui->label_modified->setVisible(true);
            ui->label_content_modified->setVisible(true);
            ui->label_content_modified->setText(QDateTime::fromMSecsSinceEpoch(mdate*1000).toString());
        }
        else
        {
            ui->label_modified->setVisible(false);
            ui->label_content_modified->setVisible(false);
        }
        #ifdef Q_OS_WIN32
        if(fileInfoW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        #else
        if(S_ISDIR(p_statbuf.st_mode))
        #endif
        {
            this->setWindowTitle(tr("Error on folder"));
            ui->label_size->hide();
            ui->label_content_size->hide();
            ui->label_file_name->setText(tr("Folder name"));
        }
        #ifdef Q_OS_UNIX
        ui->label_file_destination->setVisible(p_statbuf.st_mode==S_IFLNK);
        ui->label_content_file_destination->setVisible(p_statbuf.st_mode==S_IFLNK);
        if(S_ISLNK(p_statbuf.st_mode))
        {
            char buf[1024];
            ssize_t len;
            if ((len = readlink(TransferThread::internalStringTostring(fileInfo).c_str(), buf, sizeof(buf)-1)) != -1)
            {
                buf[len] = '\0';
                ui->label_content_file_destination->setText(buf);
            }
        }
        #else
        ui->label_file_destination->setVisible(false);
        ui->label_content_file_destination->setVisible(false);
        #endif
    }
    else
    {
        ui->label_content_file_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(fileInfo))));
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(QString::fromStdString(TransferThread::internalStringTostring(fileInfo)));
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
            ui->label_content_folder->setText(QString::fromStdString(FSabsolutePath(TransferThread::internalStringTostring(fileInfo))));

        ui->label_file_destination->hide();
        ui->label_content_file_destination->hide();
        ui->label_size->hide();
        ui->label_content_size->hide();
        ui->label_modified->hide();
        ui->label_content_modified->hide();
    }
    if(errorType==ErrorType_Folder || errorType==ErrorType_FolderWithRety)
        ui->PutToBottom->hide();
    if(errorType==ErrorType_Folder)
        ui->Retry->hide();

    ui->Rights->hide();
    #ifdef ULTRACOPIER_PLUGIN_RIGHTS
        if(isInAdmin)
            ui->Rights->hide();
        #ifdef Q_OS_WIN32
        if(errorType!=ErrorType_Rights)
            ui->Rights->hide();
        #else
        ui->Rights->hide();
        #endif
    #else
        ui->Rights->hide();
    #endif
}

FileErrorDialog::~FileErrorDialog()
{
    delete ui;
}

void FileErrorDialog::changeEvent(QEvent *e)
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

void FileErrorDialog::on_PutToBottom_clicked()
{
    action=FileError_PutToEndOfTheList;
    this->close();
}

void FileErrorDialog::on_Retry_clicked()
{
    action=FileError_Retry;
    this->close();
}

void FileErrorDialog::on_Skip_clicked()
{
    action=FileError_Skip;
    this->close();
}

void FileErrorDialog::on_Cancel_clicked()
{
    action=FileError_Cancel;
    this->close();
}

bool FileErrorDialog::getAlways()
{
    return ui->checkBoxAlways->isChecked();
}

FileErrorAction FileErrorDialog::getAction()
{
    return action;
}

void FileErrorDialog::on_checkBoxAlways_clicked()
{
    ui->Rights->setEnabled(!ui->checkBoxAlways->isChecked());
}

#ifdef ULTRACOPIER_PLUGIN_RIGHTS
void FileErrorDialog::on_Rights_clicked()
{
}
#endif
