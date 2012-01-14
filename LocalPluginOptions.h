#ifndef LOCALPLUGINOPTIONS_H
#define LOCALPLUGINOPTIONS_H

#include <QObject>

#include "interface/OptionInterface.h"
#include "OptionEngine.h"

class LocalPluginOptions : public OptionInterface
{
	Q_OBJECT
public:
	explicit LocalPluginOptions(QString group);
	~LocalPluginOptions();
	/// \brief To add option group to options
	bool addOptionGroup(QList<QPair<QString, QVariant> > KeysList);
	/*/// \brief To remove option group to options, removed to the load plugin
	bool removeOptionGroup();*/
	/// \brief To get option value
	QVariant getOptionValue(QString variableName);
	/// \brief To set option value
	void setOptionValue(QString variableName,QVariant value);
protected:
	//for the options
	OptionEngine *options;
	QString group;
	bool groupOptionAdded;
/*public slots:-> disabled because the value will not externaly changed, then useless notification
	void newOptionValue(QString group,QString variable,QVariant value);*/
};

#endif // LOCALPLUGINOPTIONS_H
