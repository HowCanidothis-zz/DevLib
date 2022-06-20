#ifndef DISTANCEDECLARATIONS_H
#define DISTANCEDECLARATIONS_H

#include "UnitsModule/measurementdeclarations.h"

namespace DistanceUnits
{
    DECLARE_MEASUREMENT_UNIT(Meters)
    DECLARE_MEASUREMENT_UNIT(Milimeters)
    DECLARE_MEASUREMENT_UNIT(Centimeters)
    DECLARE_MEASUREMENT_UNIT(Kilometers)
    DECLARE_MEASUREMENT_UNIT(Feets)
    DECLARE_MEASUREMENT_UNIT(USFeets)
    DECLARE_MEASUREMENT_UNIT(Inches)
    DECLARE_MEASUREMENT_UNIT(Miles)
    DECLARE_MEASUREMENT_UNIT(OnePerThirtyTwoInches)
};

DECLARE_MEASUREMENT(MeasurementDistance)
DECLARE_MEASUREMENT(MeasurementDiameter)
DECLARE_MEASUREMENT(MeasurementJetDiameter)
DECLARE_MEASUREMENT(MeasurementCutterDiameter)

#endif // DISTANCEDECLARATIONS_H
