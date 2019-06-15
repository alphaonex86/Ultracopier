#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_map(new ChartArea::Widget(this))
{
    ui->setupUi(this);
    //m_map->hide();
    ui->verticalLayout->addWidget(m_map);

    connect(&timer,&QTimer::timeout,this,&MainWindow::addValue);
    timer.start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addValue()
{
    m_map->addValue(rand()%5000);
}
