#ifndef PROPERTY_H
#define PROPERTY_H

#include <QUrl>

#include <functional>

#include <SharedModule/internal.hpp>
#include <SharedGuiModule/decl.h> // Vector3f

template<typename T>
struct PropertyValueExtractorPrivate
{
    static QVariant ExtractVariant(const T& value) { return value; }
};

template<typename T>
struct PropertyValueExtractorPrivate<T*>
{
    static QVariant ExtractVariant(T* ptr) { return QVariant((qint64)(ptr)); }
};

class _Export Property {
public:
    typedef std::function<void ()> FSetter;
    typedef std::function<void (const FSetter&)> FHandle;
    typedef std::function<void ()> FOnChange;
    typedef std::function<void (const QVariant& property, QVariant& new_value)> FValidator;

public:
    enum Option {
        Option_IsExportable = 0x1,// if property should be saved or loaded from file
        Option_IsPresentable = 0x2,// if property should be presented in properties model
        Option_IsReadOnly = 0x4, // if property cans be edited from gui

        Options_Default = Option_IsExportable | Option_IsPresentable,
        Options_ReadOnlyPresentable = Option_IsPresentable | Option_IsReadOnly,
        Options_InternalProperty = 0
    };
    DECL_FLAGS(Options, Option)

    enum DelegateValue {
        DelegateDefault,
        DelegateFileName,
        DelegatePositionXYZ,
        DelegateNamedUInt,
        DelegateRect,
        DelegateColor,

        DelegateUser
    };

    Property(const Name& path);
    virtual ~Property() {}
    bool SetValue(QVariant value);
    const Options& GetOptions() const { return m_options; }
    Options& ChangeOptions() { return m_options; }

    FHandle& Handler() { return m_fHandle; }
    FValidator& Validator() { return m_fValidator; }

    void Subscribe(const FOnChange& onChange);
    void Invoke();
    void InstallObserver(Dispatcher::Observer observer, const FAction& action) { m_onChangeDispatcher += {observer, action}; }
    void RemoveObserver(Dispatcher::Observer observer) { m_onChangeDispatcher -= observer; }

    virtual DelegateValue GetDelegateValue() const { return DelegateDefault; }
    virtual const QVariant* GetDelegateData() const { return nullptr; }
    virtual void SetDelegateData(const QVariant& value) { SetValue(value); }

    const QVariant& GetPreviousValue() const { return m_previousValue; }
    QVariant GetValue() const { return getValue(); }
    virtual QVariant GetMin() const { return 0; }
    virtual QVariant GetMax() const { return 0; }

protected:
    friend class PropertiesSystem;
    friend class PropertiesModel;

    virtual QVariant getDisplayValue() const { return getValue(); }
    virtual QVariant getValue() const=0;
    virtual void setValueInternal(const QVariant&)=0;

    Q_DISABLE_COPY(Property)

protected:
    Dispatcher m_onChangeDispatcher;
    FHandle m_fHandle;
    FOnChange m_fOnChange;
    FValidator m_fValidator;
    Options m_options;
    QVariant m_previousValue;
#ifdef DEBUG_BUILD
    bool m_isSubscribed;
#endif
};

template<class T>
class TPropertyBase : public Property
{
    typedef TPropertyBase Super;
public:
    typedef T value_type;
    TPropertyBase(const Name& path, const T& initial)
        : Property(path)
        , m_value(initial)
    {}

    // Avoid invoking. Sometimes it's helpfull
    void SetDirect(const T& value) { m_value = value; }

    const T& Native() const { return m_value; }
    const T* Ptr() const { return &m_value; }
    operator const T&() const { return m_value; }

    TPropertyBase<T>& operator=(const T& value) { this->SetValue(value); return *this; }

    template<class T2> const T2& Cast() const { return (const T2&)m_value; }

protected:
    T m_value;
};

template<class T>
class TProperty : public TPropertyBase<T>
{
    typedef TPropertyBase<T> Super;
public:
    TProperty(const Name& path, const T& initial)
        : TPropertyBase<T>(path, initial)
    {}

    TProperty<T>& operator=(const T& value) { this->SetValue(value); return *this; }

protected:
    QVariant getValue() const Q_DECL_OVERRIDE { return Super::m_value; }
    void setValueInternal(const QVariant& value) Q_DECL_OVERRIDE { Super::m_value = value.value<T>(); }
};

template<class T>
class TDecimalProperty : public TProperty<T>
{
    typedef TProperty<T> Super;
public:
    TDecimalProperty(const Name& path, const T& initial, const T& min, const T& max)
        : Super(path, initial)
        , m_min(min)
        , m_max(max)
    {}

    void SetMinMax(const T& min, const T& max)
    {
        m_min = min;
        m_max = max;
        if(Super::_value < m_min) {
            SetValue(m_min);
        }else if(Super::_value > m_max) {
            SetValue(m_max);
        }
    }

    const T& GetMinValue() const { return m_min; }
    const T& GetMaxValue() const { return m_max; }

    TDecimalProperty<T>& operator=(const T& value) { this->SetValue(value); return *this; }

    QVariant GetMin() const Q_DECL_OVERRIDE { return m_min; }
    QVariant GetMax() const Q_DECL_OVERRIDE { return m_max; }

protected:
    void setValueInternal(const QVariant& value) Q_DECL_OVERRIDE { this->m_value = clamp(value.value<T>(), m_min, m_max); }
protected:
    T m_min;
    T m_max;
};


