/********************************************************************************
** Form generated from reading UI file 'fileExistsDialog.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILEEXISTSDIALOG_H
#define UI_FILEEXISTSDIALOG_H

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
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_fileExistsDialog
{
public:
    QAction *actionOverwrite_if_newer;
    QAction *actionOverwrite_if_not_same_modification_date;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QFormLayout *formLayout_2;
    QLabel *label_source_size;
    QLabel *label_content_source_size;
    QLabel *label_source_modified;
    QLabel *label_content_source_modified;
    QLabel *label_source_file_name;
    QLabel *label_content_source_file_name;
    QSpacerItem *horizontalSpacer_4;
    QFormLayout *formLayout;
    QLabel *label_destination_size;
    QLabel *label_content_destination_size;
    QLabel *label_destination_modified;
    QLabel *label_content_destination_modified;
    QLabel *label_destination_file_name;
    QLabel *label_content_destination_file_name;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout_3;
    QLineEdit *lineEditNewName;
    QPushButton *SuggestNewName;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *checkBoxAlways;
    QSpacerItem *horizontalSpacer;
    QPushButton *Rename;
    QToolButton *Overwrite;
    QPushButton *Skip;
    QPushButton *Cancel;

    void setupUi(QWidget *fileExistsDialog)
    {
        if (fileExistsDialog->objectName().isEmpty())
            fileExistsDialog->setObjectName(QString::fromUtf8("fileExistsDialog"));
        fileExistsDialog->resize(469, 150);
        actionOverwrite_if_newer = new QAction(fileExistsDialog);
        actionOverwrite_if_newer->setObjectName(QString::fromUtf8("actionOverwrite_if_newer"));
        actionOverwrite_if_not_same_modification_date = new QAction(fileExistsDialog);
        actionOverwrite_if_not_same_modification_date->setObjectName(QString::fromUtf8("actionOverwrite_if_not_same_modification_date"));
        verticalLayout = new QVBoxLayout(fileExistsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(fileExistsDialog);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label);

        label_2 = new QLabel(fileExistsDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label_2);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label_source_size = new QLabel(fileExistsDialog);
        label_source_size->setObjectName(QString::fromUtf8("label_source_size"));
        label_source_size->setEnabled(false);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_source_size);

        label_content_source_size = new QLabel(fileExistsDialog);
        label_content_source_size->setObjectName(QString::fromUtf8("label_content_source_size"));
        label_content_source_size->setText(QString::fromUtf8("0 KiB"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, label_content_source_size);

        label_source_modified = new QLabel(fileExistsDialog);
        label_source_modified->setObjectName(QString::fromUtf8("label_source_modified"));
        label_source_modified->setEnabled(false);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_source_modified);

        label_content_source_modified = new QLabel(fileExistsDialog);
        label_content_source_modified->setObjectName(QString::fromUtf8("label_content_source_modified"));
        label_content_source_modified->setText(QString::fromUtf8("Today"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, label_content_source_modified);

        label_source_file_name = new QLabel(fileExistsDialog);
        label_source_file_name->setObjectName(QString::fromUtf8("label_source_file_name"));
        label_source_file_name->setEnabled(false);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_source_file_name);

        label_content_source_file_name = new QLabel(fileExistsDialog);
        label_content_source_file_name->setObjectName(QString::fromUtf8("label_content_source_file_name"));
        label_content_source_file_name->setText(QString::fromUtf8("source.txt"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, label_content_source_file_name);


        horizontalLayout_2->addLayout(formLayout_2);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_4);

        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        label_destination_size = new QLabel(fileExistsDialog);
        label_destination_size->setObjectName(QString::fromUtf8("label_destination_size"));
        label_destination_size->setEnabled(false);

        formLayout->setWidget(0, QFormLayout::LabelRole, label_destination_size);

        label_content_destination_size = new QLabel(fileExistsDialog);
        label_content_destination_size->setObjectName(QString::fromUtf8("label_content_destination_size"));
        label_content_destination_size->setText(QString::fromUtf8("0 KiB"));

        formLayout->setWidget(0, QFormLayout::FieldRole, label_content_destination_size);

        label_destination_modified = new QLabel(fileExistsDialog);
        label_destination_modified->setObjectName(QString::fromUtf8("label_destination_modified"));
        label_destination_modified->setEnabled(false);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_destination_modified);

        label_content_destination_modified = new QLabel(fileExistsDialog);
        label_content_destination_modified->setObjectName(QString::fromUtf8("label_content_destination_modified"));
        label_content_destination_modified->setText(QString::fromUtf8("Today"));

        formLayout->setWidget(1, QFormLayout::FieldRole, label_content_destination_modified);

        label_destination_file_name = new QLabel(fileExistsDialog);
        label_destination_file_name->setObjectName(QString::fromUtf8("label_destination_file_name"));
        label_destination_file_name->setEnabled(false);

        formLayout->setWidget(2, QFormLayout::LabelRole, label_destination_file_name);

        label_content_destination_file_name = new QLabel(fileExistsDialog);
        label_content_destination_file_name->setObjectName(QString::fromUtf8("label_content_destination_file_name"));
        label_content_destination_file_name->setText(QString::fromUtf8("destination.txt"));

        formLayout->setWidget(2, QFormLayout::FieldRole, label_content_destination_file_name);


        horizontalLayout_2->addLayout(formLayout);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        lineEditNewName = new QLineEdit(fileExistsDialog);
        lineEditNewName->setObjectName(QString::fromUtf8("lineEditNewName"));
        lineEditNewName->setText(QString::fromUtf8(""));
        lineEditNewName->setPlaceholderText(QString::fromUtf8(""));

        horizontalLayout_3->addWidget(lineEditNewName);

        SuggestNewName = new QPushButton(fileExistsDialog);
        SuggestNewName->setObjectName(QString::fromUtf8("SuggestNewName"));

        horizontalLayout_3->addWidget(SuggestNewName);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        checkBoxAlways = new QCheckBox(fileExistsDialog);
        checkBoxAlways->setObjectName(QString::fromUtf8("checkBoxAlways"));

        horizontalLayout_4->addWidget(checkBoxAlways);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        Rename = new QPushButton(fileExistsDialog);
        Rename->setObjectName(QString::fromUtf8("Rename"));

        horizontalLayout_4->addWidget(Rename);

        Overwrite = new QToolButton(fileExistsDialog);
        Overwrite->setObjectName(QString::fromUtf8("Overwrite"));
        Overwrite->setPopupMode(QToolButton::MenuButtonPopup);

        horizontalLayout_4->addWidget(Overwrite);

        Skip = new QPushButton(fileExistsDialog);
        Skip->setObjectName(QString::fromUtf8("Skip"));

        horizontalLayout_4->addWidget(Skip);

        Cancel = new QPushButton(fileExistsDialog);
        Cancel->setObjectName(QString::fromUtf8("Cancel"));

        horizontalLayout_4->addWidget(Cancel);


        verticalLayout->addLayout(horizontalLayout_4);


        retranslateUi(fileExistsDialog);
        QObject::connect(checkBoxAlways, SIGNAL(toggled(bool)), Cancel, SLOT(setDisabled(bool)));
        QObject::connect(checkBoxAlways, SIGNAL(toggled(bool)), lineEditNewName, SLOT(setDisabled(bool)));
        QObject::connect(checkBoxAlways, SIGNAL(toggled(bool)), SuggestNewName, SLOT(setDisabled(bool)));

        QMetaObject::connectSlotsByName(fileExistsDialog);
    } // setupUi

    void retranslateUi(QWidget *fileExistsDialog)
    {
        fileExistsDialog->setWindowTitle(QApplication::translate("fileExistsDialog", "The file exists", 0, QApplication::UnicodeUTF8));
        actionOverwrite_if_newer->setText(QApplication::translate("fileExistsDialog", "Overwrite if newer", 0, QApplication::UnicodeUTF8));
        actionOverwrite_if_not_same_modification_date->setText(QApplication::translate("fileExistsDialog", "Overwrite if not same modification date", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("fileExistsDialog", "Source", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("fileExistsDialog", "Destination", 0, QApplication::UnicodeUTF8));
        label_source_size->setText(QApplication::translate("fileExistsDialog", "Size", 0, QApplication::UnicodeUTF8));
        label_source_modified->setText(QApplication::translate("fileExistsDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_source_file_name->setText(QApplication::translate("fileExistsDialog", "File name", 0, QApplication::UnicodeUTF8));
        label_destination_size->setText(QApplication::translate("fileExistsDialog", "Size", 0, QApplication::UnicodeUTF8));
        label_destination_modified->setText(QApplication::translate("fileExistsDialog", "Modified", 0, QApplication::UnicodeUTF8));
        label_destination_file_name->setText(QApplication::translate("fileExistsDialog", "File name", 0, QApplication::UnicodeUTF8));
        SuggestNewName->setText(QApplication::translate("fileExistsDialog", "Suggest new &name", 0, QApplication::UnicodeUTF8));
        checkBoxAlways->setText(QApplication::translate("fileExistsDialog", "&Always do this action", 0, QApplication::UnicodeUTF8));
        Rename->setText(QApplication::translate("fileExistsDialog", "&Rename", 0, QApplication::UnicodeUTF8));
        Overwrite->setText(QApplication::translate("fileExistsDialog", "&Overwrite", 0, QApplication::UnicodeUTF8));
        Skip->setText(QApplication::translate("fileExistsDialog", "&Skip", 0, QApplication::UnicodeUTF8));
        Cancel->setText(QApplication::translate("fileExistsDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class fileExistsDialog: public Ui_fileExistsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILEEXISTSDIALOG_H
