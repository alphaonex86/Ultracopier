#ifndef RENAMINGRULES_H
#define RENAMINGRULES_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class RenamingRules;
}

/** Define rules for renaming */
class RenamingRules : public QDialog
{
    Q_OBJECT
public:
    explicit RenamingRules(QWidget *parent = 0);
    ~RenamingRules();
    void setRenamingRules(std::string firstRenamingRule, std::string otherRenamingRule);
    void newLanguageLoaded();
private:
    Ui::RenamingRules *ui;
    void connectUI();
    void disconnectUI();
    std::string firstRenamingRule;
    std::string otherRenamingRule;
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void firstRenamingRule_haveChanged();
    void otherRenamingRule_haveChanged();
signals:
    void sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule) const;
};

#endif // RENAMINGRULES_H
