/** \file FacilityInterface.h
\brief Define the class of the facility engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef FACILITY_INTERFACE_H
#define FACILITY_INTERFACE_H

#include <string>
#include <vector>
#include <QObject>
#include <QBuffer>

#include "../StructEnumDefinition.h"

/// \brief To define the interface, to pass the facility object from Ultracopier to the plugins without compatibility problem
//not possible to be static, because in the plugin it's not resolved
class FacilityInterface : public QObject
{
    public:
        /// \brief To force the text re-translation
        virtual void retranslate() = 0;
        /// \brief convert size in Byte to String
        virtual std::string sizeToString(const double &size) const = 0;
        /// \brief convert size unit to String
        virtual std::string sizeUnitToString(const Ultracopier::SizeUnit &sizeUnit) const = 0;
        /// \brief translate the text
        virtual std::string translateText(const std::string &text) const = 0;
        /// \brief speed to string in byte per seconds
        virtual std::string speedToString(const double &speed) const = 0;
        /// \brief Decompose the time in second
        virtual Ultracopier::TimeDecomposition secondsToTimeDecomposition(const uint32_t &seconds) const = 0;
        /// \brief have the fonctionnality
        virtual bool haveFunctionality(const std::string &fonctionnality) const = 0;
        /// \brief call the fonctionnality
        virtual std::string callFunctionality(const std::string &fonctionnality,const std::vector<std::string> &args=std::vector<std::string>()) = 0;
        /// \brief Do the simplified time
        virtual std::string simplifiedRemainingTime(const uint32_t &seconds) const = 0;
        /// \brief Do the simplified time
        virtual std::string ultimateUrl() const = 0;
        /// \brief Return the software name
        virtual std::string softwareName() const = 0;
        /// \brief return if is ultimate
        virtual bool isUltimate() const = 0;
        /// \brief return audio if created from opus file, nullptr if failed
        virtual void/*casted to #ifndef QAudioOutput*/* prepareOpusAudio(const std::string &file,QBuffer &buffer) const = 0;
};

#endif // FACILITY_INTERFACE_H
