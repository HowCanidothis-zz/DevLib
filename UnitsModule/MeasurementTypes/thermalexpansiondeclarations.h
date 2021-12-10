#ifndef THERMALEXPANSIONDECLARATIONS_H
#define THERMALEXPANSIONDECLARATIONS_H

#include "UnitsModule/measurementunitmanager.h"
static const Name MEASUREMENT_THERMAL_EXPANSION = "Thermal Expansion";

namespace ThermalExpansionUnits
{
    static const MeasurementUnit PerDegreeFahrenheit("PerDegreeFahrenheit", []{return QObject::tr("per degree Fahrenheit");}, []{ return QObject::tr("E-06/F"); }, 1.8);
    static const MeasurementUnit PerDegreeCelsius("PerDegreeCelsius", []{return QObject::tr("per degree Celsius");}, []{ return QObject::tr("E-06/C°"); }, 1.0);
    static const MeasurementUnit PerDegreeKelvin("PerDegreeKelvin", []{return QObject::tr("per degree Kelvin");}, []{ return QObject::tr("E-06/K"); }, 1.0);
};

#define MEASUREMENT_THERMAL_EXPANSION_UNIT_TO_BASE(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_THERMAL_EXPANSION)->FromUnitToBase(x)
#define MEASUREMENT_THERMAL_EXPANSION_BASE_TO_UNIT(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_THERMAL_EXPANSION)->FromBaseToUnit(x)
#define MEASUREMENT_THERMAL_EXPANSION_PRECISION() \
    MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_THERMAL_EXPANSION)->CurrentPrecision
#define MEASUREMENT_THERMAL_EXPANSION_BASE_TO_UNIT_UI(x) \
    QString::number(MEASUREMENT_THERMAL_EXPANSION_BASE_TO_UNIT(x), 'f', MEASUREMENT_THERMAL_EXPANSION_PRECISION())

#define MEASUREMENT_THERMAL_EXPANSION_STRING MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_THERMAL_EXPANSION)->CurrentUnitLabel
#define ATTACH_THERMAL_EXPANSION_MEASUREMENT(delegate, min, max) \
    ATTACH_MEASUREMENT(MEASUREMENT_THERMAL_EXPANSION, delegate, min, max)

#endif // THERMALEXPANSIONDECLARATIONS_H