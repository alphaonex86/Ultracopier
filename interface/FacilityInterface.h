/** \file FacilityInterface.h
\brief Define the class of the facility engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FACILITY_INTERFACE_H
#define FACILITY_INTERFACE_H

#include <QVariant>
#include <QString>
#include <QStringList>

#include "../StructEnumDefinition.h"

/// \brief To define the interface, to pass the facility object from Ultracopier to the plugins without compatibility problem
class FacilityInterface : public QObject
{
	public:
		/// \brief To force the text re-translation
		virtual void retranslate() = 0;
		/// \brief convert size in Byte to String
		virtual QString sizeToString(const double &size) = 0;
		/// \brief convert size unit to String
		virtual QString sizeUnitToString(const SizeUnit &sizeUnit) = 0;
		/// \brief translate the text
		virtual QString translateText(const QString &text) = 0;
		/// \brief speed to string in byte per seconds
		virtual QString speedToString(const double &speed) = 0;
		/// \brief Decompose the time in second
		virtual TimeDecomposition secondsToTimeDecomposition(const quint32 &seconds) = 0;
		/// \brief have the fonctionnality
		virtual bool haveFunctionality(const QString &fonctionnality) = 0;
		/// \brief call the fonctionnality
		virtual QVariant callFunctionality(const QString &fonctionnality,const QStringList &args=QStringList()) = 0;
		/// \brief Do the simplified time
		virtual QString simplifiedRemainingTime(const quint32 &seconds) = 0;
};

#endif // FACILITY_INTERFACE_H
