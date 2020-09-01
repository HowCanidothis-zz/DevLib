#ifndef LOCALPROPERTY_H
#define LOCALPROPERTY_H

#include <limits>

#include "property.h"
#include "externalproperty.h"

template<class T, class StorageType = T>
class LocalProperty
{
    typedef std::function<void ()> FSetter;
    typedef std::function<void (const FSetter&)> FSetterHandler;
protected:
    StorageType m_value;
    FSetterHandler m_setterHandler;

public:
    LocalProperty()
        : m_setterHandler([](const FSetter& setter){
            setter();
        })
    {}
    LocalProperty(const T& value)
        : m_value(value)
        , m_setterHandler([](const FSetter& setter){
            setter();
        })
    {}

    void Invoke()
    {
        m_setterHandler([this]{
            OnChange.Invoke();
            if(m_subscribes != nullptr) {
                m_subscribes();
            }
        });
    }

    void Subscribe(const FAction& subscribe) const
    {
        if(m_subscribes == nullptr) {
            m_subscribes = subscribe;
        } else {
            auto oldHandle = m_subscribes;
            m_subscribes = [subscribe, oldHandle]{
                oldHandle();
                subscribe();
            };
        }
    }

    void SetAndSubscribe(const FAction& subscribe) const
    {
        Subscribe(subscribe);
        subscribe();
    }

    void SetSetterHandler(const FSetterHandler& handler) {
        m_setterHandler = handler;
    }

    void SetValue(const T& value)
    {
        if(value != m_value) {
            m_setterHandler([value, this]{
                m_value = value;
                Invoke();
            });
        }
    }

    StorageType& EditSilent() { return m_value; }
    const StorageType& Native() const { return m_value; }
    Dispatcher& GetDispatcher() { return OnChange; }

    bool operator!() const { return m_value == false; }
    bool operator!=(const T& value) const { return m_value != value; }
    bool operator==(const T& value) const { return m_value == value; }
    LocalProperty& operator=(const T& value) { SetValue(value); return *this; }
    operator const T&() const { return m_value; }

    Dispatcher OnChange;

private:
    template<class T2> friend struct Serializer;
    mutable FAction m_subscribes;
};

template<class T>
class LocalPropertyLimitedDecimal : public LocalProperty<T>
{
    using Super = LocalProperty<T>;
public:
    LocalPropertyLimitedDecimal(const T& value = 0, const T& min = (std::numeric_limits<T>::lowest)(), const T& max = (std::numeric_limits<T>::max)())
        : Super(::clamp(value, min, max))
        , m_min(min)
        , m_max(max)
    {}

    void SetMinMax(const T& min, const T& max)
    {
        if(!qFuzzyCompare((double)m_max,max) || !qFuzzyCompare((double)m_min, min)) {
            m_min = min;
            m_max = max;
            SetValue(validateValue(Super::m_value));
            OnMinMaxChanged();
        }
    }

    void SetValue(const T& value)
    {
        auto validatedValue = validateValue(value);
        if(!qFuzzyCompare(double(validatedValue), double(Super::m_value))) {
            Super::m_setterHandler([validatedValue, this]{
                Super::m_value = validatedValue;
                Super::Invoke();
            });
        }
    }

    LocalPropertyLimitedDecimal& operator=(const T& value) { SetValue(value); return *this; }

    const T& GetMin() const { return m_min; }
    const T& GetMax() const { return m_max; }

    Dispatcher OnMinMaxChanged;

private:
    T validateValue(const T& value)
    {
        return ::clamp(value, m_min, m_max);
    }

private:
    template<class T2> friend struct Serializer;
    T m_min;
    T m_max;
};

using LocalPropertyInt = LocalPropertyLimitedDecimal<qint32>;
using LocalPropertyUInt = LocalPropertyLimitedDecimal<quint32>;
using LocalPropertyDouble = LocalPropertyLimitedDecimal<double>;
using LocalPropertyFloat = LocalPropertyLimitedDecimal<float>;

class LocalPropertyNamedUint : public LocalPropertyUInt
{
    using Super = LocalPropertyUInt;
public:
    LocalPropertyNamedUint()
        : Super(0, 0, 0)
    {}

    void Initialize(quint32 initial, const QStringList& names)
    {
        m_value = initial;
        m_names = names;
        SetMinMax(0, m_names.size());
    }
    const QStringList& GetNames() const { return m_names; }
    template<class T> T Cast() const { return (T)Native(); }

    LocalPropertyNamedUint& operator=(quint32 value) { SetValue(value); return *this; }
    template<typename Enum>
    LocalPropertyNamedUint& operator=(Enum value) { return operator=((quint32)value); }
    template<typename Enum>
    bool operator==(Enum value) const { return Super::m_value == (quint32)value; }
    template<typename Enum>
    bool operator!=(Enum value) const { return Super::m_value != (quint32)value; }

    template<class Buffer>
    void Serialize(Buffer& buffer)
    {
        buffer << Super::m_value;
        buffer << m_names;
    }

private:
    QStringList m_names;
};


template<class T>
class LocalPropertyPtr : public LocalProperty<T*>
{
    using Super = LocalProperty<T*>;
public:
    LocalPropertyPtr(T* initial)
        : Super(initial)
    {}

