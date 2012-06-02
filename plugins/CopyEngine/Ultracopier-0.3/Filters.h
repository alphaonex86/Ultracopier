#ifndef FILTERS_H
#define FILTERS_H

#include <QDialog>
#include <QStringList>

#include "FilterRules.h"
#include "StructEnumDefinition_CopyEngine.h"

namespace Ui {
class Filters;
}

class Filters : public QDialog
{
	Q_OBJECT
public:
	explicit Filters(QWidget *parent = 0);
	~Filters();
	void setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
	void reShowAll();
	QList<Filters_rules> getInclude();
	QList<Filters_rules> getExclude();
	void newLanguageLoaded();
private:
	Ui::Filters *ui;
	QList<Filters_rules> include;
	QList<Filters_rules> exclude;
	void haveNewFilters();
	bool convertToRegex(Filters_rules &item);
signals:
	void sendNewFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
private slots:
	void on_remove_exclusion_clicked();
	void on_remove_inclusion_clicked();
	void on_add_exclusion_clicked();
	void on_buttonBox_clicked(QAbstractButton *button);
	void on_add_inclusion_clicked();
	void on_edit_exclusion_clicked();
	void on_edit_inclusion_clicked();
};

#endif // FILTERS_H
