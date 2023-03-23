#include "widgetsdatetimewidget.h"
#include "ui_widgetsdatetimewidget.h"

#include <QTextCharFormat>

#include "WidgetsModule/Utils/widgethelpers.h"

WidgetsDateTimeWidget::WidgetsDateTimeWidget(QWidget *parent)
	: Super(parent)
    , NowEnabled(true)
    , ui(new Ui::WidgetsDateTimeWidget)
{
	ui->setupUi(this);
	
	auto style = ui->calendarWidget->weekdayTextFormat(Qt::DayOfWeek::Monday);
	ui->calendarWidget->setWeekdayTextFormat(Qt::DayOfWeek::Saturday, style);
    ui->calendarWidget->setWeekdayTextFormat(Qt::DayOfWeek::Sunday, style);
	
    connect(ui->calendarWidget, &QCalendarWidget::clicked, [this](const QDate& date){
        if(CurrentDateTime.IsRealTime() || !ui->widget->CurrentTime.Native().isValid()) {
            CurrentDateTime = QDateTime(date, QTime(0,0));
            return;
        }
        CurrentDateTime = QDateTime(date, CurrentDateTime.Native().time());
	});

    auto updateTimeRangeHandler = [this]{
        auto currentDateTime = QDateTime::currentDateTime();
        ui->calendarWidget->setSelectedDate(CurrentDateTime.Native().date());
        QTime start(0,0), end(23, 59);

        if(!CurrentDateTime.IsRealTime() && CurrentDateTime.GetMin().date() == CurrentDateTime.Native().date()){
            start = CurrentDateTime.GetMin().time();
        }
        if(!CurrentDateTime.IsRealTime() && CurrentDateTime.GetMax().date() == CurrentDateTime.Native().date()){
            end = CurrentDateTime.GetMax().time();
        }
        Q_ASSERT(!end.isValid() || start <= end);
        ui->calendarWidget->setDateRange(CurrentDateTime.GetMinValid().date(), CurrentDateTime.GetMaxValid().date());
        ui->widget->CurrentTime.SetMinMax(start, end);
    };

    CurrentDateTime.SetAndSubscribe(updateTimeRangeHandler);
    CurrentDateTime.OnMinMaxChanged.Connect(CONNECTION_DEBUG_LOCATION, [updateTimeRangeHandler]{
        updateTimeRangeHandler();
	});
	
    CurrentDateTime.ConnectBoth(CONNECTION_DEBUG_LOCATION,ui->widget->CurrentTime, [](const QDateTime& dt){ return dt.time(); },
    [this](const QTime& time){
        return QDateTime(CurrentDateTime.IsRealTime() ? ui->calendarWidget->selectedDate() : CurrentDateTime.Native().date(), time);
    });
	
	connect(ui->btnNow, &QPushButton::clicked, [this](bool){ 
        CurrentDateTime = QDateTime();
        OnNowActivate();
	});
	connect(ui->btnApply, &QPushButton::clicked, [this](bool){
        OnApplyActivate();
	});

    SharedSettings::GetInstance().LanguageSettings.ApplicationLocale.OnChanged.ConnectAndCall(CONNECTION_DEBUG_LOCATION, [this]{
        ui->calendarWidget->setLocale(SharedSettings::GetInstance().LanguageSettings.ApplicationLocale);
    }).MakeSafe(m_connections);

    WidgetPushButtonWrapper(ui->btnApply).SetControl(ButtonRole::Save);
    WidgetWrapper(ui->widget).WidgetVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, [](qint32 mode){
        return mode == DateTime;
    }, Mode);
    WidgetWrapper(ui->btnNow).WidgetVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, NowEnabled);
}

WidgetsDateTimeWidget::~WidgetsDateTimeWidget()
{
    delete ui;
}

void WidgetsDateTimeWidget::showEvent(QShowEvent* event)
{
    OnAboutToShow();
    Super::showEvent(event);
}

DispatcherConnections WidgetsDateTimeWidget::ConnectModel(LocalPropertyDate* modelProperty)
{
    DispatcherConnections ret;
    auto storedDate = ::make_shared<QDate>();
    ret += Store.Connect(CONNECTION_DEBUG_LOCATION, [modelProperty, storedDate]{
        *storedDate = *modelProperty;
    });
    ret += modelProperty->ConnectBoth(CONNECTION_DEBUG_LOCATION,CurrentDateTime,
                              [](const QDate& time){ return QDateTime(time, QTime()); },
                              [](const QDateTime& time){ return time.date(); });
    ret += Reset.Connect(CONNECTION_DEBUG_LOCATION, [this, storedDate]{
        CurrentDateTime = QDateTime(*storedDate, QTime());
    });
    ret += modelProperty->OnMinMaxChanged.ConnectAndCall(CONNECTION_DEBUG_LOCATION, [this, modelProperty]{
        CurrentDateTime.SetMinMax(QDateTime(modelProperty->GetMin(), QTime()), QDateTime(modelProperty->GetMax(), QTime()));
    });

    auto connections = DispatcherConnectionsSafeCreate();
    ret += OnAboutToShow.Connect(CONNECTION_DEBUG_LOCATION, [this, modelProperty, connections]{
        connections->clear();
        auto initialDate = modelProperty->Native();
        if(!initialDate.isValid()) initialDate = QDate::currentDate();
        ui->calendarWidget->setCurrentPage(initialDate.year(), initialDate.month());
        WidgetPushButtonWrapper(ui->btnApply).WidgetVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, [initialDate](const QDate& t){
            return initialDate != t;
        }, *modelProperty).MakeSafe(*connections);
    });
    return ret;
}

DispatcherConnections WidgetsDateTimeWidget::ConnectModel(LocalPropertyDateTime* modelProperty)
{
    DispatcherConnections ret;
    auto storedDate = ::make_shared<QDateTime>();
    ret += Store.Connect(CONNECTION_DEBUG_LOCATION, [modelProperty, storedDate]{
        *storedDate = *modelProperty;
    });
    ret += modelProperty->ConnectBoth(CONNECTION_DEBUG_LOCATION, CurrentDateTime);
    ret += Reset.Connect(CONNECTION_DEBUG_LOCATION, [this, storedDate]{
        CurrentDateTime = *storedDate;
    });
    ret += modelProperty->OnMinMaxChanged.ConnectAndCall(CONNECTION_DEBUG_LOCATION, [this, modelProperty]{
        CurrentDateTime.SetMinMax(modelProperty->GetMin(), modelProperty->GetMax());
    });

    auto connections = DispatcherConnectionsSafeCreate();
    ret += OnAboutToShow.Connect(CONNECTION_DEBUG_LOCATION, [this, modelProperty, connections]{
        connections->clear();
        auto initialDateTime = modelProperty->Native();
        auto date = initialDateTime.date();
        if(!date.isValid()) date = QDate::currentDate();
        ui->calendarWidget->setCurrentPage(date.year(), date.month());
        WidgetPushButtonWrapper(ui->btnApply).WidgetVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, [initialDateTime](const QDateTime& dt){
            return initialDateTime != dt;
        }, *modelProperty).MakeSafe(*connections);
    });
    return ret;
}
