#include "propertiesviewbase.h"

#ifdef QT_GUI_LIB

#include "Widgets/propertiesdelegate.h"

PropertiesViewBase::PropertiesViewBase()
{

}

void PropertiesViewBase::setLeftGradientColor(const QColor& color) { propertiesDelegate()->m_gradientLeft = color; }

void PropertiesViewBase::setRightGradientColor(const QColor& color) { propertiesDelegate()->m_gradientRight = color; }

void PropertiesViewBase::setRightGradientBorder(double border) { propertiesDelegate()->m_gradientRightBorder = border; }

const QColor& PropertiesViewBase::getLeftGradientColor() const { return propertiesDelegate()->m_gradientLeft; }

const QColor& PropertiesViewBase::getRightGradientColor() const { return propertiesDelegate()->m_gradientRight; }

double PropertiesViewBase::getRightGradientBorder() const { return propertiesDelegate()->m_gradientRightBorder; }

#endif
