#ifndef FILTERRULES_H
#define FILTERRULES_H

#include <QDialog>

namespace Ui {
class FilterRules;
}

class FilterRules : public QDialog
{
	Q_OBJECT
	
public:
	explicit FilterRules(QWidget *parent = 0);
	~FilterRules();
	
private:
	Ui::FilterRules *ui;
};

#endif // FILTERRULES_H
