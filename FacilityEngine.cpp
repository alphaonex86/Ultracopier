/** \file FacilityEngine.cpp
\brief To implement the facility engine, the interface is defined into FacilityInterface()
\see FacilityInterface()
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "FacilityEngine.h"
#include "ProductKey.h"

#if defined (Q_OS_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif
#ifndef NOAUDIO
#include "opusfile/opusfile.h"
#include <QAudioOutput>
#endif

FacilityEngine FacilityEngine::facilityEngine;

FacilityEngine::FacilityEngine()
{
    retranslate();
}

/// \brief separator native to the current OS
std::string FacilityEngine::separator()
{
    #ifdef Q_OS_WIN32
    return "\\";
    #else
    return "/";
    #endif
}

/// \brief To force the text re-translation
void FacilityEngine::retranslate()
{
    //undirect translated string
    Translation_perSecond="/"+tr("s").toStdString();
    Translation_tooBig=tr("Too big").toStdString();
    Translation_B=tr("B").toStdString();
    Translation_KB=tr("KB").toStdString();
    Translation_MB=tr("MB").toStdString();
    Translation_GB=tr("GB").toStdString();
    Translation_TB=tr("TB").toStdString();
    Translation_PB=tr("PB").toStdString();
    Translation_EB=tr("EB").toStdString();
    Translation_ZB=tr("ZB").toStdString();
    Translation_YB=tr("YB").toStdString();
    Translation_SimplifiedRemaningTime_LessThan10s=tr("Less than %10 seconds").toStdString();
    Translation_SimplifiedRemaningTime_AboutSeconds=tr("About %10 seconds remaining").toStdString();
    Translation_SimplifiedRemaningTime_AboutMinutes=tr("About %1 minutes remaining").toStdString();
    Translation_SimplifiedRemaningTime_AboutHours=tr("About %1 hours remaining").toStdString();
    //load the translations tab
    translations["Copy engine"]=tr("Copy engine").toStdString();
    //: a copy
    translations["Copy"]=tr("Copy").toStdString();
    //: a transfer
    translations["Transfer"]=tr("Transfer").toStdString();
    //: a move
    translations["Move"]=tr("Move").toStdString();
    translations["Start"]=tr("Start").toStdString();
    translations["Pause"]=tr("Pause").toStdString();
    translations["Resume"]=tr("Resume").toStdString();
    translations["Skip"]=tr("Skip").toStdString();
    translations["Unlimited"]=tr("Unlimited").toStdString();
    translations["Source"]=tr("Source").toStdString();
    translations["Size"]=tr("Size").toStdString();
    translations["Destination"]=tr("Destination").toStdString();
    translations["Quit"]=tr("Quit").toStdString();
    translations["Target"]=tr("Target").toStdString();
    translations["Time remaining:"]=tr("Time remaining:").toStdString();
    translations["Listing"]=tr("Listing").toStdString();
    translations["Copying"]=tr("Copying").toStdString();
    translations["Listing and copying"]=tr("Listing and copying").toStdString();
    translations["Time remaining:"]=tr("Time remaining:").toStdString();
    translations["Remaining:"]=tr("Remaining:").toStdString();
    //for copy engine
    translations["Ask"]=tr("Ask").toStdString();
    translations["Skip"]=tr("Skip").toStdString();
    translations["Overwrite"]=tr("Overwrite").toStdString();
    translations["Overwrite if newer"]=tr("Overwrite if newer").toStdString();
    translations["Overwrite if the last modification dates are different"]=tr("Overwrite if the last modification dates are different").toStdString();
    translations["Rename"]=tr("Rename").toStdString();
    translations["Put to the end of the list"]=tr("Put to the end of the list").toStdString();
    translations["Select source directory"]=tr("Select source directory").toStdString();
    translations["Select destination directory"]=tr("Select destination directory").toStdString();
    translations["Internal error"]=tr("Internal error").toStdString();
    translations["Select one or more files to open"]=tr("Select one or more files to open").toStdString();
    translations["All files"]=tr("All files").toStdString();
    translations["Save transfer list"]=tr("Save transfer list").toStdString();
    translations["Open transfer list"]=tr("Open transfer list").toStdString();
    translations["Transfer list"]=tr("Transfer list").toStdString();
    translations["Error"]=tr("Error").toStdString();
    translations["Not supported on this platform"]=tr("Not supported on this platform").toStdString();
    translations["Completed in %1"]=tr("Completed in %1").toStdString();
}

/// \brief convert size in Byte to String
std::string FacilityEngine::sizeToString(const double &size) const
{
    double size_temp=size;
    if(size_temp<1024)
        return std::to_string((unsigned int)size_temp)+sizeUnitToString(Ultracopier::SizeUnit_byte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_KiloByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_MegaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_GigaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_TeraByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_PetaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_ExaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_ZettaByte);
    if((size_temp=size_temp/1024)<1024)
        return adaptString(static_cast<float>(size_temp))+sizeUnitToString(Ultracopier::SizeUnit_YottaByte);
    return Translation_tooBig;
}

std::string FacilityEngine::adaptString(const float &size) const
{
    if(size>=100)
        return QString::number(static_cast<double>(size),'f',0).toStdString();
    else
        return QString::number(static_cast<double>(size),'g',3).toStdString();
}


/// \brief convert size unit to String
std::string FacilityEngine::sizeUnitToString(const Ultracopier::SizeUnit &sizeUnit) const
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"sizeUnit: "+std::to_string(sizeUnit));
            return "???";
    }
}

/// \brief translate the text
std::string FacilityEngine::translateText(const std::string &text) const
{
    if(translations.find(text)!=translations.cend())
        return translations.at(text);
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"translation not found: "+text);
        return text;
    }
}

/// \brief speed to string in byte per seconds
std::string FacilityEngine::speedToString(const double &speed) const
{
    return sizeToString(speed)+Translation_perSecond;
}
/// \brief Decompose the time in second
Ultracopier::TimeDecomposition FacilityEngine::secondsToTimeDecomposition(const uint32_t &seconds) const
{
    quint32 seconds_temp=seconds;
    Ultracopier::TimeDecomposition returnValue;
    returnValue.second=static_cast<uint16_t>(seconds_temp%60);
    seconds_temp-=returnValue.second;
    seconds_temp/=60;
    returnValue.minute=static_cast<uint16_t>(seconds_temp%60);
    seconds_temp-=returnValue.minute;
    seconds_temp/=60;
    returnValue.hour=static_cast<uint16_t>(seconds_temp);
    return returnValue;
}

/// \brief have the functionality
bool FacilityEngine::haveFunctionality(const std::string &fonctionnality) const
{
    #if defined (Q_OS_WIN32)
    if(fonctionnality=="shutdown")
        return true;
    #endif
    Q_UNUSED(fonctionnality);
    return false;
}

/// \brief call the fonctionnality
std::string FacilityEngine::callFunctionality(const std::string &fonctionnality,const std::vector<std::string> &args)
{
    #if defined (Q_OS_WIN32)
    ExitWindowsEx(EWX_POWEROFF | EWX_FORCE,0);
    system("shutdown /s /f /t 0");
    #endif
    Q_UNUSED(fonctionnality);
    Q_UNUSED(args);
    return std::string();
}

/// \brief Do the simplified time
std::string FacilityEngine::simplifiedRemainingTime(const uint32_t &seconds) const
{
    if(seconds<50)
    {
        if(seconds<10)
            return QString::fromStdString(Translation_SimplifiedRemaningTime_LessThan10s).arg(seconds/10+1).toStdString();
        else
            return QString::fromStdString(Translation_SimplifiedRemaningTime_AboutSeconds).arg(seconds/10+1).toStdString();
    }
    if(seconds<3600)
        return QString::fromStdString(Translation_SimplifiedRemaningTime_AboutMinutes).arg(seconds/60).toStdString();
    return QString::fromStdString(Translation_SimplifiedRemaningTime_AboutHours).arg(seconds/3600).toStdString();
}

/// \brief Return ultimate url, empty is not found or already ultimate
std::string FacilityEngine::ultimateUrl() const
{
    #ifndef ULTRACOPIER_LITTLE
    if(ProductKey::productKey->isUltimate())
        return std::string();
    else
    {
        #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
        return "https://shop.first-world.info/";
        #else
        return std::string();
        #endif
    }
    #else
    return std::string();
    #endif
}

bool FacilityEngine::isUltimate() const
{
    #ifndef ULTRACOPIER_LITTLE
    return ProductKey::productKey->isUltimate();
    #else
    return true;
    #endif
}

/// \brief Return the software name
std::string FacilityEngine::softwareName() const
{
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
    return "Supercopier";
    #else
    return "Ultracopier";
    #endif
}

/// \brief return audio if created from opus file, nullptr if failed
void *FacilityEngine::prepareOpusAudio(const std::string &file,QBuffer &buffer) const
{
    #ifndef NOAUDIO
    if(file.empty())
        return nullptr;

    QAudioOutput* audio;
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"raw audio format not supported by backend, cannot play audio.");
        return nullptr;
    }
    audio = new QAudioOutput(format);
    buffer.open(QBuffer::ReadWrite);

    int           ret;
    std::string path=file;
    if(path.find("/") == std::string::npos && path.find("\\") == std::string::npos)
    {
        QString appPath=QCoreApplication::applicationDirPath();
        if(appPath.endsWith("/") || appPath.endsWith("\\"))
            path=appPath.toStdString()+path;
        else
            path=appPath.toStdString()+"/"+path;
    }
    OggOpusFile  *of=op_open_file(file.c_str(),&ret);
    if(of==NULL) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Failed to open file"+file+", "+std::to_string(ret));
        return nullptr;
    }
    ogg_int64_t pcm_offset;
    ogg_int64_t nsamples;
    nsamples=0;
    pcm_offset=op_pcm_tell(of);
    if(pcm_offset!=0)
        fprintf(stderr,"Non-zero starting PCM offset: %li\n",(long)pcm_offset);
    for(;;) {
        ogg_int64_t   next_pcm_offset;
        opus_int16    pcm[120*48*2];
        unsigned char out[120*48*2*2];
        int           si;
        ret=op_read_stereo(of,pcm,sizeof(pcm)/sizeof(*pcm));
        if(ret==OP_HOLE) {
            fprintf(stderr,"\nHole detected! Corrupt file segment?\n");
            continue;
        }
        else if(ret<0) {
            fprintf(stderr,"\nError decoding '%s': %i\n","file.opus",ret);
            ret=EXIT_FAILURE;
            break;
        }
        next_pcm_offset=op_pcm_tell(of);
        pcm_offset=next_pcm_offset;
        if(ret<=0) {
            ret=EXIT_SUCCESS;
            break;
        }
        for(si=0;si<2*ret;si++) { /// Ensure the data is little-endian before writing it out.
            out[2*si+0]=(unsigned char)(pcm[si]&0xFF);
            out[2*si+1]=(unsigned char)(pcm[si]>>8&0xFF);
        }
        buffer.write(reinterpret_cast<char *>(out),sizeof(*out)*4*ret);
        nsamples+=ret;
    }
    if(ret==EXIT_SUCCESS)
        fprintf(stderr,"\nDone: played ");
    op_free(of);

    buffer.seek(0);
    return audio;
    // audio->start(&buffer); -> do out of this function
    #else
    (void)file;
    (void)buffer;
    return nullptr;
    #endif
}