// Extended
template<class T>
class PointerProperty : public TPropertyBase<T*>
{
    typedef TPropertyBase<T*> Super;
public:
    PointerProperty(const Name& path, T* initial)
        : Super(path, initial)
    {
        this->ChangeOptions().SetFlags(Super::Options_InternalProperty);
    }

    T* operator->() { return this->Native(); }
    const T* operator->() const { return this->Native(); }
    PointerProperty<T>& operator=(T* ptr) { this->SetValue(reinterpret_cast<size_t>(ptr)); return *this; }

    // Property interface
protected:
    QVariant getValue() const Q_DECL_OVERRIDE { return PropertyValueExtractorPrivate<typename Super::value_type>::ExtractVariant(Super::m_value); }
    void setValueInternal(const QVariant& value) Q_DECL_OVERRIDE { Super::m_value = reinterpret_cast<T*>(value.toLongLong()); }
};

class FileNameProperty : public TProperty<QString>
{
public:
    FileNameProperty(const Name& path, const QString& initial)
        : TProperty<QString>(path, initial)
    {}
    DelegateValue GetDelegateValue() const Q_DECL_OVERRIDE { return DelegateFileName; }
};

class _Export NamedUIntProperty : public TDecimalProperty<quint32>
{
    typedef TDecimalProperty<quint32> Super;
public:
    NamedUIntProperty(const Name& path, const quint32& initial)
        : Super(path, initial, 0, 0)
    {}

    void SetNames(const QStringList& names);

    DelegateValue GetDelegateValue() const Q_DECL_OVERRIDE { return DelegateNamedUInt; }
    const QVariant* GetDelegateData() const Q_DECL_OVERRIDE{ return &m_names; }

protected:
    QVariant getDisplayValue() const Q_DECL_OVERRIDE { return m_names.value<QStringList>().at(Super::m_value); }

private:
    QVariant m_names;
};

class _Export UrlListProperty : public TPropertyBase<QList<QUrl>>
{
    typedef TPropertyBase<QList<QUrl>> Super;
public:
    UrlListProperty(const Name& path, qint32 maxCount = -1)
        : Super(path, {})
        , m_maxCount(maxCount)
    {}

    void AddUniqueUrl(const QUrl& url);

    // Property interface
protected:
    QVariant getValue() const Q_DECL_OVERRIDE { return QUrl::toStringList(Super::m_value); }
    void setValueInternal(const QVariant& value) Q_DECL_OVERRIDE { Super::m_value = QUrl::fromStringList(value.toStringList()); }

private:
    qint32 m_maxCount;
};

template <class T>
class PropertiesValueToStringConverter
{
public:
    static QString ToString(const T& value);
    static T FromString(const QString& string);
};

template<class Key, class Value>
class _Export HashProperty : public TPropertyBase<QHash<Key, Value>>
{
    using Super = TPropertyBase<QHash<Key, Value>>;

public:
    HashProperty(const Name& path)
        : Super(path, {})
    {}

    void Insert(const Key& key, const Value& value)
    {
        QHash<Key, Value>& hash = Super::m_value;
        auto foundIt = hash.find(key);
        if(foundIt != hash.end()) {
            if(*foundIt != value) {
                *foundIt = value;
                Invoke();
            }
        } else {
            hash.insert(key, value);
            Invoke();
        }
    }

protected:
    QVariant getValue() const Q_DECL_OVERRIDE { return PropertiesValueToStringConverter<typename Super::value_type>::ToString(Super::m_value); }
    void setValueInternal(const QVariant& value) Q_DECL_OVERRIDE { Super::m_value = PropertiesValueToStringConverter<typename Super::value_type>::FromString(value.toString()); }
};

class _Export PropertiesDialogGeometryProperty : protected TProperty<QByteArray>
{
    typedef TProperty<QByteArray> Super;

public:
    PropertiesDialogGeometryProperty(const QString& name)
        : Super(Name("PropertiesDialogGeometry/" + name), QByteArray())
    {
        ChangeOptions().SetFlags(Option_IsExportable);
    }
};

// Internals
typedef TProperty<bool> BoolProperty;
typedef TDecimalProperty<double> DoubleProperty;
typedef TDecimalProperty<float> FloatProperty;
typedef TDecimalProperty<qint32> IntProperty;
typedef TDecimalProperty<quint32> UIntProperty;
typedef TProperty<QString> StringProperty;
typedef TProperty<QUrl> UrlProperty;
typedef TProperty<QByteArray> ByteArrayProperty;

#ifdef QT_GUI_LIB
#include <QColor>

#include <SharedGuiModule/internal.hpp>

class ColorProperty : public TProperty<QColor>
{
    typedef TProperty<QColor> Super;
public:
    ColorProperty(const Name& name, const QColor& initial)
        : Super(name, initial)
    {}

    DelegateValue GetDelegateValue() const Q_DECL_OVERRIDE { return DelegateColor; }
};

class RectProperty : public TProperty<Rect>
{
    typedef TProperty<Rect> Super;
public:
    RectProperty(const Name& name, const Rect& initial)
        : Super(name, initial)
    {}

    DelegateValue GetDelegateValue() const Q_DECL_OVERRIDE { return DelegateRect; }
};

class _Export Vector3FProperty
{
public:
    FloatProperty X;
    FloatProperty Y;
    FloatProperty Z;

    Vector3FProperty(const QString& path, const Vector3F& vector);
};

#endif // QT_GUI_LIB_LIB

#endif // PROPERTY_H
