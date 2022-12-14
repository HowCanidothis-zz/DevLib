#ifndef LOCALPROPERTYERRORSCONTAINER_H
#define LOCALPROPERTYERRORSCONTAINER_H

#include "localproperty.h"
#include "localpropertydeclarations.h"

struct LocalPropertyErrorsContainerValue
{
    Name Id;
    TranslatedStringPtr Error;
    QtMsgType Type = QtMsgType::QtCriticalMsg;
    SharedPointer<LocalPropertyBool> Visible;
    DispatcherConnectionsSafe Connection;

    operator qint32() const { return Id; }
};

class LocalPropertyErrorsContainer : public LocalPropertySet<LocalPropertyErrorsContainerValue>
{
    using Super = LocalPropertySet<LocalPropertyErrorsContainerValue>;
public:
    LocalPropertyErrorsContainer();

    void AddError(const Name& errorName, const QString& errorString, QtMsgType severity = QtMsgType::QtCriticalMsg, const SharedPointer<LocalPropertyBool>& visible = nullptr);
    void AddError(const Name& errorName, const TranslatedStringPtr& errorString, QtMsgType severity = QtMsgType::QtCriticalMsg, const SharedPointer<LocalPropertyBool>& visible = nullptr);
    void RemoveError(const Name& errorName);
    bool HasError(const Name& errorName) const;

    void Clear();

    QHash<Name, QVariant> ErrorsMetaData;

    DispatcherConnections RegisterError(const Name& errorId, const TranslatedStringPtr& errorString, const LocalPropertyBool& property, bool inverted = false, QtMsgType severity = QtMsgType::QtCriticalMsg, const SharedPointer<LocalPropertyBool>& visible = nullptr);
    DispatcherConnections RegisterError(const Name& errorId, const TranslatedStringPtr& errorString, const std::function<bool ()>& validator, const QVector<Dispatcher*>& dispatchers, QtMsgType severity = QtMsgType::QtCriticalMsg, const SharedPointer<LocalPropertyBool>& visible = nullptr);
    template<typename Function, typename ... Args>
    DispatcherConnections RegisterCritical(const Name& errorId, const TranslatedStringPtr& errorString, const Function& validator, Args ... args) {
        return RegisterError(QtCriticalMsg, errorId, errorString, validator, args...);
    }
    template<typename Function, typename ... Args>
    DispatcherConnections RegisterWarning(const Name& errorId, const TranslatedStringPtr& errorString, const Function& validator, Args ... args) {
        return RegisterError(QtWarningMsg, errorId, errorString, validator, args...);
    }
    template<typename Function, typename ... Args>
    DispatcherConnections RegisterError(QtMsgType severity, const Name& errorId, const TranslatedStringPtr& errorString, const Function& validator, Args ... args) {
        auto handler = [=]{ return validator(args->Native()...); };

        QVector<Dispatcher*> dispatchers;
        adapters::Combine([&](const auto property){
            dispatchers.append(&property->OnChanged);
        }, args...);
        return RegisterError(errorId, errorString, handler, dispatchers, severity);
    }
    DispatcherConnections Connect(const QString& prefix, const LocalPropertyErrorsContainer& errors);
    DispatcherConnections ConnectFromError(const Name& errorId, const LocalPropertyErrorsContainer& errors);

    QString ToString() const;
    QStringList ToStringList() const;

    LocalPropertyBool HasErrors;
    LocalPropertyBool HasErrorsOrWarnings;
    DispatchersCommutator OnErrorsLabelsChanged;
    CommonDispatcher<const LocalPropertyErrorsContainerValue&> OnErrorAdded;
    CommonDispatcher<const LocalPropertyErrorsContainerValue&> OnErrorRemoved;

private:
#ifdef QT_DEBUG
    QSet<Name> m_registeredErrors;
#endif
    DispatcherConnectionsSafe m_connections;
};

#endif // LOCALPROPERTYERRORSCONTAINER_H
