#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
    Ui::MainWindow *ui;
    RadialMap::Widget * m_map;
    Folder * tree;

    void recursiveTreeLoad(Folder * tree,QString folder);
};

#endif // MAINWINDOW_H
