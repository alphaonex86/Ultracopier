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
    bool setFilters(std::vector<std::string> includeStrings, std::vector<std::string> includeOptions, std::vector<std::string> excludeStrings, std::vector<std::string> excludeOptions);
    void reShowAll();
    std::vector<Filters_rules> getInclude() const;
    std::vector<Filters_rules> getExclude() const;
    void newLanguageLoaded();
private:
    Ui::Filters *ui;
    std::vector<Filters_rules> include;
    std::vector<Filters_rules> exclude;
    void updateFilters();
    bool convertToRegex(Filters_rules &item);
signals:
    void sendNewFilters(const std::vector<std::string> &includeStrings,const std::vector<std::string> &includeOptions,const std::vector<std::string> &excludeStrings,const std::vector<std::string> &excludeOptions) const;
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
