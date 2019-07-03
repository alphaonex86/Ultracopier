#include "FileErrorDialog.h"
#include "ui_fileErrorDialog.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#include <QString>

bool FileErrorDialog::isInAdmin=false;

FileErrorDialog::FileErrorDialog(QWidget *parent, std::string fileInfo, std::string errorString, const ErrorType &errorType,FacilityInterface * facilityEngine) :
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
    struct stat p_statbuf;
    if(stat(fileInfo.c_str(), &p_statbuf)==0)
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
        ui->label_content_file_name->setText(QString::fromStdString(TransferThread::resolvedName(fileInfo)));
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(QString::fromStdString(fileInfo));
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
        {
            std::string folder=fileInfo;
            if(folder.size()>80)
                folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
            ui->label_content_folder->setText(QString::fromStdString(FSabsolutePath(fileInfo)));
        }
        ui->label_content_size->setText(QString::fromStdString(facilityEngine->sizeToString(size)));
        if(ULTRACOPIER_PLUGIN_MINIMALYEAR_TIMESTAMPS<mdate)
        {
            ui->label_modified->setVisible(true);
            ui->label_content_modified->setVisible(true);
            ui->label_content_modified->setText(QDateTime::fromSecsSinceEpoch(mdate).toString());
        }
        else
        {
            ui->label_modified->setVisible(false);
            ui->label_content_modified->setVisible(false);
        }
        if(p_statbuf.st_mode==S_IFDIR)
        {
            this->setWindowTitle(tr("Error on folder"));
            ui->label_size->hide();
            ui->label_content_size->hide();
            ui->label_file_name->setText(tr("Folder name"));
        }
        #ifdef Q_OS_UNIX
        ui->label_file_destination->setVisible(p_statbuf.st_mode==S_IFLNK);
        ui->label_content_file_destination->setVisible(p_statbuf.st_mode==S_IFLNK);
        if(p_statbuf.st_mode==S_IFLNK)
        {
            char buf[1024];
            ssize_t len;
            if ((len = readlink(fileInfo.c_str(), buf, sizeof(buf)-1)) != -1)
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
        ui->label_content_file_name->setText(QString::fromStdString(TransferThread::resolvedName(fileInfo)));
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(QString::fromStdString(fileInfo));
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
            ui->label_content_folder->setText(QString::fromStdString(FSabsolutePath(fileInfo)));

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
