/** \file FacilityEngine.h
\brief To implement the facility engine, the interface is defined into FacilityInterface()
\see FacilityInterface()
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef FACILITYENGINE_H
#define FACILITYENGINE_H

#include <string>
#include <vector>
#include <unordered_map>

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
    std::string sizeToString(const double &size) const;
    /// \brief convert size unit to String
    std::string sizeUnitToString(const Ultracopier::SizeUnit &sizeUnit) const;
    /// \brief translate the text
    std::string translateText(const std::string &text) const;
    /// \brief speed to string in byte per seconds
    std::string speedToString(const double &speed) const;
    /// \brief Decompose the time in second
    Ultracopier::TimeDecomposition secondsToTimeDecomposition(const uint32_t &seconds) const;
    /// \brief have the fonctionnality
    bool haveFunctionality(const std::string &fonctionnality) const;
    /// \brief call the fonctionnality
    std::string callFunctionality(const std::string &fonctionnality,const std::vector<std::string> &args=std::vector<std::string>());
    /// \brief Do the simplified time
    std::string simplifiedRemainingTime(const uint32_t &seconds) const;
    /// \brief Return ultimate url, empty is not found or already ultimate
    std::string ultimateUrl() const;
    /// \brief Return the software name
    std::string softwareName() const;
    /// \brief separator native to the current OS
    static std::string separator();

    static FacilityEngine facilityEngine;
private:
    //undirect translated string
    std::string Translation_perSecond;
    std::string Translation_tooBig;
    std::string Translation_B;
    std::string Translation_KB;
    std::string Translation_MB;
    std::string Translation_GB;
    std::string Translation_TB;
    std::string Translation_PB;
    std::string Translation_EB;
    std::string Translation_ZB;
    std::string Translation_YB;
    //simplified remaining time
    std::string Translation_SimplifiedRemaningTime_LessThan10s;
    std::string Translation_SimplifiedRemaningTime_AboutSeconds;
    std::string Translation_SimplifiedRemaningTime_AboutMinutes;
    std::string Translation_SimplifiedRemaningTime_AboutHours;
    //internal fonction
    inline std::string adaptString(const float &nb) const;
    std::unordered_map<std::string,std::string> translations;
public slots:
    /// \brief To force the text re-translation
    void retranslate();
};

#endif // FACILITYENGINE_H
