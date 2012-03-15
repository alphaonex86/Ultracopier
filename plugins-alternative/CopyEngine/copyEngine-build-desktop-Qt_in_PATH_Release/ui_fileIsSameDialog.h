/********************************************************************************
** Form generated from reading UI file 'fileIsSameDialog.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILEISSAMEDIALOG_H
#define UI_FILEISSAMEDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_fileIsSameDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_3;
    QFormLayout *formLayout_2;
    QLabel *label_size;
    QLabel *label_content_size;
    QLabel *label_modified;
    QLabel *label_content_modified;
    QLabel *label_file_name;
    QLabel *label_content_file_name;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout_3;
    QLineEdit *lineEditNewName;
    QPushButton *SuggestNewName;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkBoxAlways;
    QSpacerItem *horizontalSpacer;
    QPushButton *Rename;
    QPushButton *Skip;
    QPushButton *Cancel;

    void setupUi(QWidget *fileIsSameDialog)
    {
        if (fileIsSameDialog->objectName().isEmpty())
            fileIsSameDialog->setObjectName(QString::fromUtf8("fileIsSameDialog"));
        fileIsSameDialog->resize(378, 148);
        verticalLayout = new QVBoxLayout(fileIsSameDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(fileIsSameDialog);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label_size = new QLabel(fileIsSameDialog);
        label_size->setObjectName(QString::fromUtf8("label_size"));
        label_size->setEnabled(false);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_size);

        label_content_size = new QLabel(fileIsSameDialog);
        label_content_size->setObjectName(QString::fromUtf8("label_content_size"));
        label_content_size->setText(QString::fromUtf8("0 KiB"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, label_content_size);

        label_modified = new QLabel(fileIsSameDialog);
        label_modified->setObjectName(QString::fromUtf8("label_modified"));
        label_modified->setEnabled(false);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_modified);

        label_content_modified = new QLabel(fileIsSameDialog);
        label_content_modified->setObjectName(QString::fromUtf8("label_content_modified"));
        label_content_modified->setText(QString::fromUtf8("Today"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, label_content_modified);

        label_file_name = new QLabel(fileIsSameDialog);
        label_file_name->setObjectName(QString::fromUtf8("label_file_name"));
        label_file_name->setEnabled(false);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_file_name);

        label_content_file_name = new QLabel(fileIsSameDialog);
        label_content_file_name->setObjectName(QString::fromUtf8("label_content_file_name"));
        label_content_file_name->setText(QString::fromUtf8("source.txt"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, label_content_file_name);


        horizontalLayout->addLayout(formLayout_2);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        lineEditNewName = new QLineEdit(fileIsSameDialog);
        lineEditNewName->setObjectName(QString::fromUtf8("lineEditNewName"));
        lineEditNewName->setText(QString::fromUtf8(""));
        lineEditNewName->setPlaceholderText(QString::fromUtf8(""));

        horizontalLayout_3->addWidget(lineEditNewName);

        SuggestNewName = new QPushButton(fileIsSameDialog);
        SuggestNewName->setObjectName(QString::fromUtf8("SuggestNewName"));

        horizontalLayout_3->addWidget(SuggestNewName);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        checkBoxAlways = new QCheckBox(fileIsSameDialog);
        checkBoxAlways->setObjectName(QString::fromUtf8("checkBoxAlways"));

        horizontalLayout_4->addWidget(checkBoxAlways);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        Rename = new QPushButton(fileIsSameDialog);
        Rename->setObjectName(QString::fromUtf8("Rename"));

        horizontalLayout_4->addWidget(Rename);

        Skip = new QPushButton(fileIsSameDialog);
        Skip->setObjectName(QString::fromUtf8("Skip"));

        horizontalLayout_4->addWidget(Skip);

        Cancel = new QPushButton(fileIsSameDialog);
        Cancel->setObjectName(QString::fromUtf8("Cancel"));

        horizontalLayout_4->addWidget(Cancel);


        verticalLayout->addLayout(horizontalLayout_4);


        retranslateUi(fileIsSameDialog);

        QMetaObject::connectSlotsByName(fileIsSameDialog);
    } // setupUi

    void retranslateUi(QWidget *fileIsSameDialog)
    {
        fileIsSameDialog->setWindowTitle(QApplication::translate("fileIsSameDialog", "This files are the same file", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("fileIsSameDialog", "The source and destination are same", 0, QApplication::UnicodeUTF8));
        label_size->setText(QApplication::translate("fileIsSameDialog", "Size", 0, QApplication::UnicodeUTF8));
        label_modified->setText(QApplication::translate("fileIsSameDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_file_name->setText(QApplication::translate("fileIsSameDialog", "File name", 0, QApplication::UnicodeUTF8));
        SuggestNewName->setText(QApplication::translate("fileIsSameDialog", "Suggest new &name", 0, QApplication::UnicodeUTF8));
        checkBoxAlways->setText(QApplication::translate("fileIsSameDialog", "&Always do this action", 0, QApplication::UnicodeUTF8));
        Rename->setText(QApplication::translate("fileIsSameDialog", "&Rename", 0, QApplication::UnicodeUTF8));
        Skip->setText(QApplication::translate("fileIsSameDialog", "&Skip", 0, QApplication::UnicodeUTF8));
        Cancel->setText(QApplication::translate("fileIsSameDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class fileIsSameDialog: public Ui_fileIsSameDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILEISSAMEDIALOG_H
