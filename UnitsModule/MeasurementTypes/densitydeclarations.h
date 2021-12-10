#ifndef DENSITYDECLARATIONS_H
#define DENSITYDECLARATIONS_H

#include "UnitsModule/measurementunitmanager.h"
static const Name MEASUREMENT_DENSITY = "Density";

namespace DensityUnits
{
    static const MeasurementUnit KilogramsPerCubicMeters("KilogramsPerCubicMeters", []{return QObject::tr("kilograms/cubic meters");}, []{ return QObject::tr("kg/m3"); }, 1.0);
    static const MeasurementUnit KilogramsPerLiter("KilogramsPerLiter", []{return QObject::tr("kilograms/liter");}, []{ return QObject::tr("kg/l"); }, 1000.0);
    static const MeasurementUnit PoundsPerGallon("PoundsPerGallon", []{return QObject::tr("pounds/gallon");}, []{ return QObject::tr("lb/gal"); }, 119.826427);
    static const MeasurementUnit PoundsPerCubicFeet("PoundsPerCubicFeet", []{return QObject::tr("pounds/cubic feet");}, []{ return QObject::tr("lb/ft3"); }, 16.018463);
    static const MeasurementUnit MilligrammPerLiter("MilligrammPerLiter", []{return QObject::tr("milligrams/liter");}, []{ return QObject::tr("mg/L"); }, 0.001);
};

#define MEASUREMENT_DENSITY_UNIT_TO_BASE(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_DENSITY)->FromUnitToBase(x)
#define MEASUREMENT_DENSITY_BASE_TO_UNIT(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_DENSITY)->FromBaseToUnit(x)
#define MEASUREMENT_DENSITY_PRECISION() \
    MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_DENSITY)->CurrentPrecision
#define MEASUREMENT_DENSITY_BASE_TO_UNIT_UI(x) \
    QString::number(MEASUREMENT_DENSITY_BASE_TO_UNIT(x), 'f', MEASUREMENT_DENSITY_PRECISION())

#define MEASUREMENT_DENSITY_STRING MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_DENSITY)->CurrentUnitLabel

#endif // DENSITYDECLARATIONS_H