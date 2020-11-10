#include "gtview.h"

#include <QPainter>
#include <QResizeEvent>

#include "gtrenderercontroller.h"

GtView::GtView(QWidget* parent, Qt::WindowFlags flags)
    : Super(parent, flags)
    , m_controller(nullptr)
{
    setMouseTracking(true);
}


void GtView::paintEvent(QPaintEvent* )
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.drawImage(rect(), m_controller->GetCurrentImage());
    painter.drawText(QRect(0,0,100,200), QString::number(1000000000.0 / m_controller->GetRenderTime()));
}

GtView::~GtView()
{

}

void GtView::SetController(const GtRendererControllerPtr& controller)
{
    Q_ASSERT(m_controller == nullptr);
    m_controller = controller;
    connect(m_controller.get(), SIGNAL(imageUpdated()), this, SLOT(update()));
}

void GtView::mouseMoveEvent(QMouseEvent* event)
{
    m_controller->MouseMoveEvent(event);
}

void GtView::mousePressEvent(QMouseEvent* event)
{
    m_controller->MousePressEvent(event);
}

void GtView::mouseReleaseEvent(QMouseEvent* event)
{
    m_controller->MouseReleaseEvent(event);
}

void GtView::wheelEvent(QWheelEvent* event)
{
    m_controller->WheelEvent(event);
}

void GtView::keyPressEvent(QKeyEvent *event)
{
    m_controller->KeyPressEvent(event);
}

void GtView::resizeEvent(QResizeEvent* event)
{
    m_controller->Resize(event->size().width(), event->size().height());
}

void GtView::keyReleaseEvent(QKeyEvent *event)
{
    m_controller->KeyReleaseEvent(event);
}

