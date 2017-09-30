#ifndef FILTERRULES_H
#define FILTERRULES_H

#include <QDialog>
#include <QAbstractButton>
#include <QPushButton>

#include "StructEnumDefinition_CopyEngine.h"

namespace Ui {
class FilterRules;
}

/** All the filter rules to include/exclude some file during the listing */
class FilterRules : public QDialog
{
    Q_OBJECT

public:
    explicit FilterRules(QWidget *parent = 0);
    ~FilterRules();
    bool getIsValid();
    std::string get_search_text();
    SearchType get_search_type();
    ApplyOn get_apply_on();
    bool get_need_match_all();
    void set_search_text(std::string search_text);
    void set_search_type(SearchType search_type);
    void set_apply_on(ApplyOn apply_on);
    void set_need_match_all(bool need_match_all);
private slots:
    void on_search_textChanged(const std::string &arg1);
    void on_isValid_clicked();
    void on_testString_textChanged(const std::string &arg1);
    void on_matched_clicked();
    void on_search_type_currentIndexChanged(int index);
    void on_need_match_all_clicked();
    void on_buttonBox_clicked(QAbstractButton *button);
private:
    Ui::FilterRules *ui;
    void updateChecking();
    bool isValid;
    bool haveBeenValided;
};

#endif // FILTERRULES_H
