#include "FileErrorDialog.h"
#include "ui_fileErrorDialog.h"
#include "TransferThread.h"

bool FileErrorDialog::isInAdmin=false;

FileErrorDialog::FileErrorDialog(QWidget *parent, QFileInfo fileInfo, QString errorString, const ErrorType &errorType) :
    QDialog(parent),
    ui(new Ui::fileErrorDialog)
{
    ui->setupUi(this);
    action=FileError_Cancel;
    ui->label_error->setText(errorString);
    if(fileInfo.exists())
    {
        ui->label_content_file_name->setText(TransferThread::resolvedName(fileInfo));
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(fileInfo.absoluteFilePath());
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
            ui->label_content_folder->setText(fileInfo.absolutePath());
        ui->label_content_size->setText(QString::number(fileInfo.size()));
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
        if(fileInfo.isDir())
        {
            this->setWindowTitle(tr("Error on folder"));
            ui->label_size->hide();
            ui->label_content_size->hide();
            ui->label_file_name->setText(tr("Folder name"));
        }
        ui->label_file_destination->setVisible(fileInfo.isSymLink());
        ui->label_content_file_destination->setVisible(fileInfo.isSymLink());
        if(fileInfo.isSymLink())
            ui->label_content_file_destination->setText(fileInfo.symLinkTarget());
    }
    else
    {
        ui->label_content_file_name->setText(TransferThread::resolvedName(fileInfo));
        if(ui->label_content_file_name->text().isEmpty())
        {
            ui->label_content_file_name->setText(fileInfo.absoluteFilePath());
            ui->label_folder->setVisible(false);
            ui->label_content_folder->setVisible(false);
        }
        else
            ui->label_content_folder->setText(fileInfo.absolutePath());

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

    #ifdef ULTRACOPIER_PLUGIN_RIGHTS
    ui->Rights->hide();
    if(isInAdmin)
        ui->Rights->hide();
    #ifdef Q_OS_WIN32
    if(errorType!=ErrorType_Rights)
        ui->Rights->hide();
    #else
    ui->Rights->hide();
    #endif
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
