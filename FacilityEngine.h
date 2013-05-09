/** \file FacilityEngine.h
\brief To implement the facility engine, the interface is defined into FacilityInterface()
\see FacilityInterface()
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef FACILITYENGINE_H
#define FACILITYENGINE_H

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QHash>

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
    QString sizeToString(const double &size) const;
    /// \brief convert size unit to String
    QString sizeUnitToString(const Ultracopier::SizeUnit &sizeUnit) const;
    /// \brief translate the text
    QString translateText(const QString &text) const;
    /// \brief speed to string in byte per seconds
    QString speedToString(const double &speed) const;
    /// \brief Decompose the time in second
    Ultracopier::TimeDecomposition secondsToTimeDecomposition(const quint32 &seconds) const;
    /// \brief have the fonctionnality
    bool haveFunctionality(const QString &fonctionnality) const;
    /// \brief call the fonctionnality
    QVariant callFunctionality(const QString &fonctionnality,const QStringList &args=QStringList());
    /// \brief Do the simplified time
    QString simplifiedRemainingTime(const quint32 &seconds) const;
    /// \brief Return ultimate url, empty is not found or already ultimate
    QString ultimateUrl() const;
private:
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
    //simplified remaining time
    QString Translation_SimplifiedRemaningTime_LessThan10s;
    QString Translation_SimplifiedRemaningTime_AboutSeconds;
    QString Translation_SimplifiedRemaningTime_AboutMinutes;
    QString Translation_SimplifiedRemaningTime_AboutHours;
    //internal fonction
    inline QString adaptString(const float &nb) const;
    QHash<QString,QString> translations;
public slots:
    /// \brief To force the text re-translation
    void retranslate();
};

#endif // FACILITYENGINE_H
