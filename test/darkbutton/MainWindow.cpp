#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <stdio.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    darkButton(new DarkButton)
{
    ui->setupUi(this);

    darkButton->setText("toto");
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/cancelDarkD.png"), QSize(), QIcon::Normal, QIcon::Off);
    icon.addFile(QString::fromUtf8(":/cancelDarkE.png"), QSize(), QIcon::Normal, QIcon::On);
    darkButton->setIcon(icon);
    darkButton->setCheckable(true);

    ui->verticalLayout->addWidget(darkButton);
    connect(&timer,&QTimer::timeout,this,&MainWindow::create);
    timer.start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::create()
{
    if(darkButton->isChecked())
    {
        darkButton->setEnabled(!darkButton->isEnabled());
        darkButton->setChecked(false);
    }
    else
        darkButton->setChecked(true);
}
