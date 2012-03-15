/********************************************************************************
** Form generated from reading UI file 'fileErrorDialog.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILEERRORDIALOG_H
#define UI_FILEERRORDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_fileErrorDialog
{
public:
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QLabel *label_error;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_2;
    QFormLayout *formLayout_2;
    QLabel *label_size;
    QLabel *label_content_size;
    QLabel *label_modified;
    QLabel *label_content_modified;
    QLabel *label_file_name;
    QLabel *label_content_file_name;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkBoxAlways;
    QSpacerItem *horizontalSpacer;
    QPushButton *PutToBottom;
    QPushButton *Retry;
    QPushButton *Skip;
    QPushButton *Cancel;

    void setupUi(QWidget *fileErrorDialog)
    {
        if (fileErrorDialog->objectName().isEmpty())
            fileErrorDialog->setObjectName(QString::fromUtf8("fileErrorDialog"));
        fileErrorDialog->resize(478, 154);
        verticalLayout_2 = new QVBoxLayout(fileErrorDialog);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        groupBox = new QGroupBox(fileErrorDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label_error = new QLabel(groupBox);
        label_error->setObjectName(QString::fromUtf8("label_error"));

        verticalLayout->addWidget(label_error);


        verticalLayout_2->addWidget(groupBox);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label_size = new QLabel(fileErrorDialog);
        label_size->setObjectName(QString::fromUtf8("label_size"));
        label_size->setEnabled(false);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_size);

        label_content_size = new QLabel(fileErrorDialog);
        label_content_size->setObjectName(QString::fromUtf8("label_content_size"));
        label_content_size->setText(QString::fromUtf8("0 KiB"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, label_content_size);

        label_modified = new QLabel(fileErrorDialog);
        label_modified->setObjectName(QString::fromUtf8("label_modified"));
        label_modified->setEnabled(false);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_modified);

        label_content_modified = new QLabel(fileErrorDialog);
        label_content_modified->setObjectName(QString::fromUtf8("label_content_modified"));
        label_content_modified->setText(QString::fromUtf8("Today"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, label_content_modified);

        label_file_name = new QLabel(fileErrorDialog);
        label_file_name->setObjectName(QString::fromUtf8("label_file_name"));
        label_file_name->setEnabled(false);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_file_name);

        label_content_file_name = new QLabel(fileErrorDialog);
        label_content_file_name->setObjectName(QString::fromUtf8("label_content_file_name"));
        label_content_file_name->setText(QString::fromUtf8("source.txt"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, label_content_file_name);


        horizontalLayout->addLayout(formLayout_2);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        checkBoxAlways = new QCheckBox(fileErrorDialog);
        checkBoxAlways->setObjectName(QString::fromUtf8("checkBoxAlways"));

        horizontalLayout_4->addWidget(checkBoxAlways);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        PutToBottom = new QPushButton(fileErrorDialog);
        PutToBottom->setObjectName(QString::fromUtf8("PutToBottom"));

        horizontalLayout_4->addWidget(PutToBottom);

        Retry = new QPushButton(fileErrorDialog);
        Retry->setObjectName(QString::fromUtf8("Retry"));

        horizontalLayout_4->addWidget(Retry);

        Skip = new QPushButton(fileErrorDialog);
        Skip->setObjectName(QString::fromUtf8("Skip"));

        horizontalLayout_4->addWidget(Skip);

        Cancel = new QPushButton(fileErrorDialog);
        Cancel->setObjectName(QString::fromUtf8("Cancel"));

        horizontalLayout_4->addWidget(Cancel);


        verticalLayout_2->addLayout(horizontalLayout_4);


        retranslateUi(fileErrorDialog);
        QObject::connect(checkBoxAlways, SIGNAL(toggled(bool)), Cancel, SLOT(setDisabled(bool)));
        QObject::connect(checkBoxAlways, SIGNAL(toggled(bool)), Retry, SLOT(setDisabled(bool)));

        QMetaObject::connectSlotsByName(fileErrorDialog);
    } // setupUi

    void retranslateUi(QWidget *fileErrorDialog)
    {
        fileErrorDialog->setWindowTitle(QApplication::translate("fileErrorDialog", "Error on file", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("fileErrorDialog", "Error", 0, QApplication::UnicodeUTF8));
        label_error->setText(QString());
        label_size->setText(QApplication::translate("fileErrorDialog", "Size", 0, QApplication::UnicodeUTF8));
        label_modified->setText(QApplication::translate("fileErrorDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_file_name->setText(QApplication::translate("fileErrorDialog", "File name", 0, QApplication::UnicodeUTF8));
        checkBoxAlways->setText(QApplication::translate("fileErrorDialog", "&Always do this action", 0, QApplication::UnicodeUTF8));
        PutToBottom->setText(QApplication::translate("fileErrorDialog", "Put to bottom", 0, QApplication::UnicodeUTF8));
        Retry->setText(QApplication::translate("fileErrorDialog", "Retry", 0, QApplication::UnicodeUTF8));
        Skip->setText(QApplication::translate("fileErrorDialog", "&Skip", 0, QApplication::UnicodeUTF8));
        Cancel->setText(QApplication::translate("fileErrorDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class fileErrorDialog: public Ui_fileErrorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILEERRORDIALOG_H
