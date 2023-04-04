#include "widgetsdatetimepopuppicker.h"
#include "ui_widgetsdatetimepopuppicker.h"

#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>
#include <QKeyEvent>

#include "WidgetsModule/Utils/widgethelpers.h"
#include "widgetsdatetimewidget.h"

WidgetsDatetimePopupPicker::WidgetsDatetimePopupPicker(QWidget *parent)
    : Super(parent)
    , ui(new Ui::WidgetsDatetimePopupPicker)
{
    ui->setupUi(this);
    setFocusProxy(ui->dateTimeEdit);
//    ui->CalendarButton->setIcon(IconsManager::GetInstance().GetIcon("CalendarIcon"));
    
    auto* menu = MenuWrapper(ui->CalendarButton).AddPreventedFromClosingMenu(tr("DateTime"));
    auto* ac = new QWidgetAction(parent);
    m_editor = new WidgetsDateTimeWidget(parent);
    m_editor->TimeShift.ConnectFrom(CONNECTION_DEBUG_LOCATION, TimeShift);
    
    ac->setDefaultWidget(m_editor);
    
    menu->addAction(ac);
    connect(ui->CalendarButton, &QPushButton::clicked, [menu, this](bool){
        menu->setProperty("a_accept", false);
        m_editor->Store();
        menu->exec(ui->dateTimeEdit->mapToGlobal(ui->dateTimeEdit->geometry().bottomLeft()));
        if(menu->property("a_accept").toBool()) {
            OnDataCommit();
        } else {
            m_editor->Reset();
        }
        OnCloseEditor();
    });
    m_editor->OnNowActivate.Connect(CONNECTION_DEBUG_LOCATION, [menu]{ menu->setProperty("a_accept", true); menu->close(); });
    m_editor->OnApplyActivate.Connect(CONNECTION_DEBUG_LOCATION, [menu]{ menu->setProperty("a_accept", true); menu->close(); });
    
    m_connectors.AddConnector<LocalPropertiesDateTimeConnector>(&m_editor->CurrentDateTime, ui->dateTimeEdit, &TimeShift);
    ui->dateTimeEdit->DisplayFormat.ConnectFrom(CONNECTION_DEBUG_LOCATION, DisplayFormat);

    WidgetPushButtonWrapper(ui->CalendarButton).SetControl(ButtonRole::DateTimePicker).SetIcon("Calendar");

}

WidgetsDatetimePopupPicker::~WidgetsDatetimePopupPicker()
{
    delete ui;
}

WidgetsDateTimeEdit* WidgetsDatetimePopupPicker::GetLineEdit() const
{
    return ui->dateTimeEdit;
}
