/** \file FacilityInterface.h
\brief Define the class of the facility engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FACILITY_INTERFACE_H
#define FACILITY_INTERFACE_H

#include <QString>

#include "../StructEnumDefinition.h"

class FacilityInterface : public QObject
{
	Q_OBJECT
	public:
		/// \brief To force the text re-translation
		virtual void retranslate() = 0;
		/// \brief convert size in Byte to String
		virtual QString sizeToString(double size) = 0;
		/// \brief convert size unit to String
		virtual QString sizeUnitToString(SizeUnit sizeUnit) = 0;
		/// \brief translate the text
		virtual QString translateText(QString text) = 0;
		/// \brief speed to string in byte per seconds
		virtual QString speedToString(double speed) = 0;
		/// \brief Decompose the time in second
		virtual TimeDecomposition secondsToTimeDecomposition(quint32 seconds) = 0;
		//second remaining -> text
};

#endif // FACILITY_INTERFACE_H
