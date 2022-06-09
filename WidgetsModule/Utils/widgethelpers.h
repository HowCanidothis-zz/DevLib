#ifndef WIDGETHELPERS_H
#define WIDGETHELPERS_H

#include <PropertiesModule/internal.hpp>

#include "WidgetsModule/widgetsdeclarations.h"

struct WidgetWrapperInjectedCommutatorData
{
    DispatchersCommutator Commutator;
    DispatcherConnectionsSafe Connections;

    WidgetWrapperInjectedCommutatorData()
        : Commutator(1000)
    {}
};

Q_DECLARE_METATYPE(SharedPointer<WidgetWrapperInjectedCommutatorData>)

#define DECLARE_WIDGET_WRAPPER_FUNCTIONS(WrapperType, type) \
    WrapperType& Make(const std::function<void (WrapperType&)>& handler) { return make<WrapperType>(handler); } \
    type* GetWidget() const { return reinterpret_cast<type*>(m_widget); }

class ObjectWrapper
{
public:
    ObjectWrapper(QObject* object)
        : m_object(object)
    {}

    template<class T>
    SharedPointer<T> Injected(const char* propertyName, const std::function<T* ()>& creator = nullptr) const
    {
        auto value = m_object->property(propertyName).value<SharedPointer<T>>();
        if(value == nullptr) {
            value = creator != nullptr ? creator() : new T();
            m_object->setProperty(propertyName, QVariant::fromValue(value));
        }
        return value;
    }

    SharedPointer<WidgetWrapperInjectedCommutatorData> InjectedCommutator(const char* propertyName, const std::function<void (QObject* w)>& handler = nullptr) const
    {
        return Injected<WidgetWrapperInjectedCommutatorData>(propertyName, [&]() -> WidgetWrapperInjectedCommutatorData* {
            auto* result = new WidgetWrapperInjectedCommutatorData();
            auto* widget = m_object;
            result->Commutator.Connect(nullptr, [handler, widget]{
                 handler(widget);
            });
            return result;
        });
    }

    template<class Property, typename ... Args>
    SharedPointer<Property> GetOrCreateProperty(const char* propName, const std::function<void (QObject*, const Property&)>& handler, Args... args) const
    {
        return Injected<Property>(propName, [&]() -> Property* {
            auto* property = new Property(args...);
            auto* widget = m_object;
            property->OnChanged.ConnectAndCall(this, [widget, handler, property]{ handler(widget, *property); });
            property->SetSetterHandler(ThreadHandlerMain);
            return property;
        });
    }

private:
    QObject* m_object;
};

class WidgetWrapper : public ObjectWrapper
{
    using Super = ObjectWrapper;
    using FConnector = DispatcherConnection (WidgetWrapper::*)(const char*, QWidget*);
public:
    WidgetWrapper(QWidget* widget);

    DispatcherConnection ConnectVisibility(const char* debugLocation, QWidget* another);
    DispatcherConnection ConnectEnablity(const char* debugLocation, QWidget* another);
    DispatcherConnections CreateVisibilityRule(const char* debugLocation, const std::function<bool ()>& handler, const QVector<Dispatcher*>& dispatchers, const QVector<QWidget*>& additionalWidgets)
    {
        return createRule(debugLocation, &WidgetVisibility(), handler, dispatchers, additionalWidgets, &WidgetWrapper::ConnectVisibility);
    }
    DispatcherConnections CreateEnablityRule(const char* debugLocation, const std::function<bool ()>& handler, const QVector<Dispatcher*>& dispatchers, const QVector<QWidget*>& additionalWidgets)
    {
        return createRule(debugLocation, &WidgetEnablity(), handler, dispatchers, additionalWidgets, &WidgetWrapper::ConnectEnablity);
    }

    void SetVisibleAnimated(bool visible, int duration = 2000, double opacity = 0.8);
    void ShowAnimated(int duration = 2000, double opacity = 0.8);
    void HideAnimated(int duration = 2000);

    WidgetWrapper& AddModalProgressBar();
    WidgetWrapper& AddToFocusManager(const QVector<QWidget*>& additionalWidgets);
    WidgetWrapper& AddEventFilter(const std::function<bool (QObject*, QEvent*)>& filter);
    WidgetWrapper& CreateCustomContextMenu(const std::function<void (QMenu*)>& creatorHandler, bool preventFromClosing = false);

