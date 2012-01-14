/** \file OptionEngine.h
\brief Define the class of the option engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef OPTION_INTERFACE_H
#define OPTION_INTERFACE_H

#include <QString>
#include <QList>
#include <QVariant>
#include <QPair>

#include "../StructEnumDefinition.h"

class OptionInterface : public QObject
{
	Q_OBJECT
	public:
		/// \brief To add option group to options
		virtual bool addOptionGroup(QList<QPair<QString, QVariant> > KeysList) = 0;
		/*/// \brief To remove option group to options, removed to the load plugin
		virtual bool removeOptionGroup() = 0;*/
		/// \brief To get option value
		virtual QVariant getOptionValue(QString variableName) = 0;
		/// \brief To set option value
		virtual void setOptionValue(QString variableName,QVariant value) = 0;
	/* signal to implement
	signals:
		//void newOptionValue(QString,QVariant);-> disabled because the value will not externally changed, then useless notification
		void resetOptions();*/
};

#endif // OPTION_INTERFACE_H
