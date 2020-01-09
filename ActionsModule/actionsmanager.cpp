#include "actionsmanager.h"

#include <QMenu>

#include "actionsscopebase.h"

ActionsManager::ActionsManager()
{

}

ActionsManager& ActionsManager::GetInstance()
{
    static ActionsManager result;
    return result;
}

void ActionsManager::CreateActionsFromRegisteredScopes()
{
    for(const auto& it : m_actionsScopes) {
        it.second->CreateActions();
    }
}

ActionsScopeBase* ActionsManager::FindScope(const Latin1Name& scopeName)
{
    auto found = m_actionsScopes.find(scopeName);
    if(found != m_actionsScopes.end()) {
        return found->second;
    }
    return nullptr;
}

bool ActionsManager::AddTo(const Latin1Name& scopeName, QMenu* menu)
{
    auto* scope = FindScope(scopeName);
    if(scope != nullptr) {
        for(auto* action : scope->GetActions()) {
            menu->addAction(action);
        }

        return true;
    }

    return false;
}

bool ActionsManager::AddTo(const Latin1Name& scopeName, QWidget* widget)
{
    auto* scope = FindScope(scopeName);
    if(scope != nullptr) {
        widget->addActions(scope->GetActionsQList());

        return true;
    }

    return false;
}

void ActionsManager::registerActionsScope(ActionsScopeBase* actionsScope)
{
    Q_ASSERT(m_actionsScopes.find(actionsScope->GetName()) == m_actionsScopes.end());

    m_actionsScopes.insert(std::make_pair(actionsScope->GetName(), actionsScope));
}

Action* ActionsManager::createAction(const Latin1Name& actionName, const FAction& action)
{
    const auto& it = m_actions.emplace(actionName, new Action(actionName, action)).first;
    return it->second;
}
