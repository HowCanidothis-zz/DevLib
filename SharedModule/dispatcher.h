#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QHash>
#include <QMutex>

#include <functional>

#include "SharedModule/MemoryManager/memorymanager.h"
#include "smartpointersadapters.h"
#include "stack.h"

#define CONNECTION_DEBUG_LOCATION DEBUG_LOCATION

class DispatcherConnection;
using DispatcherConnectionSafePtr = SharedPointer<class DispatcherConnectionSafe>;

class DispatcherConnectionSafe
{
    using Super = FAction;
public:
    DispatcherConnectionSafe();
    DispatcherConnectionSafe(const FAction& disconnector);
    ~DispatcherConnectionSafe();

    void Disconnect();

private:
    template<typename... Args> friend class CommonDispatcher;
    void disable();

private:
    FAction m_disconnector;
};

using DispatcherConnectionsSafe = QVector<DispatcherConnectionSafePtr>;

inline SharedPointer<DispatcherConnectionsSafe> DispatcherConnectionsSafeCreate()
{
    return ::make_shared<DispatcherConnectionsSafe>();
}

class DispatcherConnection
{
    using FDispatcherRegistrator = std::function<void (const DispatcherConnectionSafePtr&)>;

    friend class DispatcherConnectionSafe;
    template<typename ... Args> friend class CommonDispatcher;
    FAction m_disconnector;
    FDispatcherRegistrator m_registrator;

    DispatcherConnection(const FAction& disconnector, const FDispatcherRegistrator& registrator);

public:
    DispatcherConnection();

    void Disconnect() const;

    DispatcherConnectionSafePtr MakeSafe();

    template<typename ... SafeConnections>
    void MakeSafe(SafeConnections&... connections)
    {
        auto connection = MakeSafe();
        adapters::Combine([&connection](DispatcherConnectionsSafe& connections){
            connections.append(::make_shared<DispatcherConnectionSafe>([connection]{
                connection->Disconnect();
            }));
        }, connections...);
    }
};

class DispatcherConnections : public QVector<DispatcherConnection>
{
    using Super = QVector<DispatcherConnection>;
public:
    using Super::Super;

    void Disconnect();

    void MakeSafe(DispatcherConnectionsSafe& safeConnections);

    template<typename ... SafeConnections>
    void MakeSafe(DispatcherConnectionsSafe& connections, SafeConnections&... additionalConnections)
    {
        for(auto& connection : *this) {
            connection.MakeSafe(connections, additionalConnections...);
        }
    }

    DispatcherConnectionsSafe MakeSafe();
};

template<class T> class LocalProperty;
template<class T> struct LocalPropertyOptional;

template<typename ... Args>
class CommonDispatcher
{
public:
    using Type = void (Args...);
    using Observer = const void*;
    using FCommonDispatcherAction = std::function<Type>;
    using FCommonDispatcherActionWithResult = std::function<bool (Args...)>;
    using ConnectionSubscribe = FCommonDispatcherAction;

    struct ActionHandler
    {
        Observer Key;
        FCommonDispatcherAction Handler;
    };

    CommonDispatcher()
        : m_lastId(0)
    {
    }

    virtual ~CommonDispatcher()
    {
        for(const auto& connection : m_safeConnections) {
            if(!connection.expired()) {
                connection.lock()->disable();
            }
        }
    }

    bool IsEmpty() const
    {
        return m_subscribes.isEmpty();
    }

    virtual void Invoke(Args... args) const
    {
        QHash<Observer, FCommonDispatcherAction> subscribesCopy;
        {
            QMutexLocker locker(&m_mutex);
            subscribesCopy = m_subscribes;
        }
        for(const auto& subscribe : subscribesCopy)
        {
            subscribe(args...);
        }
        QMap<qint32, ConnectionSubscribe> connectionSubscribesCopy;
        {
            QMutexLocker locker(&m_mutex);
            connectionSubscribesCopy = m_connectionSubscribes;
        }
        for(const auto& connections : connectionSubscribesCopy)
        {
            connections(args...);
        }
    }

    void operator()(Args... args) const
    {
        Invoke(args...);
    }

    DispatcherConnection ConnectFromWithParameters(const char* connectionInfo, CommonDispatcher& another) const
    {
        return another.Connect(connectionInfo, [this, connectionInfo](Args... args){
            Invoke(args...);
        });
    }

    DispatcherConnections ConnectBoth(const char* connectionDescription, CommonDispatcher& another) const
    {
        DispatcherConnections result;
        auto sync = ::make_shared<std::atomic_bool>(false);
        result += another.Connect(connectionDescription, [this, sync, connectionDescription](Args... args){
            if(!*sync) {
                *sync = true;
                Invoke(args...);
                *sync = false;
            }
        });
        result += Connect(connectionDescription, [this, &another, sync, connectionDescription](Args... args){
            if(!*sync) {
                *sync = true;
                another.Invoke(args...);
                *sync = false;
            }
        });
        return result;
    }

