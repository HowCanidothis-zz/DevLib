#ifndef PROMISE_H
#define PROMISE_H

#include <functional>
#include <atomic>
#include <set>
#include <mutex>
#include <condition_variable>

#include "SharedModule/MemoryManager/memorymanager.h"
#include "SharedModule/smartpointersadapters.h"
#include "SharedModule/shared_decl.h"
#include "SharedModule/dispatcher.h"

class PromiseData ATTACH_MEMORY_SPY(PromiseData)
{
public:
    using FCallback = std::function<void (bool)>;
    PromiseData()
        : m_result(false)
        , m_isResolved(false)
    {}
    ~PromiseData()
    {
        if(!m_isResolved) {
            resolve(false);
        }
    }

private:
    friend class Promise;
    bool m_result;
    std::atomic_bool m_isResolved;
    CommonDispatcher<bool> onFinished;
    std::mutex m_mutex;

    void resolve(bool value)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if(!m_isResolved) {
                m_isResolved = true;
                m_result = value;
                onFinished(value);
            }
        }

        onFinished -= this;
    }

    DispatcherConnection then(const FCallback& handler)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_isResolved) {
            handler(m_result);
            return DispatcherConnection();
        } else {
            return onFinished.Connect(this, handler);
        }
    }

    void mute()
    {
         onFinished -= this;
    }
};

class Promise
{
    SharedPointer<PromiseData> m_data;
public:

    Promise()
        : m_data(::make_shared<PromiseData>())
    {}

    PromiseData* GetData() const { return m_data.get(); }
    bool GetValue() const { return m_data->m_result; }
    bool IsResolved() const { return m_data->m_isResolved; }
    DispatcherConnection Then(const typename PromiseData::FCallback& handler) const { return m_data->then(handler); }
    void Resolve(bool value) const {  m_data->resolve(value); }
    void Mute() { m_data->mute(); }

    template<typename ... Args>
    static Promise OnFirstInvokeWithResult(CommonDispatcher<Args...>& dispatcher, const typename CommonDispatcher<Args...>::FCommonDispatcherActionWithResult& acceptHandler = [](Args...) { return true; })
    {
        Promise result;
        auto connections = ::make_shared<DispatcherConnectionsSafe>();
        dispatcher.Connect(nullptr, [result, connections, acceptHandler](Args... args){
            if(acceptHandler(args...)) {
                connections->clear();
                result.Resolve(true);
            }
        }).MakeSafe(*connections);
        return result;
    }
};

using AsyncResult = Promise;

class AsyncError : public AsyncResult
{
public:
    AsyncError() {
        Resolve(false);
    }
};

class AsyncSuccess : public AsyncResult
{
public:
    AsyncSuccess() {
        Resolve(true);
    }
};

class FutureResultData ATTACH_MEMORY_SPY(FutureResultData)
{
    template<class T> friend class QtFutureWatcher;
    friend class FutureResult;
    std::atomic<bool> m_result;
    std::atomic<int> m_promisesCounter;
    std::condition_variable m_conditional;
    std::mutex m_mutex;

    void ref()
    {
        m_promisesCounter++;
    }
    void deref()
    {
        m_promisesCounter--;

        if(isFinished()) {
            onFinished();
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_conditional.notify_one();
            }
            onFinished -= this;
        }
    }

    bool isFinished() const { return m_promisesCounter == 0; }
    bool getResult() const { return m_result; }

    void addPromise(const AsyncResult& promise, const SharedPointer<FutureResultData>& self)
    {
        ref();
        promise.Then([this, self](const bool& result){
            if(!result) {
                m_result = false;
            }
            deref();
        });

        return;
    }

    void then(const std::function<void (bool)>& action)
    {
        if(isFinished()) {
            action(getResult());
        } else {
            onFinished.Connect(this, [this, action]{
                action(m_result);
            });
        }
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while(!isFinished()) {
            m_conditional.wait(lock);
        }
    }

    Dispatcher onFinished;

public:
    FutureResultData()
        : m_result(true)
        , m_promisesCounter(0)
    {}

    ~FutureResultData()
    {

    }
};

class FutureResult ATTACH_MEMORY_SPY(FutureResult)
{
    template<class T> friend class QtFutureWatcher;
    SharedPointer<FutureResultData> m_data;
public:
    FutureResult()
        : m_data(::make_shared<FutureResultData>())
    {}

    bool IsFinished() const { return m_data->isFinished(); }
    bool GetResult() const { return m_data->getResult(); }

    void operator+=(const Promise& promise)
    {
        m_data->addPromise(promise, m_data);
    }

    void Then(const std::function<void (bool)>& action)
    {
        m_data->then(action);
    }

    void Wait()
    {
        m_data->wait();
    }
};

#include <QFuture>
#include <QFutureWatcher>
template<class T>
class QtFutureWatcher : public QFutureWatcher<T>
{
    using Super = QFutureWatcher<T>;
    QFuture<T> m_future;
    AsyncResult m_result;
public:
    QtFutureWatcher(const QFuture<T>& future, const AsyncResult& result)
        : m_future(future)
        , m_result(result)
    {
        Super::connect(this, &QtFutureWatcher<T>::finished, [this](){
            m_result.Resolve(true);
            this->deleteLater();
        });
        Super::setFuture(future);
    }
};

#endif // PROMISE_H
