#include "progressbarattachment.h"

#include "WidgetsModule/ProgressBars/mainprogressbar.h"
#include "WidgetsModule/ProgressBars/shadowprogressbar.h"

ProgressBarAttachment::ProgressBarAttachment(QWidget* target)
    : m_target(target)
{

}

void ProgressBarAttachment::EnableProgressBar()
{
    m_progressBarPane = new MainProgressBar(m_target, Qt::Window | Qt::FramelessWindowHint);
    m_progressBarPane->installEventFilter(this);
}

void ProgressBarAttachment::EnableShadowProgressBar()
{
    m_shadowProgressBarPane = new ShadowProgressBar(m_target, Qt::Window | Qt::FramelessWindowHint);
    m_shadowProgressBarPane->setAttribute(Qt::WA_NoSystemBackground);
    m_shadowProgressBarPane->setAttribute(Qt::WA_TranslucentBackground);
    m_shadowProgressBarPane->installEventFilter(this);

    m_shadowProgressBarPane->show();
}

bool ProgressBarAttachment::eventFilter(QObject* watched, QEvent* event)
{
    if(event->type() == QEvent::Close) {
        // auto closeEvent = static_cast<QCloseEvent*>(event);
        if(watched == m_progressBarPane) {
            event->ignore();
            m_target->close();
            return true;
        }
    }
    return false;
}
