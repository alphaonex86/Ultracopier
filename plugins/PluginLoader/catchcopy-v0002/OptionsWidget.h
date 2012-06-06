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
	void retranslate();
private:
	Ui::OptionsWidget *ui;
signals:
	void sendAllDllIsImportant(bool allDllIsImportant);
private slots:
	void on_allDllIsImportant_toggled(bool checked);
};

#endif // OptionsWidget_H
