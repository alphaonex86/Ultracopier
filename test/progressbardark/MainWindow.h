#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ProgressBarDark.h"

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
    ProgressBarDark * progressBarDark;
    QTimer treeTimer;
};

#endif // MAINWINDOW_H
