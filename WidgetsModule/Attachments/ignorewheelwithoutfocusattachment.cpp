#include "ignorewheelwithoutfocusattachment.h"

#include <QDoubleSpinBox>
#include <QComboBox>

IgnoreWheelWithoutFocusAttachment::IgnoreWheelWithoutFocusAttachment()
{

}

void IgnoreWheelWithoutFocusAttachment::Attach(QWidget* widget)
{
    widget->installEventFilter(&getInstance());
    widget->setFocusPolicy(Qt::ClickFocus);
}

void IgnoreWheelWithoutFocusAttachment::AttachRecursive(QWidget* widget, const std::function<bool (QWidget* w)>& filter)
{
    auto childWidgets = widget->findChildren<QWidget*>();
    for(auto* childWidget : childWidgets) {
        if(childWidget != nullptr && filter(childWidget)) {
            childWidget->installEventFilter(&getInstance());
            childWidget->setFocusPolicy(Qt::ClickFocus);
        }
    }
}

void IgnoreWheelWithoutFocusAttachment::AttachRecursiveSpinBoxesAndComboBoxes(QWidget* widget)
{
    AttachRecursive(widget, [](QWidget* w) -> bool {
        return qobject_cast<QSpinBox*>(w) != nullptr || qobject_cast<QDoubleSpinBox*>(w) != nullptr || qobject_cast<QComboBox*>(w);
    });
}

bool IgnoreWheelWithoutFocusAttachment::eventFilter(QObject* watched, QEvent* event)
{
    if(event->type() == QEvent::Wheel) {
        auto* widget = qobject_cast<QWidget*>(watched);
        if(widget != nullptr && !widget->hasFocus()) {
            event->ignore();
            return true;
        }
    }
    return false;
}

IgnoreWheelWithoutFocusAttachment& IgnoreWheelWithoutFocusAttachment::getInstance()
{
    static IgnoreWheelWithoutFocusAttachment result;
    return result;
}