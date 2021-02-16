#include "gtobjectbase.h"

#include "GraphicsToolsModule/gtrenderer.h"

GtDrawableBase::GtDrawableBase(GtRenderer* renderer)
    : m_renderer(renderer)
    , m_destroyed(false)
{
}

GtDrawableBase::~GtDrawableBase()
{
}

ThreadHandler GtDrawableBase::CreateThreadHandler()
{
    return [this](const FAction& action) -> AsyncResult {
        if(m_destroyed) {
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

void GtDrawableBase::enableDepthTest()
{
    m_renderer->enableDepthTest();
}

void GtDrawableBase::disableDepthTest()
{
    m_renderer->disableDepthTest();
}

void GtDrawableBase::Destroy()
{
    Update([this](OpenGLFunctions* f){
        onDestroy(f);
    });
    m_destroyed = true;
    m_renderer->RemoveDrawable(this);
}

void GtDrawableBase::Update(const std::function<void (OpenGLFunctions*)>& f)
{
    if(m_destroyed) {
        return;
    }
    if(QThread::currentThread() == m_renderer) {
        f(m_renderer);
    } else {
        m_renderer->Asynch([this, f]{
            f(m_renderer);
        });
    }
}

void GtDrawableBase::Update(const FAction& f)
{
    if(m_destroyed) {
        return;
    }
    if(QThread::currentThread() == m_renderer) {
        f();
    } else {
        m_renderer->Asynch([f]{
            f();
        });
    }
}

const GtRenderProperties& GtDrawableBase::getRenderProperties() const
{
    return m_renderer->m_renderProperties;
}

void GtDrawableBase::initialize(GtRenderer* renderer)
{
    onInitialize(renderer);
}
