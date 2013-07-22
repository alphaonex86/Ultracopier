#ifndef FILTERS_H
#define FILTERS_H

#include <QDialog>
#include <QStringList>

#include "FilterRules.h"
#include "StructEnumDefinition_CopyEngine.h"

namespace Ui {
class Filters;
}

/** To add/edit one filter rules */
class Filters : public QDialog
{
    Q_OBJECT
public:
    explicit Filters(QWidget *parent = 0);
    ~Filters();
    void setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
    void reShowAll();
    QList<Filters_rules> getInclude() const;
    QList<Filters_rules> getExclude() const;
    void newLanguageLoaded();
private:
    Ui::Filters *ui;
    QList<Filters_rules> include;
    QList<Filters_rules> exclude;
    void updateFilters();
    bool convertToRegex(Filters_rules &item);
signals:
    void sendNewFilters(const QStringList &includeStrings,const QStringList &includeOptions,const QStringList &excludeStrings,const QStringList &excludeOptions) const;
    void haveNewFilters() const;
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
