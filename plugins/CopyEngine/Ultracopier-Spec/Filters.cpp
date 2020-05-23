#include "Filters.h"
#include "ui_Filters.h"
#include "../../../cpp11addition.h"

#include <QRegularExpression>

Filters::Filters(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::Filters)
{
    ui->setupUi(this);
}

Filters::~Filters()
{
    delete ui;
}

bool Filters::setFilters(std::vector<std::string> includeStrings,std::vector<std::string> includeOptions,std::vector<std::string> excludeStrings,std::vector<std::string> excludeOptions)
{
    if(includeStrings.size()!=includeOptions.size() || excludeStrings.size()!=excludeOptions.size())
        return false;
    Filters_rules new_item;

    include.clear();
    unsigned int index=0;
    while(index<(unsigned int)includeStrings.size())
    {
        new_item.search_text=includeStrings.at(index);
        std::vector<std::string> options=stringsplit(includeOptions.at(index),';');
        new_item.need_match_all=false;
        new_item.search_type=SearchType_rawText;
        new_item.apply_on=ApplyOn_fileAndFolder;

        if(vectorcontainsAtLeastOne(options,std::string("SearchType_simpleRegex")))
            new_item.search_type=SearchType_simpleRegex;
        if(vectorcontainsAtLeastOne(options,std::string("SearchType_perlRegex")))
            new_item.search_type=SearchType_perlRegex;
        if(vectorcontainsAtLeastOne(options,std::string("ApplyOn_file")))
            new_item.apply_on=ApplyOn_file;
        if(vectorcontainsAtLeastOne(options,std::string("ApplyOn_folder")))
            new_item.apply_on=ApplyOn_folder;
        if(vectorcontainsAtLeastOne(options,std::string("need_match_all")))
            new_item.need_match_all=true;

        if(convertToRegex(new_item))
            include.push_back(new_item);

        index++;
    }

    exclude.clear();
    index=0;
    while(index<excludeStrings.size())
    {
        new_item.search_text=excludeStrings.at(index);
        std::vector<std::string> options=stringsplit(excludeOptions.at(index),';');
        new_item.need_match_all=false;
        new_item.search_type=SearchType_rawText;
        new_item.apply_on=ApplyOn_fileAndFolder;

        if(vectorcontainsAtLeastOne(options,std::string("SearchType_simpleRegex")))
            new_item.search_type=SearchType_simpleRegex;
        if(vectorcontainsAtLeastOne(options,std::string("SearchType_perlRegex")))
            new_item.search_type=SearchType_perlRegex;
        if(vectorcontainsAtLeastOne(options,std::string("ApplyOn_file")))
            new_item.apply_on=ApplyOn_file;
        if(vectorcontainsAtLeastOne(options,std::string("ApplyOn_folder")))
            new_item.apply_on=ApplyOn_folder;
        if(vectorcontainsAtLeastOne(options,std::string("need_match_all")))
            new_item.need_match_all=true;

        if(convertToRegex(new_item))
            exclude.push_back(new_item);

        index++;
    }

    reShowAll();
    return true;
}

void Filters::reShowAll()
{
    ui->inclusion->clear();
    unsigned int index=0;
    while(index<(unsigned int)include.size())
    {
        std::string entryShow=include.at(index).search_text+" (";
        std::vector<std::string> optionsToShow;
        switch(include.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow.push_back(tr("Raw text").toStdString());
            break;
            case SearchType_simpleRegex:
                optionsToShow.push_back(tr("Simplified regex").toStdString());
            break;
            case SearchType_perlRegex:
                optionsToShow.push_back(tr("Perl's regex").toStdString());
            break;
            default:
            break;
        }
        switch(include.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow.push_back(tr("Only on file").toStdString());
            break;
            case ApplyOn_folder:
                optionsToShow.push_back(tr("Only on folder").toStdString());
            break;
            default:
            break;
        }
        if(include.at(index).need_match_all)
            optionsToShow.push_back(tr("Full match").toStdString());
        entryShow+=stringimplode(optionsToShow,",");
        entryShow+=")";
        ui->inclusion->addItem(new QListWidgetItem(QString::fromStdString(entryShow)));
        index++;
    }
    ui->exclusion->clear();
    index=0;
    while(index<(unsigned int)exclude.size())
    {
        std::string entryShow=exclude.at(index).search_text+" (";
        std::vector<std::string> optionsToShow;
        switch(exclude.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow.push_back(tr("Raw text").toStdString());
            break;
            case SearchType_simpleRegex:
                optionsToShow.push_back(tr("Simplified regex").toStdString());
            break;
            case SearchType_perlRegex:
                optionsToShow.push_back(tr("Perl's regex").toStdString());
            break;
            default:
            break;
        }
        switch(exclude.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow.push_back(tr("Only on file").toStdString());
            break;
            case ApplyOn_folder:
                optionsToShow.push_back(tr("Only on folder").toStdString());
            break;
            default:
            break;
        }
        if(exclude.at(index).need_match_all)
            optionsToShow.push_back(tr("Full match").toStdString());
        entryShow+=stringimplode(optionsToShow,",");
        entryShow+=")";
        ui->exclusion->addItem(new QListWidgetItem(QString::fromStdString(entryShow)));
        index++;
    }
}

