/********************************************************************************
** Form generated from reading UI file 'folderExistsDialog.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FOLDEREXISTSDIALOG_H
#define UI_FOLDEREXISTSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_folderExistsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label_source;
    QLabel *label_destination;
    QLabel *label_message;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer;
    QFormLayout *formLayout;
    QLabel *label_source_modified;
    QLabel *label_content_source_modified;
    QLabel *label_source_folder_name;
    QLabel *label_content_source_folder_name;
    QSpacerItem *horizontalSpacer_2;
    QFormLayout *formLayout_2;
    QLabel *label_destination_modified;
    QLabel *label_destination_folder_name;
    QLabel *label_content_destination_modified;
    QLabel *label_content_destination_folder_name;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *lineEditNewName;
    QPushButton *SuggestNewName;
    QHBoxLayout *horizontalLayout_3;
    QCheckBox *checkBoxAlways;
    QPushButton *Rename;
    QPushButton *Merge;
    QPushButton *Skip;
    QPushButton *Cancel;

    void setupUi(QDialog *folderExistsDialog)
    {
        if (folderExistsDialog->objectName().isEmpty())
            folderExistsDialog->setObjectName(QString::fromUtf8("folderExistsDialog"));
        folderExistsDialog->resize(443, 146);
        verticalLayout = new QVBoxLayout(folderExistsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label_source = new QLabel(folderExistsDialog);
        label_source->setObjectName(QString::fromUtf8("label_source"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_source->sizePolicy().hasHeightForWidth());
        label_source->setSizePolicy(sizePolicy);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label_source->setFont(font);
        label_source->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label_source);

        label_destination = new QLabel(folderExistsDialog);
        label_destination->setObjectName(QString::fromUtf8("label_destination"));
        label_destination->setFont(font);
        label_destination->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label_destination);


        verticalLayout->addLayout(horizontalLayout);

        label_message = new QLabel(folderExistsDialog);
        label_message->setObjectName(QString::fromUtf8("label_message"));

        verticalLayout->addWidget(label_message);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label_source_modified = new QLabel(folderExistsDialog);
        label_source_modified->setObjectName(QString::fromUtf8("label_source_modified"));
        label_source_modified->setEnabled(false);

        formLayout->setWidget(0, QFormLayout::LabelRole, label_source_modified);

        label_content_source_modified = new QLabel(folderExistsDialog);
        label_content_source_modified->setObjectName(QString::fromUtf8("label_content_source_modified"));
        label_content_source_modified->setText(QString::fromUtf8("Today"));

        formLayout->setWidget(0, QFormLayout::FieldRole, label_content_source_modified);

        label_source_folder_name = new QLabel(folderExistsDialog);
        label_source_folder_name->setObjectName(QString::fromUtf8("label_source_folder_name"));
        label_source_folder_name->setEnabled(false);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_source_folder_name);

        label_content_source_folder_name = new QLabel(folderExistsDialog);
        label_content_source_folder_name->setObjectName(QString::fromUtf8("label_content_source_folder_name"));
        label_content_source_folder_name->setText(QString::fromUtf8("folder"));

        formLayout->setWidget(1, QFormLayout::FieldRole, label_content_source_folder_name);


        horizontalLayout_4->addLayout(formLayout);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);

        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label_destination_modified = new QLabel(folderExistsDialog);
        label_destination_modified->setObjectName(QString::fromUtf8("label_destination_modified"));
        label_destination_modified->setEnabled(false);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_destination_modified);

        label_destination_folder_name = new QLabel(folderExistsDialog);
        label_destination_folder_name->setObjectName(QString::fromUtf8("label_destination_folder_name"));
        label_destination_folder_name->setEnabled(false);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_destination_folder_name);

        label_content_destination_modified = new QLabel(folderExistsDialog);
        label_content_destination_modified->setObjectName(QString::fromUtf8("label_content_destination_modified"));
        label_content_destination_modified->setText(QString::fromUtf8("Today"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, label_content_destination_modified);

        label_content_destination_folder_name = new QLabel(folderExistsDialog);
        label_content_destination_folder_name->setObjectName(QString::fromUtf8("label_content_destination_folder_name"));
        label_content_destination_folder_name->setText(QString::fromUtf8("folder"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, label_content_destination_folder_name);


        horizontalLayout_4->addLayout(formLayout_2);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        lineEditNewName = new QLineEdit(folderExistsDialog);
        lineEditNewName->setObjectName(QString::fromUtf8("lineEditNewName"));
        lineEditNewName->setText(QString::fromUtf8(""));
        lineEditNewName->setPlaceholderText(QString::fromUtf8(""));

        horizontalLayout_2->addWidget(lineEditNewName);

        SuggestNewName = new QPushButton(folderExistsDialog);
        SuggestNewName->setObjectName(QString::fromUtf8("SuggestNewName"));

        horizontalLayout_2->addWidget(SuggestNewName);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        checkBoxAlways = new QCheckBox(folderExistsDialog);
        checkBoxAlways->setObjectName(QString::fromUtf8("checkBoxAlways"));

        horizontalLayout_3->addWidget(checkBoxAlways);

        Rename = new QPushButton(folderExistsDialog);
        Rename->setObjectName(QString::fromUtf8("Rename"));

        horizontalLayout_3->addWidget(Rename);

        Merge = new QPushButton(folderExistsDialog);
        Merge->setObjectName(QString::fromUtf8("Merge"));

        horizontalLayout_3->addWidget(Merge);

        Skip = new QPushButton(folderExistsDialog);
        Skip->setObjectName(QString::fromUtf8("Skip"));

        horizontalLayout_3->addWidget(Skip);

        Cancel = new QPushButton(folderExistsDialog);
        Cancel->setObjectName(QString::fromUtf8("Cancel"));

        horizontalLayout_3->addWidget(Cancel);


        verticalLayout->addLayout(horizontalLayout_3);


        retranslateUi(folderExistsDialog);
        QObject::connect(checkBoxAlways, SIGNAL(clicked(bool)), lineEditNewName, SLOT(setDisabled(bool)));
        QObject::connect(checkBoxAlways, SIGNAL(clicked(bool)), SuggestNewName, SLOT(setDisabled(bool)));

        QMetaObject::connectSlotsByName(folderExistsDialog);
    } // setupUi

    void retranslateUi(QDialog *folderExistsDialog)
    {
        folderExistsDialog->setWindowTitle(QApplication::translate("folderExistsDialog", "This folders are the same folder", 0, QApplication::UnicodeUTF8));
        label_source->setText(QApplication::translate("folderExistsDialog", "Source", 0, QApplication::UnicodeUTF8));
        label_destination->setText(QApplication::translate("folderExistsDialog", "Destination", 0, QApplication::UnicodeUTF8));
        label_message->setText(QApplication::translate("folderExistsDialog", "The source and destination is same", 0, QApplication::UnicodeUTF8));
        label_source_modified->setText(QApplication::translate("folderExistsDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_source_folder_name->setText(QApplication::translate("folderExistsDialog", "Folder name", 0, QApplication::UnicodeUTF8));
        label_destination_modified->setText(QApplication::translate("folderExistsDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_destination_folder_name->setText(QApplication::translate("folderExistsDialog", "Folder name", 0, QApplication::UnicodeUTF8));
        SuggestNewName->setText(QApplication::translate("folderExistsDialog", "Suggest new &name", 0, QApplication::UnicodeUTF8));
        checkBoxAlways->setText(QApplication::translate("folderExistsDialog", "&Always do this action", 0, QApplication::UnicodeUTF8));
        Rename->setText(QApplication::translate("folderExistsDialog", "&Rename", 0, QApplication::UnicodeUTF8));
        Merge->setText(QApplication::translate("folderExistsDialog", "Merge", 0, QApplication::UnicodeUTF8));
        Skip->setText(QApplication::translate("folderExistsDialog", "Skip", 0, QApplication::UnicodeUTF8));
        Cancel->setText(QApplication::translate("folderExistsDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class folderExistsDialog: public Ui_folderExistsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FOLDEREXISTSDIALOG_H
