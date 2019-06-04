#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "radialMap/widget.h"
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_map(new RadialMap::Widget(this,false)),
    tree(new Folder(""))
{
    recursiveTreeLoad(tree,"/etc/");

    ui->setupUi(this);
    //m_map->hide();
    ui->verticalLayout->addWidget(m_map);
    connect(&treeTimer,&QTimer::timeout,this,&MainWindow::create);
    treeTimer.setSingleShot(true);
    treeTimer.start(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::create()
{
    m_map->create(tree);
}

void MainWindow::recursiveTreeLoad(Folder * tree,QString folder)
{
    QDir dir(folder);
    QFileInfoList list=dir.entryInfoList();
    foreach (QFileInfo finfo, list) {
        const char * const c_name=finfo.fileName().toStdString().c_str();
        if(c_name[0]!=0x00 && c_name[0]!='.')
        {
            if(finfo.isDir())
            {
                Folder * newDir=new Folder(c_name);
                tree->append(newDir);
                recursiveTreeLoad(newDir,finfo.absoluteFilePath());
            }
            else
                tree->append(c_name,finfo.size());
        }
    }
}