    bool operator!=(const T* another) const { return Super::m_value != another; }
    bool operator==(const T* another) const { return Super::m_value == another; }
    LocalPropertyPtr& operator=(T* value) { Super::SetValue(value); return *this; }
    operator const T*() const { return Super::m_value; }
    const T* operator->() const { return Super::m_value; }
    T* operator->() { return Super::m_value; }
};

template<class T>
class LocalPropertySharedPtr : public LocalProperty<SharedPointer<T>>
{
    using Super = LocalProperty<SharedPointer<T>>;
public:
    LocalPropertySharedPtr(T* initial = nullptr)
        : Super(SharedPointer<T>(initial))
    {}

    const T* get() const { return Super::m_value.get(); }
    T* get() { return Super::m_value.get(); }

    void SetValue(T* value)
    {
        if(value != Super::m_value.get()) {
            Super::m_setterHandler([value, this]{
                Super::m_value = value;
                Super::Invoke();
            });
        }
    }

    bool operator!=(const T* another) const { return Super::m_value.get() != another; }
    bool operator==(const T* another) const { return Super::m_value.get() == another; }
    LocalPropertySharedPtr& operator=(T* value) { SetValue(value); return *this; }
    LocalPropertySharedPtr& operator=(const SharedPointer<T>& value) { Super::SetValue(value); return *this; }
    operator const LocalPropertySharedPtr&() const { return Super::m_value; }
    const T* operator->() const { return Super::m_value.get(); }
    T* operator->() { return Super::m_value.get(); }
    T& operator*() { return *Super::m_value; }
    const T& operator*() const { return *Super::m_value; }
};

template<class T>
class LocalPropertySet : public LocalProperty<QSet<T>>
{
    using ContainerType = QSet<T>;
    typedef LocalProperty<QSet<T>> Super;
public:
    LocalPropertySet()
    {}
    LocalPropertySet(const QSet<T>& value)
        : Super(value)
    {}

    bool IsEmpty() const { return this->m_value.isEmpty(); }
    qint32 Size() const { return this->m_value.size(); }
    bool IsContains(const T& value) const { return this->m_value.contains(value); }

    void Clear()
    {
        if(!this->m_value.isEmpty()) {
            this->m_value.clear();
            this->Invoke();
        }
    }

    void Insert(const T& value)
    {
        auto find = this->m_value.find(value);
        if(find != this->m_value.end()) {
            this->m_value.insert(value);
            this->Invoke();
        }
    }

    void Remove(const T& value)
    {
        auto find = this->m_value.find(value);
        if(find != this->m_value.end()) {
            this->m_value.erase(find);
            this->Invoke();
        }
    }

    typename QSet<T>::const_iterator begin() const { return this->m_value.begin(); }
    typename QSet<T>::const_iterator end() const { return this->m_value.end(); }
};

template<class T>
class LocalPropertyVector : public LocalProperty<QVector<T>>
{
    using ContainerType = QVector<T>;
    using Super = LocalProperty<ContainerType>;
public:
    LocalPropertyVector()
    {}
    LocalPropertyVector(const ContainerType& value)
        : Super(value)
    {}

    bool IsEmpty() const { return this->m_value.isEmpty(); }
    qint32 Size() const { return this->m_value.size(); }
    bool IsContains(const T& value) const { return this->m_value.contains(value); }

    void Clear()
    {
        if(!this->m_value.isEmpty()) {
            this->m_value.clear();
            this->Invoke();
        }
    }

    void Append(const T& value)
    {
        auto find = this->m_value.find(value);
        if(find != this->m_value.end()) {
            this->m_value.append(value);
            this->Invoke();
        }
    }

    typename ContainerType::const_iterator begin() const { return this->m_value.begin(); }
    typename ContainerType::const_iterator end() const { return this->m_value.end(); }
};

struct PropertyFromLocalProperty
{
    template<class T>
    static SharedPointer<Property> Create(const Name& name, T& localProperty);
    template<class T>
    inline static SharedPointer<Property> Create(const QString& name, T& localProperty) { return Create(Name(name), localProperty); }
};

class PropertyFromLocalPropertyContainer : public QVector<SharedPointer<Property>>
{
    using Super = QVector<SharedPointer<Property>>;
public:
    using Super::Super;
};

template<>
inline SharedPointer<Property> PropertyFromLocalProperty::Create(const Name& name, LocalPropertyLimitedDecimal<double>& localProperty)
{
    return ::make_shared<ExternalDoubleProperty>(
                name,
                [&localProperty] { return localProperty.Native(); },
                [&localProperty](double value, double) { localProperty = value; },
                localProperty.GetMin(),
                localProperty.GetMax()
    );
}

template<>
inline SharedPointer<Property> PropertyFromLocalProperty::Create(const Name& name, LocalProperty<QColor>& localProperty)
{
    return ::make_shared<ExternalColorProperty>(
                name,
                [&localProperty] { return localProperty.Native(); },
                [&localProperty](const QColor& value, const QColor&) { localProperty = value; }
    );
}

template<>
inline SharedPointer<Property> PropertyFromLocalProperty::Create(const Name& name, LocalPropertyNamedUint& localProperty)
{
    auto result = ::make_shared<ExternalNamedUIntProperty>(
                name,
                [&localProperty] { return localProperty.Native(); },
                [&localProperty](quint32 value, quint32) { localProperty = value; },
                localProperty.GetMin(),
                localProperty.GetMax()
    );
    result->SetNames(localProperty.GetNames());
    return std::move(result);
}

#endif // LOCALPROPERTY_H
