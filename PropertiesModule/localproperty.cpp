#include "localproperty.h"

DispatcherConnections LocalPropertiesConnectBoth(const char* debugLocation, const QVector<Dispatcher*>& dispatchers1, const FAction& evaluator1, const QVector<Dispatcher*>& dispatchers2, const FAction& evaluator2){
    DispatcherConnections result;
    auto sync = ::make_shared<std::atomic_bool>(false);
    auto eval1 = [debugLocation, evaluator1, sync]{
        if(!*sync) {
            *sync = true;
            evaluator1();
            *sync = false;
        }
    };
    auto eval2 = [debugLocation, evaluator2, sync]{
        if(!*sync) {
            *sync = true;
            evaluator2();
            *sync = false;
        }
    };
    for(auto* dispatcher : dispatchers1) {
        result += dispatcher->Connect(CONNECTION_DEBUG_LOCATION, eval1);
    }
    for(auto* dispatcher : dispatchers2) {
        result += dispatcher->Connect(CONNECTION_DEBUG_LOCATION, eval2);
    }
    eval1();
    return result;
}

LocalPropertyBoolCommutator::LocalPropertyBoolCommutator(bool defaultState, qint32 msecs, const ThreadHandlerNoThreadCheck& threadHandler)
    : Super(defaultState)
    , m_commutator(msecs, threadHandler)
    , m_defaultState(defaultState)
{
    m_commutator += { this, [this]{
        Update();
    }};
}

void LocalPropertyBoolCommutator::ClearProperties()
{
    m_properties.clear();
}

void LocalPropertyBoolCommutator::Update()
{
    bool result = m_defaultState;
    bool oppositeState = !result;
    for(auto* property : ::make_const(m_properties)) {
        if(*property == oppositeState) {
            result = oppositeState;
            break;
        }
    }
    SetValue(result);
}

DispatcherConnections LocalPropertyBoolCommutator::AddProperties(const char* connectionInfo, const QVector<LocalProperty<bool>*>& properties)
{
    DispatcherConnections result;
    for(auto* property : properties) {
        result += m_commutator.ConnectFrom(connectionInfo, property->OnChanged);
    }
    m_properties += properties;
    return result;
}

LocalPropertyDate::LocalPropertyDate(const QDate& value, const QDate& min, const QDate& max)
    : Super(applyRange(value, min, max))
    , m_min(min)
    , m_max(max)
{}

void LocalPropertyDate::SetMinMax(const QDate& min, const QDate& max)
{
    if(LocalPropertyNotEqual(m_max, max) || LocalPropertyNotEqual(m_min, min)) {
        m_min = min;
        m_max = max;
        SetValue(Super::m_value);
        OnMinMaxChanged();
    }
}

QDate LocalPropertyDate::applyRange(const QDate& cur, const QDate& min, const QDate& max)
{
    if(!cur.isValid()) {
        return cur;
    }
    return QDate::fromJulianDay(::clamp(cur.toJulianDay(), validatedMin(min).toJulianDay(), validatedMax(max).toJulianDay()));
}

QDate LocalPropertyDate::applyMinMax(const QDate& value) const
{
    return applyRange(value, m_min, m_max);
}

void LocalPropertyDate::validate(QDate& value) const
{
    value = applyMinMax(value);
}

LocalPropertyTime::LocalPropertyTime(const QTime& value, const QTime& min, const QTime& max)
    : Super(applyRange(value, min, max))
    , m_min(min)
    , m_max(max)
{
}

void LocalPropertyTime::SetMinMax(const QTime& min, const QTime& max)
{
    if(LocalPropertyNotEqual(m_max, max) || LocalPropertyNotEqual(m_min, min)) {
        m_min = min;
        m_max = max;
        SetValue(Super::m_value);
        OnMinMaxChanged();
    }
}

QTime LocalPropertyTime::applyRange(const QTime& cur, const QTime& min, const QTime& max)
{
    if(!cur.isValid()) {
        return cur;
    }
    return QTime::fromMSecsSinceStartOfDay(::clamp(cur.msecsSinceStartOfDay(), validatedMin(min).msecsSinceStartOfDay(), validatedMax(max).msecsSinceStartOfDay()));
}

QTime LocalPropertyTime::applyMinMax(const QTime& value) const
{
    return applyRange(value, m_min, m_max);
}

void LocalPropertyTime::validate(QTime& value) const
{
    value = applyMinMax(value);
}

LocalPropertyDateTime::LocalPropertyDateTime(const QDateTime& value, const QDateTime& min, const QDateTime& max)
    : Super(applyRange(value, min, max))
    , m_min(min)
    , m_max(max)
{
}

void LocalPropertyDateTime::SetMinMax(const QDateTime& min, const QDateTime& max)
{
    if(LocalPropertyNotEqual(m_max, max) || LocalPropertyNotEqual(m_min, min)) {
        m_min = min;
        m_max = max;
        SetValue(Super::m_value);
        OnMinMaxChanged();
    }
}

QDateTime LocalPropertyDateTime::applyRange(const QDateTime& cur, const QDateTime& min, const QDateTime& max)
{
    if(!cur.isValid()) {
        return cur;
    }
    return QDateTime::fromMSecsSinceEpoch(::clamp(cur.toMSecsSinceEpoch(), validatedMin(min).toMSecsSinceEpoch(), validatedMax(max).toMSecsSinceEpoch()));
}

void LocalPropertyDateTime::validate(QDateTime& value) const
{
    value = applyRange(value, m_min, m_max);
}