/** \file FacilityEngine.cpp
\brief To implement the facility engine, the interface is defined into FacilityInterface()
\see FacilityInterface()
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "FacilityEngine.h"

#if defined (Q_OS_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

FacilityEngine FacilityEngine::facilityEngine;

FacilityEngine::FacilityEngine()
{
    retranslate();
}

/// \brief To force the text re-translation
void FacilityEngine::retranslate()
{
    //undirect translated string
    Translation_perSecond=QStringLiteral("/")+tr("s");
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
    //load the translations tab
    translations["Copy engine"]=tr("Copy engine");
    //: a copy
    translations["Copy"]=tr("Copy");
    //: a transfer
    translations["Transfer"]=tr("Transfer");
    //: a move
    translations["Move"]=tr("Move");
    translations["Start"]=tr("Start");
    translations["Pause"]=tr("Pause");
    translations["Resume"]=tr("Resume");
    translations["Skip"]=tr("Skip");
    translations["Unlimited"]=tr("Unlimited");
    translations["Source"]=tr("Source");
    translations["Size"]=tr("Size");
    translations["Destination"]=tr("Destination");
    translations["Quit"]=tr("Quit");
    translations["Target"]=tr("Target");
    translations["Time remaining:"]=tr("Time remaining:");
    translations["Listing"]=tr("Listing");
    translations["Copying"]=tr("Copying");
    translations["Listing and copying"]=tr("Listing and copying");
    translations["Time remaining:"]=tr("Time remaining:");
    //for copy engine
    translations["Ask"]=tr("Ask");
    translations["Skip"]=tr("Skip");
    translations["Overwrite"]=tr("Overwrite");
    translations["Overwrite if newer"]=tr("Overwrite if newer");
    translations["Overwrite if the last modification dates are different"]=tr("Overwrite if the last modification dates are different");
    translations["Rename"]=tr("Rename");
    translations["Put to the end of the list"]=tr("Put to the end of the list");
    translations["Select source directory"]=tr("Select source directory");
    translations["Select destination directory"]=tr("Select destination directory");
    translations["Internal error"]=tr("Internal error");
    translations["Select one or more files to open"]=tr("Select one or more files to open");
    translations["All files"]=tr("All files");
    translations["Save transfer list"]=tr("Save transfer list");
    translations["Open transfer list"]=tr("Open transfer list");
    translations["Transfer list"]=tr("Transfer list");
    translations["Error"]=tr("Error");
    translations["Not supported on this platform"]=tr("Not supported on this platform");
    translations["Completed in %1"]=tr("Completed in %1");
}

/// \brief convert size in Byte to String
QString FacilityEngine::sizeToString(const double &size) const
{
    double size_temp=size;
    if(size_temp<1024)
        return QString::number(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_byte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_KiloByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_MegaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_GigaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_TeraByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_PetaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_ExaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_ZettaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(size_temp)+sizeUnitToString(Ultracopier::SizeUnit_YottaByte);
    return Translation_tooBig;
}

QString FacilityEngine::adaptString(const float &size) const
{
    if(size>=100)
        return QString::number(size,'f',0);
    else
        return QString::number(size,'g',3);
}


/// \brief convert size unit to String
QString FacilityEngine::sizeUnitToString(const Ultracopier::SizeUnit &sizeUnit) const
{
    switch(sizeUnit)
    {
        case Ultracopier::SizeUnit_byte:
            return Translation_B;
        case Ultracopier::SizeUnit_KiloByte:
            return Translation_KB;
        case Ultracopier::SizeUnit_MegaByte:
            return Translation_MB;
        case Ultracopier::SizeUnit_GigaByte:
            return Translation_GB;
        case Ultracopier::SizeUnit_TeraByte:
            return Translation_TB;
        case Ultracopier::SizeUnit_PetaByte:
            return Translation_PB;
        case Ultracopier::SizeUnit_ExaByte:
            return Translation_EB;
        case Ultracopier::SizeUnit_ZettaByte:
            return Translation_ZB;
        case Ultracopier::SizeUnit_YottaByte:
            return Translation_YB;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"sizeUnit: "+QString::number(sizeUnit));
            return "???";
    }
}

/// \brief translate the text
QString FacilityEngine::translateText(const QString &text) const
{
    if(translations.contains(text))
        return translations.value(text);
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"translation not found: "+text);
        return text;
    }
}

/// \brief speed to string in byte per seconds
QString FacilityEngine::speedToString(const double &speed) const
{
    return sizeToString(speed)+Translation_perSecond;
}
/// \brief Decompose the time in second
Ultracopier::TimeDecomposition FacilityEngine::secondsToTimeDecomposition(const quint32 &seconds) const
{
    quint32 seconds_temp=seconds;
    Ultracopier::TimeDecomposition returnValue;
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
bool FacilityEngine::haveFunctionality(const QString &fonctionnality) const
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
QString FacilityEngine::simplifiedRemainingTime(const quint32 &seconds) const
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

/// \brief Return ultimate url, empty is not found or already ultimate
QString FacilityEngine::ultimateUrl() const
{
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    return QString();
    #else
        #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
        return QStringLiteral("http://ultracopier.first-world.info/shop.html");
        #else
        return QString();
        #endif
    #endif
}

/// \brief Return the software name
QString FacilityEngine::softwareName() const
{
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
        return QStringLiteral("Supercopier");
    #else
        return QStringLiteral("Ultracopier");
    #endif
}
