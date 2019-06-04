#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "radialMap/widget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void create();
private:
    Ui::MainWindow *ui;
    RadialMap::Widget * m_map;
    Folder * tree;
    QTimer treeTimer;

    void recursiveTreeLoad(Folder * tree,QString folder);
};

#endif // MAINWINDOW_H
