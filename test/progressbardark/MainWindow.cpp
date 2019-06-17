#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <stdio.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    progressBarDark(new ProgressBarDark)
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(progressBarDark);
    connect(&treeTimer,&QTimer::timeout,this,&MainWindow::create);
    treeTimer.start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::create()
{
    int val=progressBarDark->value();
    if(val>=100)
        val=0;
    else
        val++;
    progressBarDark->setValue(val);
}
