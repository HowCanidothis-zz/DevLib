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

    Locale.OnChanged.ConnectAndCall(CONNECTION_DEBUG_LOCATION, [this]{
        ui->calendarWidget->setLocale(Locale);
        ui->widget->Locale = Locale.Native();
    });

    WidgetPushButtonWrapper(ui->btnApply).SetControl(ButtonRole::Save);

    WidgetWrapper(ui->btnNow).WidgetVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, NowEnabled);
}

WidgetsDateTimeWidget::~WidgetsDateTimeWidget()
{
    delete ui;
}

DispatcherConnections WidgetsDateTimeWidget::ConnectModel(LocalPropertyDateTime* modelProperty, bool reactive)
{
    DispatcherConnections ret;
    if(reactive) {
        ret += modelProperty->ConnectBoth(CONNECTION_DEBUG_LOCATION,CurrentDateTime,
                                  [](const QDateTime& time){ return time; },
                                  [](const QDateTime& time){ return time; });
    } else {
        CurrentDateTime = modelProperty->Native();
        OnApplyActivate.Connect(CONNECTION_DEBUG_LOCATION, [modelProperty, this]{
            *modelProperty = CurrentDateTime.Native();
        });
    }
    ret += modelProperty->OnMinMaxChanged.Connect(CONNECTION_DEBUG_LOCATION, [this, modelProperty]{
        CurrentDateTime.SetMinMax(modelProperty->GetMin(), modelProperty->GetMax());
    });
    CurrentDateTime.SetMinMax(modelProperty->GetMin(), modelProperty->GetMax());
    return ret;
}
