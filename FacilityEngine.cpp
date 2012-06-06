/** \file FacilityEngine.cpp
\brief To implement the facility engine, the interface is defined into FacilityInterface()
\see FacilityInterface()
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

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
	Translation_SimplifiedRemaningTime_LessThan10s=tr("Less than %10 seconds");
	Translation_SimplifiedRemaningTime_AboutSeconds=tr("About %10 seconds remaining");
	Translation_SimplifiedRemaningTime_AboutMinutes=tr("About %1 minutes remaining");
	Translation_SimplifiedRemaningTime_AboutHours=tr("About %1 hours remaining");
}

/// \brief convert size in Byte to String
QString FacilityEngine::sizeToString(const double &size)
{
	double size_temp=size;
	if(size_temp<1024)
		return QString::number(size_temp)+sizeUnitToString(SizeUnit_byte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_KiloByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_MegaByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_GigaByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_TeraByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_PetaByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_ExaByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_ZettaByte);
	if((size_temp=size_temp/1024)<1024)
		return adaptString(size_temp)+sizeUnitToString(SizeUnit_YottaByte);
	return Translation_tooBig;
}

QString FacilityEngine::adaptString(const float &size)
{
	if(size>=100)
		return QString::number(size,'f',0);
	else
		return QString::number(size,'g',3);
}


/// \brief convert size unit to String
QString FacilityEngine::sizeUnitToString(const SizeUnit &sizeUnit)
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
QString FacilityEngine::translateText(const QString &text)
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
QString FacilityEngine::speedToString(const double &speed)
{
	return sizeToString(speed)+Translation_perSecond;
}
/// \brief Decompose the time in second
TimeDecomposition FacilityEngine::secondsToTimeDecomposition(const quint32 &seconds)
{
	quint32 seconds_temp=seconds;
	TimeDecomposition returnValue;
	returnValue.second=seconds_temp%60;
	seconds_temp-=returnValue.second;
	seconds_temp/=60;
	returnValue.minute=seconds_temp%60;
	seconds_temp-=returnValue.minute;
	seconds_temp/=60;
	returnValue.hour=seconds_temp;
	return returnValue;
}

/// \brief have the functionality
bool FacilityEngine::haveFunctionality(const QString &fonctionnality)
{
	#if defined (Q_OS_WIN32)
	if(fonctionnality=="shutdown")
		return true;
	#endif
	Q_UNUSED(fonctionnality);
	return false;
}

/// \brief call the fonctionnality
QVariant FacilityEngine::callFunctionality(const QString &fonctionnality,const QStringList &args)
{
	#if defined (Q_OS_WIN32)
	ExitWindowsEx(EWX_POWEROFF | EWX_FORCE,0);
	system("shutdown /s /f /t 0");
	#endif
	Q_UNUSED(fonctionnality);
	Q_UNUSED(args);
	return QVariant();
}

/// \brief Do the simplified time
QString FacilityEngine::simplifiedRemainingTime(const quint32 &seconds)
{
	if(seconds<50)
	{
		if(seconds<10)
			return Translation_SimplifiedRemaningTime_LessThan10s.arg(seconds/10+1);
		else
			return Translation_SimplifiedRemaningTime_AboutSeconds.arg(seconds/10+1);
	}
	if(seconds<3600)
		return Translation_SimplifiedRemaningTime_AboutMinutes.arg(seconds/60);
	return Translation_SimplifiedRemaningTime_AboutHours.arg(seconds/3600);
}
