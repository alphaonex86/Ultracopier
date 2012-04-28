#ifndef FILTERS_H
#define FILTERS_H

#include <QDialog>

namespace Ui {
class Filters;
}

class Filters : public QDialog
{
	Q_OBJECT
	
public:
	explicit Filters(QWidget *parent = 0);
	~Filters();
	
private:
	Ui::Filters *ui;
};

#endif // FILTERS_H