std::vector<Filters_rules> Filters::getInclude() const
{
    return include;
}

std::vector<Filters_rules> Filters::getExclude() const
{
    return exclude;
}

void Filters::newLanguageLoaded()
{
    ui->retranslateUi(this);
    reShowAll();
}

void Filters::updateFilters()
{
    std::vector<std::string> includeStrings,includeOptions,excludeStrings,excludeOptions;
    unsigned int index=0;
    while(index<(unsigned int)include.size())
    {
        includeStrings.push_back(include.at(index).search_text);
        std::vector<std::string> optionsToShow;

        switch(include.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow.push_back("SearchType_rawText");
            break;
            case SearchType_simpleRegex:
                optionsToShow.push_back("SearchType_simpleRegex");
            break;
            case SearchType_perlRegex:
                optionsToShow.push_back("SearchType_perlRegex");
            break;
            default:
            break;
        }
        switch(include.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow.push_back("ApplyOn_file");
            break;
            case ApplyOn_fileAndFolder:
                optionsToShow.push_back("ApplyOn_fileAndFolder");
            break;
            case ApplyOn_folder:
                optionsToShow.push_back("ApplyOn_folder");
            break;
            default:
            break;
        }
        if(include.at(index).need_match_all)
            optionsToShow.push_back(tr("Full match").toStdString());
        includeOptions.push_back(stringimplode(optionsToShow,";"));
        index++;
    }
    index=0;
    while(index<(unsigned int)exclude.size())
    {
        excludeStrings.push_back(exclude.at(index).search_text);
        std::vector<std::string> optionsToShow;

        switch(exclude.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow.push_back("SearchType_rawText");
            break;
            case SearchType_simpleRegex:
                optionsToShow.push_back("SearchType_simpleRegex");
            break;
            case SearchType_perlRegex:
                optionsToShow.push_back("SearchType_perlRegex");
            break;
            default:
            break;
        }
        switch(exclude.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow.push_back("ApplyOn_file");
            break;
            case ApplyOn_fileAndFolder:
                optionsToShow.push_back("ApplyOn_fileAndFolder");
            break;
            case ApplyOn_folder:
                optionsToShow.push_back("ApplyOn_folder");
            break;
            default:
            break;
        }
        if(exclude.at(index).need_match_all)
            optionsToShow.push_back(tr("Full match").toStdString());
        excludeOptions.push_back(stringimplode(optionsToShow,";"));
        index++;
    }
    emit sendNewFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    emit haveNewFilters();
}

bool Filters::convertToRegex(Filters_rules &item)
{
    bool isValid=!item.search_text.empty();
    if(isValid)
    {
        std::regex regex;
        std::string tempString;
        if(item.search_type==SearchType_rawText)
        {
            //here to validate below the regex
            tempString=QRegularExpression::escape(QString::fromStdString(item.search_text)).toStdString();
            //do search on string only on file or file and folder, QRegularExpression::escape() introduce \ on special char
            if(item.apply_on!=ApplyOn::ApplyOn_folder)
                if(item.search_text.find('/') != std::string::npos || item.search_text.find('\\') != std::string::npos)
                    isValid=false;
        }
        else if(item.search_type==SearchType_simpleRegex)
        {
            tempString=QRegularExpression::escape(QString::fromStdString(item.search_text)).toStdString();
            stringreplaceAll(tempString,"\\*","[^\\\\/]*");
        }
        else if(item.search_type==SearchType_perlRegex)
        {
            tempString=item.search_text;
            if(stringStartWith(tempString,'^') && stringEndsWith(tempString,'$'))
            {
                item.need_match_all=true;
                if(stringStartWith(tempString,'^'))
                    tempString=tempString.substr(1,tempString.size()-1);
                if(stringEndsWith(tempString,'$'))
                    tempString=tempString.substr(0,tempString.size()-1);
                item.search_text=tempString;
            }
        }
        if(isValid)
        {
            if(item.need_match_all==true)
                tempString="^"+tempString+"$";
            regex=std::regex(tempString);
            //isValid=regex.isValid();
            item.regex=regex;
            return true;
        }
        else
            return false;
    }
    return false;
}

