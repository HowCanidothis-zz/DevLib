#include "localpropertieswidgetconnector.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

LocalPropertiesWidgetConnectorBase::LocalPropertiesWidgetConnectorBase(const Setter& widgetSetter, const Setter& propertySetter)
    : m_widgetSetter([this, widgetSetter](){
        if(!m_ignorePropertyChange) {
            guards::LambdaGuard guard([this]{ m_ignoreWidgetChange = false; }, [this] { m_ignoreWidgetChange = true; } );
            widgetSetter();
        }
    })
    , m_propertySetter([this, propertySetter]{
        if(!m_ignoreWidgetChange) {
            guards::LambdaGuard guard([this]{ m_ignorePropertyChange = false; }, [this] { m_ignorePropertyChange = true; } );
            propertySetter();
        }
    })
    , m_dispatcherConnections(this)
    , m_ignorePropertyChange(false)
    , m_ignoreWidgetChange(false)
{
    m_widgetSetter();
}

LocalPropertiesWidgetConnectorBase::~LocalPropertiesWidgetConnectorBase()
{
    disconnect(m_connection);
}

LocalPropertiesCheckBoxConnector::LocalPropertiesCheckBoxConnector(LocalProperty<bool>* property, QCheckBox* checkBox)
    : Super([checkBox, property]{
                checkBox->setChecked(*property);
            },
            [property, checkBox]{
                *property = checkBox->isChecked();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(checkBox, &QCheckBox::clicked, [this](bool value){
        m_propertySetter();
    });
}


LocalPropertiesLineEditConnector::LocalPropertiesLineEditConnector(LocalProperty<QString>* property, QLineEdit* lineEdit)
    : Super([lineEdit, property](){
               lineEdit->setText(*property);
            },
            [lineEdit, property](){
               *property = lineEdit->text();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(lineEdit, &QLineEdit::editingFinished, [this](){
        m_propertySetter();
    });
}

LocalPropertiesTextEditConnector::LocalPropertiesTextEditConnector(LocalProperty<QString>* property, QTextEdit* textEdit, LocalPropertiesTextEditConnector::SubmitType submitType)
    : Super([textEdit, property](){
               textEdit->setText(*property);
            }, [textEdit, property]{
               *property = textEdit->toPlainText();
            })
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    switch (submitType) {
    case SubmitType_OnEveryChange:
        m_connection = connect(textEdit, &QTextEdit::textChanged, [this](){
            m_propertySetter();
        });
        break;
    default:
        break;
    }
}

LocalPropertiesDoubleSpinBoxConnector::LocalPropertiesDoubleSpinBoxConnector(LocalPropertyDouble* property, QDoubleSpinBox* spinBox)
    : Super([spinBox, property](){
                spinBox->setRange(property->GetMin(), property->GetMax());
                spinBox->setValue(*property);
            },
            [spinBox, property](){
                *property = spinBox->value();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, spinBox](){
        m_propertySetter();
    });
}

LocalPropertiesDoubleSpinBoxConnector::LocalPropertiesDoubleSpinBoxConnector(LocalPropertyFloat* property, QDoubleSpinBox* spinBox)
    : Super([spinBox, property](){
                spinBox->setRange(property->GetMin(), property->GetMax());
                spinBox->setValue(*property);
            },
            [spinBox, property](){
                *property = spinBox->value();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](){
        m_propertySetter();
    });
}

LocalPropertiesSpinBoxConnector::LocalPropertiesSpinBoxConnector(LocalPropertyInt* property, QSpinBox* spinBox)
    : Super([spinBox, property](){
                spinBox->setRange(property->GetMin(), property->GetMax());
                spinBox->setValue(*property);
            },
            [spinBox, property](){
                *property = spinBox->value();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](){
        m_propertySetter();
    });
}

void LocalPropertiesWidgetConnectorsContainer::Clear()
{
    m_connectors.Clear();
}

LocalPropertiesComboBoxConnector::LocalPropertiesComboBoxConnector(LocalPropertyNamedUint* property, QComboBox* comboBox)
    : Super([property, comboBox]{
                comboBox->setCurrentIndex(*property);
            },
            [property, comboBox]{
                *property = comboBox->currentIndex();
            }
    )
{
    m_dispatcherConnections.Add(property->GetDispatcher(),[this]{
        m_widgetSetter();
    });

    m_connection = connect(comboBox, static_cast<void (QComboBox::*)(qint32)>(&QComboBox::currentIndexChanged), [this]{
        m_propertySetter();
    });
}