    DispatcherConnection ConnectAction(const char* connectionInfo, const FAction& handler) const
    {
        return Connect(connectionInfo, [handler, connectionInfo](Args...) { handler(); });
    }

    template<typename ... Dispatchers>
    DispatcherConnections ConnectCombined(const char* connectionInfo, const FAction& handler, Dispatchers&... args) const
    {
        DispatcherConnections result;
        adapters::Combine([this, &result, handler, connectionInfo](const auto& target){
            result += target.ConnectAction(connectionInfo, handler);
        }, *this, args...);
        return result;
    }

    template<typename Disp, typename ... Dispatchers>
    DispatcherConnections ConnectFrom(const char* locationInfo, const Disp& another, Dispatchers&... args) const
    {
        DispatcherConnections result;
        adapters::Combine([this, &result, locationInfo](const auto& target){
            result += target.ConnectAction(locationInfo, [this, locationInfo]{ Invoke(); });
        }, another, args...);
        return result;
    }

    template<typename ... Dispatchers>
    DispatcherConnections ConnectAndCallCombined(const char* connectionInfo, const FAction& handler, Dispatchers&... args) const
    {
        auto ret = ConnectCombined(connectionInfo, handler, args...);
        handler();
        return ret;
    }

    DispatcherConnection Connect(const char* locationInfo, const FCommonDispatcherAction& handler) const
    {
        QMutexLocker lock(&m_mutex);
        auto id = m_lastId++;
        m_connectionSubscribes.insert(id, handler);
        return DispatcherConnection([this, id, locationInfo]{
            FCommonDispatcherAction subscribe;
            QMutexLocker locker(&m_mutex);

            auto foundIt = m_connectionSubscribes.find(id);
            if(foundIt != m_connectionSubscribes.end()) {
                subscribe = foundIt.value();
                m_safeConnections.remove(id);
                m_connectionSubscribes.remove(id);
            }
        }, [this, id, locationInfo](const DispatcherConnectionSafePtr& connection){
            QMutexLocker lock(&m_mutex);
            m_safeConnections.insert(id, std::weak_ptr<DispatcherConnectionSafe>(connection));
        });
    }

    DispatcherConnection ConnectAndCall(const char* connectionInfo, const FAction& handler) const
    {
        auto result = ConnectAction(connectionInfo, handler);
        handler();
        return result;
    }

    const CommonDispatcher& operator+=(const ActionHandler& subscribeHandler) const
    {
        QMutexLocker lock(&m_mutex);
        Q_ASSERT(!m_subscribes.contains(subscribeHandler.Key));
        m_subscribes.insert(subscribeHandler.Key, subscribeHandler.Handler);
        return *this;
    }

    const CommonDispatcher& operator-=(Observer key) const
    {
        FCommonDispatcherAction action;
        {
            QMutexLocker lock(&m_mutex);
            {
                auto foundIt = m_subscribes.find(key);
                if(foundIt != m_subscribes.end()) {
                    action = foundIt.value();
                    m_subscribes.erase(foundIt);
                }
            }
        }
        return *this;
    }

    template<typename ... SafeConnections>
    DispatcherConnections OnFirstInvoke(const FCommonDispatcherAction& action) const
    {
        auto connections = ::make_shared<DispatcherConnections>();
        *connections += Connect(CONNECTION_DEBUG_LOCATION, [action, connections](Args... args){
            action(args...);
            connections->Disconnect();
        });
        return *connections;
    }

private:
    friend class Connection;
    mutable QHash<qint32, std::weak_ptr<DispatcherConnectionSafe>> m_safeConnections;
    mutable QMap<qint32, FCommonDispatcherAction> m_connectionSubscribes;
    mutable QHash<Observer, FCommonDispatcherAction> m_subscribes;
    mutable QMutex m_mutex;
    mutable qint32 m_lastId;
};

template<class T>
class WithDispatchersConnections : public T
{
    using Super = T;
public:
    using Super::Super;

    DispatcherConnections ConnectionsUnsafe;
};

using Dispatcher = CommonDispatcher<>;

DECLARE_WITH_FIELD(DispatcherConnectionsSafe, Connections);
DECLARE_WITH_FIELD(DispatcherConnections, ConnectionsUnsafe);

inline SharedPointer<Dispatcher> DispatcherCreate() { return ::make_shared<Dispatcher>(); }
Q_DECLARE_METATYPE(SharedPointer<DispatcherConnectionsSafe>)

#endif // NOTIFICATION_H
