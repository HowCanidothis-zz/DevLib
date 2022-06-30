#include "widgetbuilder.h"

#include <QSplitter>
#include <QBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

#include "WidgetsModule/Widgets/widgetsspinboxwithcustomdisplay.h"

WidgetBuilder::WidgetBuilder(QWidget* parent, Qt::Orientation layoutOrientation, qint32 margins)
    : m_addWidgetDelegate(defaultAddDelegate())
{
    QBoxLayout* layout;
    if(layoutOrientation == Qt::Vertical) {
        layout = new QVBoxLayout(parent);
    } else {
        layout = new QHBoxLayout(parent);
    }
    layout->setMargin(margins);
    m_addWidgetFunctors.append([layout](QWidget* widget){ layout->addWidget(widget); });
}

WidgetBuilder::~WidgetBuilder()
{
}

WidgetBuilder& WidgetBuilder::StartSplitter(const std::function<void (WidgetBuilder&, QSplitter*)>& handler)
{
    auto* splitter = new QSplitter();
    m_addWidgetFunctors.last()(splitter);
    m_addWidgetFunctors.append([splitter](QWidget* w){ splitter->insertWidget(splitter->count(), w); });
    handler(*this, splitter);
    m_addWidgetFunctors.pop();
    return *this;
}

WidgetBuilder& WidgetBuilder::StartLayout(const WidgetBuilderLayoutParams& params, const std::function<void (WidgetBuilder&)>& handler)
{
    auto* dummyWidget = new QWidget();
    QBoxLayout* toAdd;
    if(params.Orientation == Qt::Vertical) {
        toAdd = new QVBoxLayout(dummyWidget);
    } else {
        toAdd = new QHBoxLayout(dummyWidget);
    }
    qDebug() << dummyWidget->sizePolicy();
    m_addWidgetFunctors.last()(dummyWidget);
    m_addWidgetFunctors.append([toAdd](QWidget* w){ toAdd->addWidget(w); });
    handler(*this);
    if(params.AddSpacerToTheEnd) {
        if(params.Orientation == Qt::Vertical) {
            toAdd->addItem(new QSpacerItem(20,40, QSizePolicy::Minimum, QSizePolicy::Expanding));
        } else {
            toAdd->addItem(new QSpacerItem(40,20, QSizePolicy::Expanding));
        }
    }
    m_addWidgetFunctors.pop();
    return *this;
}

WidgetBuilder& WidgetBuilder::Make(const std::function<void (WidgetBuilder&)>& handler)
{
    handler(*this);
    return *this;
}

WidgetBuilder& WidgetBuilder::StartLabeledLayout(QuadTreeF::BoundingRect_Location location, const std::function<void (WidgetBuilder&)>& handler)
{
    switch(location)
    {
    case QuadTreeF::Location_MiddleLeft:
        m_addWidgetDelegate = [this](const FTranslationHandler& translation, QWidget* widget) {
            StartLayout(Qt::Horizontal, [translation, widget](WidgetBuilder& builder){
                builder.defaultAddDelegate()(translation, new QLabel(translation()));
                builder.defaultAddDelegate()(translation, widget);
            });
        }; break;
    case QuadTreeF::Location_MiddleRight:
        m_addWidgetDelegate = [this](const FTranslationHandler& translation, QWidget* widget) {
            StartLayout(Qt::Horizontal, [translation, widget](WidgetBuilder& builder){
                builder.defaultAddDelegate()(translation, widget);
                builder.defaultAddDelegate()(translation, new QLabel(translation()));
            });
        }; break;
    case QuadTreeF::Location_MiddleTop:
        m_addWidgetDelegate = [this](const FTranslationHandler& translation, QWidget* widget) {
            StartLayout(Qt::Vertical, [translation, widget](WidgetBuilder& builder){
                auto* newLabel = new QLabel(translation());
                newLabel->setAlignment(Qt::AlignCenter);
                builder.defaultAddDelegate()(translation, newLabel);
                builder.defaultAddDelegate()(translation, widget);
            });
        }; break;
    case QuadTreeF::Location_MiddleBottom:
        m_addWidgetDelegate = [this](const FTranslationHandler& translation, QWidget* widget) {
            StartLayout(Qt::Vertical, [translation, widget](WidgetBuilder& builder){
                auto* newLabel = new QLabel(translation());
                newLabel->setAlignment(Qt::AlignCenter);
                builder.defaultAddDelegate()(translation, widget);
                builder.defaultAddDelegate()(translation, newLabel);
            });
        }; break;
    default: Q_ASSERT(false); return *this;
    };

    handler(*this);
    m_addWidgetDelegate = defaultAddDelegate();
    return *this;
}

WidgetsDoubleSpinBoxWithCustomDisplay* WidgetBuilder::AddDoubleSpinBox(const FTranslationHandler& label)
{
    return (WidgetsDoubleSpinBoxWithCustomDisplay*)AddWidget<WidgetsDoubleSpinBoxWithCustomDisplay>(label).GetWidget();
}

WidgetsSpinBoxWithCustomDisplay* WidgetBuilder::AddSpinBox(const FTranslationHandler& label)
{
    return (WidgetsSpinBoxWithCustomDisplay*)AddWidget<WidgetsSpinBoxWithCustomDisplay>(label).GetWidget();
}

WidgetPushButtonWrapper WidgetBuilder::AddButton(const FTranslationHandler& label)
{
    return AddWidget<QPushButton>(label).Cast<WidgetPushButtonWrapper>();
}

WidgetComboboxWrapper WidgetBuilder::AddCombobox(const FTranslationHandler& label)
{
    return AddWidget<QComboBox>(label).Cast<WidgetComboboxWrapper>();
}

WidgetBuilder::FAddDelegate WidgetBuilder::defaultAddDelegate()
{
    return [this](const FTranslationHandler&, QWidget* w){ m_addWidgetFunctors.last()(w); };
}