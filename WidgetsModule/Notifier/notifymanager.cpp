﻿
#include <QDesktopWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QTimer>
#include <SharedModule/internal.hpp>

#include "notifywidget.h"
#include "notifymanager.h"

NotifyManager::NotifyManager(QObject *parent)
    : QObject(parent)
    , BottomMargin(10)
    , RightMargin(10)
    , Spacing(20)
    , DisplayTime(3 * 1000)
    , Width(300)
    , Height(50)
    , ReservedHeight(300)
    , IsNotifactionsEnabled(true)
    , m_freeHeight(QApplication::desktop()->availableGeometry().height() - ReservedHeight)
    , m_exceedData(::make_shared<NotifyData>(Warning, ""))
    , m_exceedCounter(0)
{
    m_onLayoutChanged.Subscribe({ &BottomMargin.OnChange, &RightMargin.OnChange, &Spacing.OnChange, &Width.OnChange, &ReservedHeight.OnChange });
    m_onLayoutChanged.Connect(this, [this]{
        rearrange();
    });
}

NotifyManager::~NotifyManager()
{
}

void NotifyManager::Notify(NotifyManager::MessageType messageType, const QString& body)
{
    if(!IsNotifactionsEnabled) {
        return;
    }
    auto formatedBody = body;
    formatedBody.replace("\\n", "<br/>");
    auto data = ::make_shared<NotifyData>(messageType, formatedBody);

    if(m_dataQueue.size() < 10) {
        m_exceedCounter = 0;
        m_dataQueue.enqueue(data);
    } else if(m_exceedCounter == 0) {
        m_dataQueue.enqueue(m_exceedData);
        m_exceedCounter++;
    } else {
        m_exceedCounter++;
    }
    OnDataRecieved(data);
    showNext();
}

void NotifyManager::Notify(QtMsgType qtMessageType, const QString& body)
{
    switch (qtMessageType) {
    case QtWarningMsg: Notify(NotifyManager::Warning, body); break;
    case QtCriticalMsg: Notify(NotifyManager::Error, body); break;
    case QtInfoMsg: Notify(NotifyManager::Info, body); break;
    default: break;
    }
}

NotifyManager& NotifyManager::GetInstance()
{
    static NotifyManager manager;
    return manager;
}

void NotifyManager::rearrange()
{
    QDesktopWidget *desktop = QApplication::desktop();
    QRect desktopRect = desktop->availableGeometry();
    QPoint bottomRignt = desktopRect.bottomRight();

    qint32 index = 1;
    qint32 height = 0;
    for(NotifyWidget* notifyWidget : m_notifyList) {
        height += notifyWidget->height() + Spacing;
        QPoint pos = bottomRignt - QPoint(Width + RightMargin, height + BottomMargin);
        notifyWidget->setFixedWidth(Width);
        //notifyWidget->setFixedHeight(Height);
        notifyWidget->setProperty("pos", pos);
        QPropertyAnimation *animation = new QPropertyAnimation(notifyWidget, "pos", this);
        animation->setStartValue(notifyWidget->pos());
        animation->setEndValue(pos);
        animation->setDuration(300);
        animation->start();

        connect(animation, &QPropertyAnimation::finished, this, [animation, this](){
            animation->deleteLater();
        });
        index++;
    }

    m_freeHeight = bottomRignt.y() - height - ReservedHeight;
}

void NotifyManager::showNext()
{
    if(m_freeHeight < 0 || m_dataQueue.isEmpty()) {
        return;
    }

    auto data = m_dataQueue.dequeue();

    if(data == m_exceedData) {
        m_exceedData->Body = tr("Over %1 messages skipped due to exceeding the queue").arg(m_exceedCounter);
    }

    NotifyWidget* notify = new NotifyWidget(data);
    OnLinkActivated.ConnectFrom(notify->OnLinkActivated);
    notify->setFixedWidth(Width);
    //notify->setFixedHeight(Height);

    QDesktopWidget* desktop = QApplication::desktop();
    QRect desktopRect = desktop->availableGeometry();

    notify->ShowGriant(DisplayTime);

    qint32 usedSpace = notify->height() + Spacing;
    QPoint bottomRignt = desktopRect.bottomRight();
    QPoint pos = bottomRignt - QPoint(notify->width() + RightMargin, (bottomRignt.y() - m_freeHeight - ReservedHeight) + usedSpace + BottomMargin);

    // TODO. Due to limitations of OS and Qt. Synchronize this values with styleSheets borders
    QPainterPath path;
    path.addRoundedRect(notify->rect(), 3.0, 3.0);
    notify->setMask(path.toFillPolygon().toPolygon());

    m_freeHeight -= usedSpace;

    notify->move(pos);
    m_notifyList.append(notify);

    notify->OnDisappeared.Connect(this, [notify, this](){
        m_notifyList.removeOne(notify);
        rearrange();
        showNext();

        notify->deleteLater();
    });
}

