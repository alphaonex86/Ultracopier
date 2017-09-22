#include "Filters.h"
#include "ui_Filters.h"

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

void Filters::setFilters(std::vector<std::string> includeStrings,std::vector<std::string> includeOptions,std::vector<std::string> excludeStrings,std::vector<std::string> excludeOptions)
{
    if(includeStrings.size()!=includeOptions.size() || excludeStrings.size()!=excludeOptions.size())
        return;
    Filters_rules new_item;

    include.clear();
    int index=0;
    while(index<includeStrings.size())
    {
        new_item.search_text=includeStrings.at(index);
        QStringList options=includeOptions.at(index).split(QStringLiteral(";"));
        new_item.need_match_all=false;
        new_item.search_type=SearchType_rawText;
        new_item.apply_on=ApplyOn_fileAndFolder;

        if(options.contains(QStringLiteral("SearchType_simpleRegex")))
            new_item.search_type=SearchType_simpleRegex;
        if(options.contains(QStringLiteral("SearchType_perlRegex")))
            new_item.search_type=SearchType_perlRegex;
        if(options.contains(QStringLiteral("ApplyOn_file")))
            new_item.apply_on=ApplyOn_file;
        if(options.contains(QStringLiteral("ApplyOn_folder")))
            new_item.apply_on=ApplyOn_folder;
        if(options.contains(QStringLiteral("need_match_all")))
            new_item.need_match_all=true;

        if(convertToRegex(new_item))
            include << new_item;

        index++;
    }

    exclude.clear();
    index=0;
    while(index<excludeStrings.size())
    {
        new_item.search_text=excludeStrings.at(index);
        QStringList options=excludeOptions.at(index).split(QStringLiteral(";"));
        new_item.need_match_all=false;
        new_item.search_type=SearchType_rawText;
        new_item.apply_on=ApplyOn_fileAndFolder;

        if(options.contains(QStringLiteral("SearchType_simpleRegex")))
            new_item.search_type=SearchType_simpleRegex;
        if(options.contains(QStringLiteral("SearchType_perlRegex")))
            new_item.search_type=SearchType_perlRegex;
        if(options.contains(QStringLiteral("ApplyOn_file")))
            new_item.apply_on=ApplyOn_file;
        if(options.contains(QStringLiteral("ApplyOn_folder")))
            new_item.apply_on=ApplyOn_folder;
        if(options.contains(QStringLiteral("need_match_all")))
            new_item.need_match_all=true;

        if(convertToRegex(new_item))
            exclude << new_item;

        index++;
    }

    reShowAll();
}

