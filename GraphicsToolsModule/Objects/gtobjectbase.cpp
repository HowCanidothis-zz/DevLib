#include "gtobjectbase.h"

#include "GraphicsToolsModule/gtrenderer.h"

GtDrawableBase::GtDrawableBase(GtRenderer* renderer)
    : m_renderer(renderer)
    , m_destroyed(::make_shared<std::atomic_bool>(false))
    , m_rendererDrawable(false)
{
}

GtDrawableBase::~GtDrawableBase()
{
}

ThreadHandler GtDrawableBase::CreateThreadHandler()
{
    auto destroyed = m_destroyed;
    return [this, destroyed](const FAction& action) -> AsyncResult {
        if(*destroyed) {
            return AsyncError();
        }
        if(QThread::currentThread() == m_renderer) {
            action();
            return AsyncSuccess();
        } else {
            return m_renderer->Asynch([action]{
                action();
            });
        }
    };
}

void GtDrawableBase::delayedDraw(const FAction& draw)
{
    m_renderer->addDelayedDraw(draw);
}

ThreadHandlerNoThreadCheck GtDrawableBase::CreateThreadNoCheckHandler()
{
    auto destroyed = m_destroyed;
    return [destroyed, this](const FAction& action) -> AsyncResult {
        if(*destroyed) {
            return AsyncError();
        }
        return m_renderer->Asynch([action]{
            action();
        });
    };
}

void GtDrawableBase::enableDepthTest()
{
    m_renderer->enableDepthTest();
}

void GtDrawableBase::disableDepthTest()
{
    m_renderer->disableDepthTest();
}

AsyncResult GtDrawableBase::Destroy()
{
    auto result = Update([this](OpenGLFunctions* f){
        onDestroy(f);
    });
    *m_destroyed = true;
    if(!m_rendererDrawable) {
        m_renderer->RemoveDrawable(this);
    } else {
        m_renderer->Asynch([this]{
            delete this;
        });
    }
    return result;
}

AsyncResult GtDrawableBase::Update(const std::function<void (OpenGLFunctions*)>& f)
{
    if(*m_destroyed) {
        return AsyncError();
    }
    if(QThread::currentThread() == m_renderer) {
        f(m_renderer);
        return AsyncSuccess();
    }
    return m_renderer->Asynch([this, f]{
        f(m_renderer);
    });
}

AsyncResult GtDrawableBase::Update(const FAction& f)
{
    if(*m_destroyed) {
        return AsyncError();
    }
    if(QThread::currentThread() == m_renderer) {
        f();
        return AsyncSuccess();
    }
    return m_renderer->Asynch([f]{
        f();
    });
}

const GtRenderProperties& GtDrawableBase::getRenderProperties() const
{
    return m_renderer->m_renderProperties;
}

void GtDrawableBase::initialize(GtRenderer* renderer)
{
    onInitialize(renderer);
}
