/********************************************************************************
** Form generated from reading UI file 'options.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPTIONS_H
#define UI_OPTIONS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_options
{
public:
    QFormLayout *formLayout;
    QLabel *label;
    QCheckBox *doRightTransfer;
    QLabel *label_2;
    QCheckBox *keepDate;
    QLabel *label_3;
    QCheckBox *prealloc;
    QLabel *label_4;
    QSpinBox *blockSize;
    QLabel *label_5;
    QCheckBox *autoStart;
    QLabel *label_6;
    QLabel *label_7;
    QComboBox *comboBoxFolderError;
    QComboBox *comboBoxFolderColision;
    QLabel *label_8;
    QCheckBox *checkBoxDestinationFolderExists;

    void setupUi(QWidget *options)
    {
        if (options->objectName().isEmpty())
            options->setObjectName(QString::fromUtf8("options"));
        options->resize(313, 212);
        formLayout = new QFormLayout(options);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        label = new QLabel(options);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        doRightTransfer = new QCheckBox(options);
        doRightTransfer->setObjectName(QString::fromUtf8("doRightTransfer"));

        formLayout->setWidget(0, QFormLayout::FieldRole, doRightTransfer);

        label_2 = new QLabel(options);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        keepDate = new QCheckBox(options);
        keepDate->setObjectName(QString::fromUtf8("keepDate"));

        formLayout->setWidget(1, QFormLayout::FieldRole, keepDate);

        label_3 = new QLabel(options);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        prealloc = new QCheckBox(options);
        prealloc->setObjectName(QString::fromUtf8("prealloc"));

        formLayout->setWidget(2, QFormLayout::FieldRole, prealloc);

        label_4 = new QLabel(options);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        blockSize = new QSpinBox(options);
        blockSize->setObjectName(QString::fromUtf8("blockSize"));
        blockSize->setMinimum(1);
        blockSize->setMaximum(64000);

        formLayout->setWidget(3, QFormLayout::FieldRole, blockSize);

        label_5 = new QLabel(options);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_5);

        autoStart = new QCheckBox(options);
        autoStart->setObjectName(QString::fromUtf8("autoStart"));

        formLayout->setWidget(4, QFormLayout::FieldRole, autoStart);

        label_6 = new QLabel(options);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_6);

        label_7 = new QLabel(options);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_7);

        comboBoxFolderError = new QComboBox(options);
        comboBoxFolderError->setObjectName(QString::fromUtf8("comboBoxFolderError"));

        formLayout->setWidget(5, QFormLayout::FieldRole, comboBoxFolderError);

        comboBoxFolderColision = new QComboBox(options);
        comboBoxFolderColision->setObjectName(QString::fromUtf8("comboBoxFolderColision"));

        formLayout->setWidget(6, QFormLayout::FieldRole, comboBoxFolderColision);

        label_8 = new QLabel(options);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_8);

        checkBoxDestinationFolderExists = new QCheckBox(options);
        checkBoxDestinationFolderExists->setObjectName(QString::fromUtf8("checkBoxDestinationFolderExists"));

        formLayout->setWidget(7, QFormLayout::FieldRole, checkBoxDestinationFolderExists);


        retranslateUi(options);

        QMetaObject::connectSlotsByName(options);
    } // setupUi

    void retranslateUi(QWidget *options)
    {
        label->setText(QApplication::translate("options", "Transfer the file rights", 0, QApplication::UnicodeUTF8));
        doRightTransfer->setText(QApplication::translate("options", "Transfer", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("options", "Keep the file date", 0, QApplication::UnicodeUTF8));
        keepDate->setText(QApplication::translate("options", "keep it", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("options", "Preallocate all file", 0, QApplication::UnicodeUTF8));
        prealloc->setText(QApplication::translate("options", "Preallocate", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("options", "Block size", 0, QApplication::UnicodeUTF8));
        blockSize->setSuffix(QApplication::translate("options", "KB", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("options", "Auto start the transfer", 0, QApplication::UnicodeUTF8));
        autoStart->setText(QApplication::translate("options", "Auto-Start", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("options", "When folder error", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("options", "When folder colision", 0, QApplication::UnicodeUTF8));
        comboBoxFolderError->clear();
        comboBoxFolderError->insertItems(0, QStringList()
         << QApplication::translate("options", "Ask", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("options", "Skip", 0, QApplication::UnicodeUTF8)
        );
        comboBoxFolderColision->clear();
        comboBoxFolderColision->insertItems(0, QStringList()
         << QApplication::translate("options", "Ask", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("options", "Merge", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("options", "Skip", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("options", "Rename", 0, QApplication::UnicodeUTF8)
        );
        label_8->setText(QApplication::translate("options", "Check if destination folder exists", 0, QApplication::UnicodeUTF8));
        checkBoxDestinationFolderExists->setText(QApplication::translate("options", "Check it", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(options);
    } // retranslateUi

};

namespace Ui {
    class options: public Ui_options {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONS_H