    WidgetWrapper& BlockWheel();
    WidgetWrapper& FixUp();
    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetWrapper, QWidget)
    WidgetWrapper& SetPalette(const QHash<qint32, LocalPropertyColor*>& palette);

    DispatcherConnectionsSafe& WidgetConnections();
    LocalPropertyBool& WidgetVisibility(bool animated = false);
    LocalPropertyBool& WidgetEnablity();
    LocalPropertyBool& WidgetCollapsing(bool horizontal, qint32 initialWidth);

    bool HasParent(QWidget* parent);
    void ForeachParentWidget(const std::function<bool(QWidget*)>& handler);
    void ForeachChildWidget(const std::function<void (QWidget*)>& handler);

    QWidget* operator->() const { return m_widget; }
    operator QWidget*() const { return m_widget; }

private:
    DispatcherConnections createRule(const char* debugLocation, LocalPropertyBool* property, const std::function<bool ()>& handler, const QVector<Dispatcher*>& dispatchers, const QVector<QWidget*>& additionalWidgets,
                                     const FConnector& connector);

protected:
    template<class T>
    T& make(const std::function<void (T&)>& handler)
    {
        auto* tThis = reinterpret_cast<T*>(this);
        handler(*tThis);
        return *tThis;
    }

protected:
    QWidget* m_widget;
};

class WidgetLineEditWrapper : public WidgetWrapper
{
public:
    WidgetLineEditWrapper(class QLineEdit* lineEdit);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetLineEditWrapper, QLineEdit)
    WidgetLineEditWrapper& SetDynamicSizeAdjusting();

private:
};

class WidgetComboboxWrapper : public WidgetWrapper
{
public:
    WidgetComboboxWrapper(class QComboBox* combobox);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetComboboxWrapper, QComboBox)
    WidgetComboboxWrapper& EnableStandardItems(const QSet<qint32>& indices);
    WidgetComboboxWrapper& DisableStandardItems(const QSet<qint32>& indices);
    WidgetComboboxWrapper& DisconnectModel();
    class QCompleter* CreateCompleter(QAbstractItemModel* model, const std::function<void (const QModelIndex& index)>& onActivated, qint32 column = 0);
};

class WidgetGroupboxWrapper : public WidgetWrapper
{
public:
    WidgetGroupboxWrapper(class QGroupBox* groupBox);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetGroupboxWrapper, QGroupBox)
    WidgetGroupboxWrapper& AddCollapsing();
    WidgetGroupboxWrapper& AddCollapsingDispatcher(Dispatcher* updater);
};

class LocalPropertyDoubleDisplay : public LocalPropertyDouble
{
    using Super = LocalPropertyDouble;
public:
    LocalPropertyDoubleDisplay(double value = 0, double min = (std::numeric_limits<double>::lowest)(), double max = (std::numeric_limits<double>::max)())
        : Super(value, min, max)
        , Precision(2)
    {
        auto update = [this]{
            DisplayValue.SetMinMax(*this, *this);
        };
        this->OnChanged.Connect(this, update);
        update();
    }

    LocalPropertyDoubleDisplay& operator-=(double value) { SetValue(Super::Native() - value); return *this; }
    LocalPropertyDoubleDisplay& operator+=(double value) { SetValue(Super::Native() + value); return *this; }
    LocalPropertyDoubleDisplay& operator=(double value) { SetValue(value); return *this; }

    LocalPropertyDouble DisplayValue;
    LocalPropertyInt Precision;
};

class WidgetLabelWrapper : public WidgetWrapper
{
public:
    WidgetLabelWrapper(QLabel* label);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetLabelWrapper, QLabel)

    TranslatedStringPtr WidgetText();
};

class WidgetTableViewWrapper : public WidgetWrapper
{
public:
    WidgetTableViewWrapper(QTableView* tableView);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(WidgetTableViewWrapper, QTableView)
    bool CopySelectedTableContentsToClipboard(bool includeHeaders = false);
    QList<int> SelectedRowsSorted();
    QList<int> SelectedColumnsSorted();
    QSet<int> SelectedRowsSet();
    QSet<int> SelectedColumnsSet();
    void SelectRowsAndScrollToFirst(const QSet<qint32>& rows);
    void SelectColumnsAndScrollToFirst(const QSet<qint32>& columns);
    class QHeaderView* InitializeHorizontal(const DescTableViewParams& params = DescTableViewParams());
    QHeaderView* InitializeVertical(const DescTableViewParams& params = DescTableViewParams());
    class WidgetsMatchingAttachment* CreateMatching(QAbstractItemModel* targetModel, const QSet<qint32>& targetImportColumns);
};

