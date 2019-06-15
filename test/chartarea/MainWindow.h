#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "widget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void addValue();
private:
    Ui::MainWindow *ui;
    ChartArea::Widget * m_map;
    QTimer timer;
};

#endif // MAINWINDOW_H
