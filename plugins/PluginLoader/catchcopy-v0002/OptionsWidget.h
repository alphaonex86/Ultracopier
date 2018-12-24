#ifndef OptionsWidget_H
#define OptionsWidget_H

#include <QWidget>

namespace Ui {
class OptionsWidget;
}

class OptionsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsWidget(QWidget *parent = 0);
    ~OptionsWidget();
    void setAllDllIsImportant(bool allDllIsImportant);
    void setAllUserIsImportant(bool allDllIsImportant);
    void setDebug(bool Debug);
    void retranslate();
private:
    Ui::OptionsWidget *ui;
signals:
    void sendAllDllIsImportant(bool allDllIsImportant);
    void sendAllUserIsImportant(bool allDllIsImportant);
    void sendDebug(bool Debug);
private slots:
    void on_allDllIsImportant_toggled(bool checked);
    void on_allUserIsImportant_toggled(bool checked);
    void on_Debug_toggled(bool checked);
};

#endif // OptionsWidget_H