void Filters::reShowAll()
{
    ui->inclusion->clear();
    int index=0;
    while(index<include.size())
    {
        QString entryShow=include.at(index).search_text+QStringLiteral(" (");
        QStringList optionsToShow;
        switch(include.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow << tr("Raw text");
            break;
            case SearchType_simpleRegex:
                optionsToShow << tr("Simplified regex");
            break;
            case SearchType_perlRegex:
                optionsToShow << tr("Perl's regex");
            break;
            default:
            break;
        }
        switch(include.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow << tr("Only on file");
            break;
            case ApplyOn_folder:
                optionsToShow << tr("Only on folder");
            break;
            default:
            break;
        }
        if(include.at(index).need_match_all)
            optionsToShow << tr("Full match");
        entryShow+=optionsToShow.join(QStringLiteral(","));
        entryShow+=")";
        ui->inclusion->addItem(new QListWidgetItem(entryShow));
        index++;
    }
    ui->exclusion->clear();
    index=0;
    while(index<exclude.size())
    {
        QString entryShow=exclude.at(index).search_text+QStringLiteral(" (");
        QStringList optionsToShow;
        switch(exclude.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow << tr("Raw text");
            break;
            case SearchType_simpleRegex:
                optionsToShow << tr("Simplified regex");
            break;
            case SearchType_perlRegex:
                optionsToShow << tr("Perl's regex");
            break;
            default:
            break;
        }
        switch(exclude.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow << tr("Only on file");
            break;
            case ApplyOn_folder:
                optionsToShow << tr("Only on folder");
            break;
            default:
            break;
        }
        if(exclude.at(index).need_match_all)
            optionsToShow << tr("Full match");
        entryShow+=optionsToShow.join(QStringLiteral(","));
        entryShow+=")";
        ui->exclusion->addItem(new QListWidgetItem(entryShow));
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
    QStringList includeStrings,includeOptions,excludeStrings,excludeOptions;
    int index=0;
    while(index<include.size())
    {
        includeStrings<<include.at(index).search_text;
        QStringList optionsToShow;

        switch(include.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow << QStringLiteral("SearchType_rawText");
            break;
            case SearchType_simpleRegex:
                optionsToShow << QStringLiteral("SearchType_simpleRegex");
            break;
            case SearchType_perlRegex:
                optionsToShow << QStringLiteral("SearchType_perlRegex");
            break;
            default:
            break;
        }
        switch(include.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow << QStringLiteral("ApplyOn_file");
            break;
            case ApplyOn_fileAndFolder:
                optionsToShow << QStringLiteral("ApplyOn_fileAndFolder");
            break;
            case ApplyOn_folder:
                optionsToShow << QStringLiteral("ApplyOn_folder");
            break;
            default:
            break;
        }
        if(include.at(index).need_match_all)
            optionsToShow << tr("Full match");
        includeOptions<<optionsToShow.join(QStringLiteral(";"));
        index++;
    }
    index=0;
    while(index<exclude.size())
    {
        excludeStrings<<exclude.at(index).search_text;
        QStringList optionsToShow;

        switch(exclude.at(index).search_type)
        {
            case SearchType_rawText:
                optionsToShow << QStringLiteral("SearchType_rawText");
            break;
            case SearchType_simpleRegex:
                optionsToShow << QStringLiteral("SearchType_simpleRegex");
            break;
            case SearchType_perlRegex:
                optionsToShow << QStringLiteral("SearchType_perlRegex");
            break;
            default:
            break;
        }
        switch(exclude.at(index).apply_on)
        {
            case ApplyOn_file:
                optionsToShow << QStringLiteral("ApplyOn_file");
            break;
            case ApplyOn_fileAndFolder:
                optionsToShow << QStringLiteral("ApplyOn_fileAndFolder");
            break;
            case ApplyOn_folder:
                optionsToShow << QStringLiteral("ApplyOn_folder");
            break;
            default:
            break;
        }
        if(exclude.at(index).need_match_all)
            optionsToShow << tr("Full match");
        excludeOptions<<optionsToShow.join(QStringLiteral(";"));
        index++;
    }
    emit sendNewFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    emit haveNewFilters();
}

bool Filters::convertToRegex(Filters_rules &item)
{
    bool isValid=!item.search_text.isEmpty();
    if(isValid)
    {
        QRegularExpression regex;
        QString tempString;
        if(item.search_type==SearchType_rawText)
        {
            tempString=QRegularExpression::escape(item.search_text);
            if(tempString.contains('/') || tempString.contains('\\'))
                isValid=false;
        }
        else if(item.search_type==SearchType_simpleRegex)
        {
            tempString=QRegularExpression::escape(item.search_text);
            tempString.replace(QStringLiteral("\\*"),QStringLiteral("[^\\\\/]*"));
        }
        else if(item.search_type==SearchType_perlRegex)
        {
            tempString=item.search_text;
            if(tempString.startsWith('^') && tempString.endsWith('$'))
            {
                item.need_match_all=true;
                tempString.remove(QRegularExpression("^\\^"));
                tempString.remove(QRegularExpression("\\$$"));
                item.search_text=tempString;
            }
        }
        if(isValid)
        {
            if(item.need_match_all==true)
                tempString=QStringLiteral("^")+tempString+QStringLiteral("$");
            regex=QRegularExpression(tempString);
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
            exclude.removeAt(index);
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
            include.removeAt(index);
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
        exclude << new_item;
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
            include << new_item;
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
                    exclude.removeAt(index);
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
                    exclude.removeAt(index);
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