void Filters::on_remove_exclusion_clicked()
{
    bool removedEntry=false;
    int index=0;
    while(index<ui->exclusion->count())
    {
        if(ui->exclusion->item(index)->isSelected())
        {
            delete ui->exclusion->item(index);
            exclude.erase(exclude.cbegin()+index);
            removedEntry=true;
        }
        else
            index++;
    }
    if(removedEntry)
    {
        reShowAll();
        updateFilters();
    }
}

void Filters::on_remove_inclusion_clicked()
{
    bool removedEntry=false;
    int index=0;
    while(index<ui->inclusion->count())
    {
        if(ui->inclusion->item(index)->isSelected())
        {
            delete ui->inclusion->item(index);
            include.erase(include.cbegin()+index);
            removedEntry=true;
        }
        else
            index++;
    }
    if(removedEntry)
    {
        reShowAll();
        updateFilters();
    }
}

void Filters::on_add_exclusion_clicked()
{
    FilterRules dialog(this);
    dialog.exec();
    if(dialog.getIsValid())
    {
        Filters_rules new_item;
        new_item.apply_on=dialog.get_apply_on();
        new_item.need_match_all=dialog.get_need_match_all();
        new_item.search_text=dialog.get_search_text();
        new_item.search_type=dialog.get_search_type();
        exclude.push_back(new_item);
        reShowAll();
        updateFilters();
    }
}

void Filters::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::RejectRole)
        reject();
}

void Filters::on_add_inclusion_clicked()
{
    FilterRules dialog(this);
    dialog.exec();
    if(dialog.getIsValid())
    {
        Filters_rules new_item;
        new_item.apply_on=dialog.get_apply_on();
        new_item.need_match_all=dialog.get_need_match_all();
        new_item.search_text=dialog.get_search_text();
        new_item.search_type=dialog.get_search_type();
        if(convertToRegex(new_item))
            include.push_back(new_item);
        reShowAll();
        updateFilters();
    }
}

void Filters::on_edit_exclusion_clicked()
{
    bool editedEntry=false;
    int index=0;
    while(index<ui->exclusion->count())
    {
        if(ui->exclusion->item(index)->isSelected())
        {
            FilterRules dialog(this);
            dialog.set_apply_on(exclude.at(index).apply_on);
            dialog.set_need_match_all(exclude.at(index).need_match_all);
            dialog.set_search_text(exclude.at(index).search_text);
            dialog.set_search_type(exclude.at(index).search_type);
            dialog.exec();
            if(dialog.getIsValid())
            {
                exclude[index].apply_on=dialog.get_apply_on();
                exclude[index].need_match_all=dialog.get_need_match_all();
                exclude[index].search_text=dialog.get_search_text();
                exclude[index].search_type=dialog.get_search_type();
                if(!convertToRegex(exclude[index]))
                    exclude.erase(exclude.cbegin()+index);
                editedEntry=true;
            }
        }
        index++;
    }
    if(editedEntry)
    {
        reShowAll();
        updateFilters();
    }
}

void Filters::on_edit_inclusion_clicked()
{
    bool editedEntry=false;
    int index=0;
    while(index<ui->inclusion->count())
    {
        if(ui->inclusion->item(index)->isSelected())
        {
            FilterRules dialog(this);
            dialog.set_apply_on(exclude.at(index).apply_on);
            dialog.set_need_match_all(exclude.at(index).need_match_all);
            dialog.set_search_text(exclude.at(index).search_text);
            dialog.set_search_type(exclude.at(index).search_type);
            dialog.exec();
            if(dialog.getIsValid())
            {
                exclude[index].apply_on=dialog.get_apply_on();
                exclude[index].need_match_all=dialog.get_need_match_all();
                exclude[index].search_text=dialog.get_search_text();
                exclude[index].search_type=dialog.get_search_type();
                if(!convertToRegex(exclude[index]))
                    exclude.erase(exclude.cbegin()+index);
                editedEntry=true;
            }
        }
        index++;
    }
    if(editedEntry)
    {
        reShowAll();
        updateFilters();
    }
}
