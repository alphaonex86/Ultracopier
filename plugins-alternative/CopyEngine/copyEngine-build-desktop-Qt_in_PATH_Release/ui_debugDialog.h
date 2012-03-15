/********************************************************************************
** Form generated from reading UI file 'debugDialog.ui'
**
** Created: Wed Mar 7 11:14:42 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DEBUGDIALOG_H
#define UI_DEBUGDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QListWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_debugDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_4;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_3;
    QListWidget *writeThreadList;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout_2;
    QListWidget *tranferList;
    QHBoxLayout *horizontalLayout;
    QCheckBox *readStatus;
    QCheckBox *readBlocking;

    void setupUi(QWidget *debugDialog)
    {
        if (debugDialog->objectName().isEmpty())
            debugDialog->setObjectName(QString::fromUtf8("debugDialog"));
        debugDialog->resize(549, 368);
        debugDialog->setWindowTitle(QString::fromUtf8("Monitor"));
        verticalLayout = new QVBoxLayout(debugDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        groupBox = new QGroupBox(debugDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setTitle(QString::fromUtf8("Write thread"));
        horizontalLayout_3 = new QHBoxLayout(groupBox);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        writeThreadList = new QListWidget(groupBox);
        writeThreadList->setObjectName(QString::fromUtf8("writeThreadList"));

        horizontalLayout_3->addWidget(writeThreadList);


        horizontalLayout_4->addWidget(groupBox);

        groupBox_2 = new QGroupBox(debugDialog);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setTitle(QString::fromUtf8("Transfer list"));
        horizontalLayout_2 = new QHBoxLayout(groupBox_2);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        tranferList = new QListWidget(groupBox_2);
        tranferList->setObjectName(QString::fromUtf8("tranferList"));

        horizontalLayout_2->addWidget(tranferList);


        horizontalLayout_4->addWidget(groupBox_2);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        readStatus = new QCheckBox(debugDialog);
        readStatus->setObjectName(QString::fromUtf8("readStatus"));
        readStatus->setText(QString::fromUtf8("Read thread running"));

        horizontalLayout->addWidget(readStatus);

        readBlocking = new QCheckBox(debugDialog);
        readBlocking->setObjectName(QString::fromUtf8("readBlocking"));
        readBlocking->setText(QString::fromUtf8("Block to read acces"));

        horizontalLayout->addWidget(readBlocking);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(debugDialog);

        QMetaObject::connectSlotsByName(debugDialog);
    } // setupUi

    void retranslateUi(QWidget *debugDialog)
    {
        Q_UNUSED(debugDialog);
    } // retranslateUi

};

namespace Ui {
    class debugDialog: public Ui_debugDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DEBUGDIALOG_H
