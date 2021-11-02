#ifndef YIELDPOINTDECLARATIONS_H
#define YIELDPOINTDECLARATIONS_H

#include "UnitsModule/measurementunitmanager.h"
static const Name MEASUREMENT_YIELD_POINT = "Yield Point";

namespace YieldPointUnits
{
    static const MeasurementUnit PoundForcePerSquareFoot("PoundForcePerSquareFoot", []{return QObject::tr("pound force per square foot");}, []{ return QObject::tr("lbf/100ft2"); }, 1.0);
    static const MeasurementUnit Pascal("Pascal", []{return QObject::tr("pascal");}, []{ return QObject::tr("Pa");; }, 0.02088547);
};

#define MEASUREMENT_YIELD_POINT_UNIT_TO_BASE(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_YIELD_POINT)->FromUnitToBase(x)
#define MEASUREMENT_YIELD_POINT_BASE_TO_UNIT(x) \
    MeasurementManager::GetInstance().GetCurrentUnit(MEASUREMENT_YIELD_POINT)->FromBaseToUnit(x)
#define MEASUREMENT_YIELD_POINT_PRECISION() \
    MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_YIELD_POINT)->CurrentPrecision
#define MEASUREMENT_YIELD_POINT_BASE_TO_UNIT_UI(x) \
    QString::number(MEASUREMENT_YIELD_POINT_BASE_TO_UNIT(x), 'f', MEASUREMENT_YIELD_POINT_PRECISION())

#define MEASUREMENT_YIELD_POINT_STRING MeasurementManager::GetInstance().GetMeasurement(MEASUREMENT_YIELD_POINT)->CurrentUnitLabel
#define ATTACH_YIELD_POINT_MEASUREMENT(delegate, min, max, step) \
    ATTACH_MEASUREMENT(MEASUREMENT_YIELD_POINT, delegate, min, max, step)



#endif // YIELDPOINTDECLARATIONS_H