class ActionWrapper : public ObjectWrapper
{
    using Super = ObjectWrapper;
public:
    ActionWrapper(QAction* action);

    ActionWrapper& Make(const std::function<void (ActionWrapper&)>& handler);
    ActionWrapper& SetShortcut(const QKeySequence& keySequence);
    ActionWrapper& SetText(const QString& text);

    QAction* GetAction() const { return m_action; }

    LocalPropertyBool& ActionVisibility();
    LocalPropertyBool& ActionEnablity();
    TranslatedStringPtr ActionText();

    QAction* operator->() const { return m_action; }
    operator QAction*() const { return m_action; }

private:
    QAction* m_action;
};

class DialogWrapper : public WidgetWrapper
{
    using Super = WidgetWrapper;
public:
    DialogWrapper(const Name& id, const std::function<DescCustomDialogParams ()>& paramsCreator);

    template<class T>
    T* GetCustomView() const { return WidgetsDialogsManager::GetInstance().CustomDialogView<T>(GetWidget()); }
    void Show(const DescShowDialogParams& params);

    DECLARE_WIDGET_WRAPPER_FUNCTIONS(DialogWrapper, QDialog)
};

class MenuWrapper
{
public:
    MenuWrapper(QWidget* widget, const WidgetsGlobalTableActionsScopeHandlersPtr& handlers = nullptr)
        : m_widget(widget)
        , m_globalActionsHandlers(handlers)
    {}

    const MenuWrapper& Make(const std::function<void (const MenuWrapper&)>& handler) const { handler(*this); return *this; }
    MenuWrapper& AddGlobalAction(const QString& path);
    MenuWrapper& AddGlobalTableAction(const Latin1Name& id);
    ActionWrapper AddAction(const QString& title, const std::function<void ()>& handle) const;
    ActionWrapper AddAction(const QString &title, const std::function<void (QAction*)> &handle) const;

    template<class Property>
    ActionWrapper AddCheckboxAction(const QString& title, Property* value) const
    {
        return AddCheckboxAction(title, *value, [value](bool val){
            *value = val;
        });
    }
    template<class Property>
    ActionWrapper AddColorAction(const QString& title, Property* color) const
    {
        return AddColorAction(title, *color, [color](const QColor& val){
            *color = val;
        });
    }
    template<class Property>
    ActionWrapper AddDoubleAction(const QString& title, Property* value) const
    {
        return AddDoubleAction(title, *value, [value](double val){
            *value = val;
        });
    }

    ActionWrapper AddCheckboxAction(const QString& title, bool checked, const std::function<void (bool)>& handler) const;
    ActionWrapper AddColorAction(const QString& title, const QColor& color, const std::function<void (const QColor& color)>& handler) const;
    ActionWrapper AddDoubleAction(const QString& title, double value, const std::function<void (double value)>& handler) const;
    ActionWrapper AddTableColumnsAction();
    ActionWrapper AddSeparator() const;
    class QMenu* AddPreventedFromClosingMenu(const QString& title) const;
    static QMenu* CreatePreventedFromClosingMenu(const QString& title);
    QMenu* AddMenu(const QString& label) const;

    QMenu* GetMenu() const { return reinterpret_cast<QMenu*>(m_widget); }

private:
    QWidget* m_widget;
    WidgetsGlobalTableActionsScopeHandlersPtr m_globalActionsHandlers;
};

_Export void forEachModelIndex(const QAbstractItemModel* model, QModelIndex parent, const std::function<bool (const QModelIndex& index)>& function);

class WidgetsObserver : public QObject
{
    WidgetsObserver();
public:
    static WidgetsObserver& GetInstance();

    void EnableAutoCollapsibleGroupboxes();

    CommonDispatcher<QWidget*> OnAdded;

private:
    bool eventFilter(QObject* watched, QEvent* e) override;
};

#endif // WIDGETHELPERS_H
