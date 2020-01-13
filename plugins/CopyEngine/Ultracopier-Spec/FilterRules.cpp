#include "FilterRules.h"
#include "ui_FilterRules.h"

#include <QRegularExpression>

FilterRules::FilterRules(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::FilterRules)
{
    ui->setupUi(this);
    updateChecking();
    haveBeenValided=false;
}

FilterRules::~FilterRules()
{
    delete ui;
}

bool FilterRules::getIsValid()
{
    return isValid && haveBeenValided;
}

std::string FilterRules::get_search_text()
{
    return ui->search->text().toStdString();
}

SearchType FilterRules::get_search_type()
{
    switch(ui->search_type->currentIndex())
    {
        case 0:
            return SearchType_rawText;
        case 1:
            return SearchType_simpleRegex;
        case 2:
            return SearchType_perlRegex;
    }
    return SearchType_simpleRegex;
}

ApplyOn FilterRules::get_apply_on()
{
    switch(ui->apply_on->currentIndex())
    {
        case 0:
            return ApplyOn_file;
        case 1:
            return ApplyOn_fileAndFolder;
        case 2:
            return ApplyOn_folder;
    }
    return ApplyOn_fileAndFolder;
}

bool FilterRules::get_need_match_all()
{
    return ui->need_match_all->isChecked();
}

void FilterRules::set_search_text(std::string search_text)
{
    ui->search->setText(QString::fromStdString(search_text));
}

void FilterRules::set_search_type(SearchType search_type)
{
    switch(search_type)
    {
        case SearchType_rawText:
            ui->search_type->setCurrentIndex(0);
        break;
        case SearchType_simpleRegex:
            ui->search_type->setCurrentIndex(1);
        break;
        case SearchType_perlRegex:
            ui->search_type->setCurrentIndex(2);
        break;
    }
}

void FilterRules::set_apply_on(ApplyOn apply_on)
{
    switch(apply_on)
    {
        case ApplyOn_file:
            ui->apply_on->setCurrentIndex(0);
        break;
        case ApplyOn_fileAndFolder:
            ui->apply_on->setCurrentIndex(1);
        break;
        case ApplyOn_folder:
            ui->apply_on->setCurrentIndex(2);
        break;
    }
}

void FilterRules::set_need_match_all(bool need_match_all)
{
    ui->need_match_all->setChecked(need_match_all);
}

void FilterRules::on_search_textChanged(const std::string &arg1)
{
    Q_UNUSED(arg1);
    updateChecking();
}

void FilterRules::updateChecking()
{
    QRegularExpression regex;
    isValid=!ui->search->text().isEmpty();
    if(isValid)
    {
        QString tempString;
        if(ui->search_type->currentIndex()==0)
        {
            //tempString=QRegularExpression::escape(ui->search->text()); -> generate bug because escape contains slash
            if(tempString.contains('/') || tempString.contains('\\'))
                isValid=false;
        }
        else if(ui->search_type->currentIndex()==1)
        {
            tempString=QRegularExpression::escape(ui->search->text());
            tempString.replace(QStringLiteral("\\*"),QStringLiteral("[^\\\\/]*"));
        }
        else if(ui->search_type->currentIndex()==2)
        {
            tempString=ui->search->text();
            if(tempString.startsWith('^') && tempString.endsWith('$'))
            {
                ui->need_match_all->setChecked(true);
                tempString.remove(QRegularExpression(QStringLiteral("^\\^")));
                tempString.remove(QRegularExpression(QStringLiteral("\\$$")));
                ui->search->setText(tempString);
            }
        }
        if(isValid)
        {
            if(ui->need_match_all->isChecked())
                tempString=QStringLiteral("^")+tempString+QStringLiteral("$");
            regex=QRegularExpression(tempString);
            isValid=regex.isValid();
        }
    }

    ui->isValid->setChecked(isValid);
    ui->testString->setEnabled(isValid);
    ui->label_test_string->setEnabled(isValid);
    ui->matched->setEnabled(isValid);
    ui->matched->setChecked(isValid && ui->testString->text().contains(regex));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

void FilterRules::on_isValid_clicked()
{
    updateChecking();
}

void FilterRules::on_testString_textChanged(const std::string &arg1)
{
    Q_UNUSED(arg1);
    updateChecking();
}

void FilterRules::on_matched_clicked()
{
    updateChecking();
}

void FilterRules::on_search_type_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateChecking();
}

void FilterRules::on_need_match_all_clicked()
{
    updateChecking();
}

void FilterRules::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::RejectRole)
        reject();
    else
    {
        haveBeenValided=true;
        accept();
    }
}

void FilterRules::on_search_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateChecking();
}
