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
}

MainWindow::~MainWindow()
{
    delete ui;
}
