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
	void setRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
	void newLanguageLoaded();
private:
	Ui::RenamingRules *ui;
	void connectUI();
	void disconnectUI();
	QString firstRenamingRule;
	QString otherRenamingRule;
private slots:
	void on_buttonBox_clicked(QAbstractButton *button);
	void firstRenamingRule_haveChanged();
	void otherRenamingRule_haveChanged();
signals:
	void sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
};

#endif // RENAMINGRULES_H
