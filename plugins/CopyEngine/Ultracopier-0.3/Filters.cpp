#include "Filters.h"
#include "ui_Filters.h"

#include <QMessageBox>

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

void Filters::setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions)
{
	if(includeStrings.size()!=includeOptions.size() || excludeStrings.size()!=excludeOptions.size())
		return;
	Filters_rules new_item;

	include.clear();
	int index=0;
	while(index<includeStrings.size())
	{
		new_item.search_text=includeStrings.at(index);
		QStringList options=includeOptions.at(index).split(";");
		new_item.need_match_all=false;
		new_item.search_type=SearchType_rawText;
		new_item.apply_on=ApplyOn_fileAndFolder;

		if(options.contains("SearchType_simpleRegex"))
			new_item.search_type=SearchType_simpleRegex;
		if(options.contains("SearchType_perlRegex"))
			new_item.search_type=SearchType_perlRegex;
		if(options.contains("ApplyOn_file"))
			new_item.apply_on=ApplyOn_file;
		if(options.contains("ApplyOn_folder"))
			new_item.apply_on=ApplyOn_folder;
		if(options.contains("need_match_all"))
			new_item.need_match_all=true;

		include << new_item;

		index++;
	}

	exclude.clear();
	index=0;
	while(index<excludeStrings.size())
	{
		new_item.search_text=excludeStrings.at(index);
		QStringList options=excludeOptions.at(index).split(";");
		new_item.need_match_all=false;
		new_item.search_type=SearchType_rawText;
		new_item.apply_on=ApplyOn_fileAndFolder;

		if(options.contains("SearchType_simpleRegex"))
			new_item.search_type=SearchType_simpleRegex;
		if(options.contains("SearchType_perlRegex"))
			new_item.search_type=SearchType_perlRegex;
		if(options.contains("ApplyOn_file"))
			new_item.apply_on=ApplyOn_file;
		if(options.contains("ApplyOn_folder"))
			new_item.apply_on=ApplyOn_folder;
		if(options.contains("need_match_all"))
			new_item.need_match_all=true;

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
		QString entryShow=include.at(index).search_text+" (";
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
		entryShow+=optionsToShow.join(",");
		entryShow+=")";
		ui->inclusion->addItem(new QListWidgetItem(entryShow));
		index++;
	}
	ui->exclusion->clear();
	index=0;
	while(index<exclude.size())
	{
		QString entryShow=exclude.at(index).search_text+" (";
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
		entryShow+=optionsToShow.join(",");
		entryShow+=")";
		ui->exclusion->addItem(new QListWidgetItem(entryShow));
		index++;
	}
}

QList<Filters_rules> Filters::getInclude()
{
	return include;
}

QList<Filters_rules> Filters::getExclude()
{
	return exclude;
}

void Filters::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		reShowAll();
		break;
	default:
		break;
	}
}

void Filters::haveNewFilters()
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
				optionsToShow << "SearchType_rawText";
			break;
			case SearchType_simpleRegex:
				optionsToShow << "SearchType_simpleRegex";
			break;
			case SearchType_perlRegex:
				optionsToShow << "SearchType_perlRegex";
			break;
			default:
			break;
		}
		switch(include.at(index).apply_on)
		{
			case ApplyOn_file:
				optionsToShow << "ApplyOn_file";
			break;
			case ApplyOn_fileAndFolder:
				optionsToShow << "ApplyOn_fileAndFolder";
			break;
			case ApplyOn_folder:
				optionsToShow << "ApplyOn_folder";
			break;
			default:
			break;
		}
		if(include.at(index).need_match_all)
			optionsToShow << tr("need_match_all");
		includeOptions<<optionsToShow.join(";");
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
				optionsToShow << "SearchType_rawText";
			break;
			case SearchType_simpleRegex:
				optionsToShow << "SearchType_simpleRegex";
			break;
			case SearchType_perlRegex:
				optionsToShow << "SearchType_perlRegex";
			break;
			default:
			break;
		}
		switch(exclude.at(index).apply_on)
		{
			case ApplyOn_file:
				optionsToShow << "ApplyOn_file";
			break;
			case ApplyOn_fileAndFolder:
				optionsToShow << "ApplyOn_fileAndFolder";
			break;
			case ApplyOn_folder:
				optionsToShow << "ApplyOn_folder";
			break;
			default:
			break;
		}
		if(exclude.at(index).need_match_all)
			optionsToShow << tr("need_match_all");
		excludeOptions<<optionsToShow.join(";");
		index++;
	}
	emit sendNewFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
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
		haveNewFilters();
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
		haveNewFilters();
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
		haveNewFilters();
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
		include << new_item;
		reShowAll();
		haveNewFilters();
	}
}
