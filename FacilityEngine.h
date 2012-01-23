#ifndef FACILITYENGINE_H
#define FACILITYENGINE_H

#include "interface/FacilityInterface.h"
#include "Environment.h"

/** \brief Class to group general function for the plugin

This class is used into some plugin like copy engine plugin, to all into one place all common function, group the traduction, and all what it can grouped across all plugin into Ultracopier core application.
*/
class FacilityEngine : public FacilityInterface
{
	Q_OBJECT
public:
	explicit FacilityEngine();
	/// \brief convert size in Byte to String
	QString sizeToString(double size);
	/// \brief convert size unit to String
	QString sizeUnitToString(SizeUnit sizeUnit);
	/// \brief translate the text
	QString translateText(QString text);
	/// \brief speed to string in byte per seconds
	QString speedToString(double speed);
	/// \brief Decompose the time in second
	TimeDecomposition secondsToTimeDecomposition(quint32 seconds);
private:
	//translated string
	QString Translation_Copy_engine;
	QString Translation_Copy;
	QString Translation_Move;
	QString Translation_Pause;
	QString Translation_Resume;
	QString Translation_Skip;
	QString Translation_Unlimited;
	//undirect translated string
	QString Translation_perSecond;
	QString Translation_tooBig;
	QString Translation_B;
	QString Translation_KB;
	QString Translation_MB;
	QString Translation_GB;
	QString Translation_TB;
	QString Translation_PB;
	QString Translation_EB;
	QString Translation_ZB;
	QString Translation_YB;
	//internal fonction
	QString adaptString(float nb);
public slots:
	/// \brief To force the text re-translation
	void retranslate();
};

#endif // FACILITYENGINE_H
