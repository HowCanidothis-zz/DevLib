#include "gtscene.h"
#include "Objects/gtobjectbase.h"

GtScene::GtScene()
{
    m_initialized = false;
}

GtScene::~GtScene()
{
    foreachGtDrawableBase([](GtDrawableBase* object) {
        delete object;
    });
}

void GtScene::SetInitializationFunction(const GtScene::FInitializationFunction& function)
{
    Q_ASSERT(!m_initialized);
    m_initFunction = function;
}

void GtScene::Draw(OpenGLFunctions* f)
{
    for(auto* drawable : m_drawables) {
        drawable->Draw(f);
    }
}

Stack<GtInteractableBase*> GtScene::FindClosestToPoint(const Point3F& point, float tolerance) const
{
    Stack<GtInteractableBase*> result;
    for(auto* interactable : m_interactables) {
        if(interactable->DistanceToPoint(point) < tolerance) {
            result.Append(interactable);
        }
    }
    return result;
}

void GtScene::AddInteractable(GtInteractableBase *interactable)
{
    Q_ASSERT(!m_interactables.contains(interactable));
    m_interactables.insert(interactable);
}

void GtScene::AddDrawable(GtDrawableBase* drawable)
{
    Q_ASSERT(!m_drawables.contains(drawable));
    m_drawables.insert(drawable);
}

void GtScene::RemoveInteractable(GtInteractableBase* interactable)
{
    Q_ASSERT(m_interactables.contains(interactable));
    m_interactables.remove(interactable);
    delete interactable;
}

void GtScene::RemoveDrawable(GtDrawableBase* drawable)
{
    Q_ASSERT(m_drawables.contains(drawable));
    m_drawables.remove(drawable);
    delete drawable;
}

void GtScene::foreachGtDrawableBase(const std::function<void (GtDrawableBase*)>& action)
{
    for(auto* drawable : m_drawables) {
        action(drawable);
    }
    for(auto* interactable : m_interactables) {
        action(interactable);
    }
}
