#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "radialMap/widget.h"
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <stdio.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_map(new RadialMap::Widget(this)),
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

uint64_t MainWindow::recursiveTreeLoad(Folder * tree,std::string folder)
{
    uint64_t size=0;
    DIR *d;
    struct dirent *entry;
    d = opendir(folder.c_str());
    if (d)
    {
        while ((entry = readdir(d)) != NULL)
        {
            if(entry==NULL)
                break;
            bool skip=false;
            if(entry->d_name[0]=='.')
            {
                if(entry->d_name[1]==0x00)
                    skip=true;
                else if(entry->d_name[1]=='.' && entry->d_name[2]==0x00)
                    skip=true;
            }
            if(!skip)
            {

                if(entry->d_type == DT_DIR)
                {
                    Folder * newDir=new Folder(entry->d_name);
                    size+=recursiveTreeLoad(newDir,folder+entry->d_name+"/");
                    tree->append(newDir);
                }
                else
                {
                    struct stat statbuf;
                    std::string path=folder+entry->d_name;
                    if(stat(path.c_str(), &statbuf) != -1)
                    {
                        size+=statbuf.st_size;
                        tree->append(entry->d_name,statbuf.st_size);
                    }
                }
            }
        }
        closedir(d);
    }
    if(tree->size()!=size)//wrong is recursiveTreeLoad() is call after append
        abort();
    return size;
}
