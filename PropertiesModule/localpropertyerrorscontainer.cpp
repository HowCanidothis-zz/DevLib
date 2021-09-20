#include "localpropertyerrorscontainer.h"

#include "translatormanager.h"

LocalPropertyErrorsContainer::LocalPropertyErrorsContainer()
    : HasErrors(false)
    , OnErrorsLabelsChanged(500)
{
    OnChange += {this, [this]{
        HasErrors = !IsEmpty();
    }};

    TranslatorManager::GetInstance().OnLanguageChanged.Connect(this, [this]{
        OnErrorsLabelsChanged();
    }).MakeSafe(m_connections);
}

void LocalPropertyErrorsContainer::AddError(const Name& errorName, const QString& errorString)
{
    AddError(errorName, ::make_shared<TranslatedString>([errorString]{ return errorString; }));
}

void LocalPropertyErrorsContainer::AddError(const Name& errorName, const TranslatedStringPtr& errorString)
{
    LocalPropertyErrorsContainerValue toInsert{ errorName, errorString };
    OnErrorsLabelsChanged.ConnectFrom(errorString->OnChange).MakeSafe(m_connections);
    if(Super::Insert(toInsert)) {
        OnErrorAdded(toInsert);
    }
}

void LocalPropertyErrorsContainer::RemoveError(const Name& errorName)
{
    LocalPropertyErrorsContainerValue toRemove{ errorName, ::make_shared<TranslatedString>([]{ return QString(); }) };
    if(Super::Remove(toRemove)) {
        OnErrorRemoved(toRemove);
    }
}

DispatcherConnection LocalPropertyErrorsContainer::RegisterError(const Name& errorId, const TranslatedStringPtr& errorString, const LocalProperty<bool>& property, bool inverted)
{
#ifdef QT_DEBUG
    Q_ASSERT(!m_registeredErrors.contains(errorId));
    m_registeredErrors.insert(errorId);
#endif
    auto* pProperty = const_cast<LocalProperty<bool>*>(&property);
    auto update = [this, errorId, pProperty, errorString, inverted]{
        if(*pProperty ^ inverted) {
            AddError(errorId, errorString);
        } else {
            RemoveError(errorId);
        }
    };
    update();
    return pProperty->OnChange.Connect(this, update);
}

DispatcherConnections LocalPropertyErrorsContainer::RegisterError(const Name& errorId, const TranslatedStringPtr& errorString, const std::function<bool ()>& validator, const QVector<Dispatcher*>& dispatchers)
{
#ifdef QT_DEBUG
    Q_ASSERT(!m_registeredErrors.contains(errorId));
    m_registeredErrors.insert(errorId);
#endif
    DispatcherConnections result;
    auto update = [this, validator, errorId, errorString]{
        if(!validator()) {
            AddError(errorId, errorString);
        } else {
            RemoveError(errorId);
        }
    };

    for(auto* dispatcher : dispatchers) {
        result += dispatcher->Connect(this, update);
    }

    update();
    return result;
}

DispatcherConnections LocalPropertyErrorsContainer::Connect(const QString& prefix, const LocalPropertyErrorsContainer& errors)
{
    auto* pErrors = const_cast<LocalPropertyErrorsContainer*>(&errors);
    auto addError = [this, prefix](const LocalPropertyErrorsContainerValue& value){
        AddError(Name(prefix + value.Id.AsString()), value.Error);
    };
    auto removeError = [this, prefix](const LocalPropertyErrorsContainerValue& value) {
        RemoveError(Name(prefix + value.Id.AsString()));
    };
    DispatcherConnections result;
    result += pErrors->OnErrorAdded.Connect(this, addError);
    result += pErrors->OnErrorRemoved.Connect(this, removeError);
    for(const auto& error : errors) {
        AddError(Name(prefix + error.Id.AsString()), error.Error);
    }
    return result;
}

QString LocalPropertyErrorsContainer::ToString() const
{
    QString resultText;
    for(const auto& error : *this) {
        resultText += error.Error->Native() + "\n";
    }
    return resultText;
}

QStringList LocalPropertyErrorsContainer::ToStringList() const
{
    QStringList result;
    for(const auto& error : *this) {
        result += error.Error->Native();
    }
    return result;
}
