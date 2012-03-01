#include "FacilityEngine.h"

#if defined (Q_OS_WIN32)
#include <windows.h>
#endif

FacilityEngine::FacilityEngine()
{
	retranslate();
}

/// \brief To force the text re-translation
void FacilityEngine::retranslate()
{
	//translated string
	Translation_Copy_engine=tr("Copy engine");
	Translation_Copy=tr("Copy");
	Translation_Move=tr("Move");
	Translation_Pause=tr("Pause");
	Translation_Resume=tr("Resume");
	Translation_Skip=tr("Skip");
	Translation_Unlimited=tr("Unlimited");
	//undirect translated string
	Translation_perSecond="/"+tr("s");
	Translation_tooBig=tr("Too big");
	Translation_B=tr("B");
	Translation_KB=tr("KB");
	Translation_MB=tr("MB");
	Translation_GB=tr("GB");
	Translation_TB=tr("TB");
	Translation_PB=tr("PB");
	Translation_EB=tr("EB");
	Translation_ZB=tr("ZB");
	Translation_YB=tr("YB");
}

/// \brief convert size in Byte to String
QString FacilityEngine::sizeToString(double size)
{
	if(size<1024)
		return QString::number(size)+sizeUnitToString(SizeUnit_byte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_KiloByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_MegaByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_GigaByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_TeraByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_PetaByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_ExaByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_ZettaByte);
	if((size=size/1024)<1024)
		return adaptString(size)+sizeUnitToString(SizeUnit_YottaByte);
	return Translation_tooBig;
}

QString FacilityEngine::adaptString(float size)
{
	if(size>=100)
		return QString::number(size,'f',0);
	else
		return QString::number(size,'g',3);
}


/// \brief convert size unit to String
QString FacilityEngine::sizeUnitToString(SizeUnit sizeUnit)
{
	switch(sizeUnit)
	{
		case SizeUnit_byte:
			return Translation_B;
		case SizeUnit_KiloByte:
			return Translation_KB;
		case SizeUnit_MegaByte:
			return Translation_MB;
		case SizeUnit_GigaByte:
			return Translation_GB;
		case SizeUnit_TeraByte:
			return Translation_TB;
		case SizeUnit_PetaByte:
			return Translation_PB;
		case SizeUnit_ExaByte:
			return Translation_EB;
		case SizeUnit_ZettaByte:
			return Translation_ZB;
		case SizeUnit_YottaByte:
			return Translation_YB;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"sizeUnit: "+QString::number(sizeUnit));
			return "???";
	}
}

/// \brief translate the text
QString FacilityEngine::translateText(QString text)
{
	if(text=="Copy engine")
		return Translation_Copy_engine;
	if(text=="Copy")
		return Translation_Copy;
	if(text=="Move")
		return Translation_Move;
	if(text=="Pause")
		return Translation_Pause;
	if(text=="Resume")
		return Translation_Resume;
	if(text=="Skip")
		return Translation_Skip;
	if(text=="Unlimited")
		return Translation_Unlimited;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"text: "+text);
	return text;
}

/// \brief speed to string in byte per seconds
QString FacilityEngine::speedToString(double speed)
{
	return sizeToString(speed)+Translation_perSecond;
}
/// \brief Decompose the time in second
TimeDecomposition FacilityEngine::secondsToTimeDecomposition(quint32 seconds)
{
	TimeDecomposition returnValue;
	returnValue.second=seconds%60;
	seconds-=returnValue.second;
	seconds/=60;
	returnValue.minute=seconds%60;
	seconds-=returnValue.minute;
	seconds/=60;
	returnValue.hour=seconds;
	return returnValue;
}

/// \brief have the functionality
bool FacilityEngine::haveFunctionality(QString fonctionnality)
{
	#if defined (Q_OS_WIN32)
	if(fonctionnality=="shutdown")
		return true;
	#endif
	Q_UNUSED(fonctionnality);
	return false;
}

/// \brief call the fonctionnality
QVariant FacilityEngine::callFunctionality(QString fonctionnality,QStringList args)
{
	#if defined (Q_OS_WIN32)
	BOOL WINAPI ExitWindowsEx(EWX_SHUTDOWN,0);
	#endif
	Q_UNUSED(fonctionnality);
	Q_UNUSED(args);
	return QVariant();
}
