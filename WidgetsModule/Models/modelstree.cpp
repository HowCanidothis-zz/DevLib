#include "modelstree.h"

#include "modelstreeitembase.h"

void ModelsTree::remove(ModelsTreeItemBase* parent, const std::function<bool (ModelsTreeItemBase*)>& removePredicate)
{
    QSet<ModelsTreeItemBase*> toRemove;
    auto& childs = parent->m_childs;
    for(const auto& child : childs) {
        if(removePredicate(child.get())) {
            toRemove.insert(child.get());
        } else {
            remove(child.get(), removePredicate);
        }
    }

    if(!toRemove.isEmpty()) {
        auto newSize = childs.size() - toRemove.size();
        OnAboutToRemoveRows(newSize, childs.size() - 1, parent);
        auto endIt = std::remove_if(childs.begin(), childs.end(), [&toRemove](const SharedPointer<ModelsTreeItemBase>& child){
            return toRemove.contains(child.get());
        }); Q_UNUSED(endIt);
        childs.resize(newSize);
        OnRowsRemoved();
    }
}

void ModelsTree::ForeachChangeValue(const std::function<bool (ModelsTreeItemBase* item)>& handler)
{
    m_root->ForeachChild([handler, this](ModelsTreeItemBase* item){
        if(handler(item)) {
            OnTreeValueChanged(item, {});
        }
    });
}

void ModelsTree::Update(const std::function<void ()>& predicate)
{
    OnAboutToBeUpdated();
    predicate();
    OnUpdated();
}

ModelsTree::ModelsTree(ModelsTreeItemBase* root)
{
    m_root = root;
}

void ModelsTree::SetRoot(const ModelsTreeItemBasePtr& root)
{
    OnAboutToBeReseted();
    m_root = root;
    OnReseted();
}

void ModelsTree::Clear()
{
    OnAboutToBeReseted();
    m_root->m_childs.clear();
    OnReseted();
}

void ModelsTree::Add(const ModelsTreeItemBasePtr& item, ModelsTreeItemBase* parent)
{
    OnAboutToInsertRows(parent->GetChilds().size(), parent->GetChilds().size(), parent);
    parent->AddChild(item);
    OnRowsInserted();
}

void ModelsTree::Change(const std::function<void ()>& predicate)
{
    OnAboutToBeReseted();
    predicate();
    OnReseted();
}

void ModelsTree::Remove(ModelsTreeItemBase* item)
{
    if(item == m_root.get()) {
        return;
    }

    auto& childs = item->GetParent()->m_childs;
    auto row = item->GetRow();
    OnAboutToRemoveRows(row, row, item->GetParent());
    auto endIt = std::remove_if(childs.begin(), childs.end(), [item](const SharedPointer<ModelsTreeItemBase>& containerItem){
        return containerItem.get() == item;
    });
    childs.resize(std::distance(childs.begin(), endIt));
    OnRowsRemoved();
}

void ModelsTree::RemoveChildren(ModelsTreeItemBase* item)
{
    if (item->m_childs.isEmpty()) {
        return ;
    }
    OnAboutToRemoveRows(0, item->m_childs.size()-1, item);
    item->m_childs.clear();
    OnRowsRemoved();
}

void ModelsTree::SetChecked(qint64 key, ModelsTreeItemBase* item, Qt::CheckState checked) {
    if (item->GetChecked(key) == checked) {
        return;
    }

    item->SetChecked(key, checked);
    OnTreeValueChanged(item, {Qt::CheckStateRole});
//    OnChanged();
}

const SharedPointer<ModelsTreeItemBase>& ModelsTree::ItemPtrFromPointer(ModelsTreeItemBase* item) const
{
    auto* parent = item->GetParent();
    if(parent == nullptr) {
        return m_root;
    }
    return parent->GetChildPtr(item);
}

ModelsTreeItemBase* ModelsTree::ItemFromModelIndex(const QModelIndex& modelIndex)
{
    if(!modelIndex.isValid()) {
        return m_root.get();
    }
    return reinterpret_cast<ModelsTreeItemBase*>(modelIndex.internalPointer());
}

const SharedPointer<ModelsTreeItemBase>& ModelsTree::ItemPtrFromModelIndex(const QModelIndex& modelIndex)
{
    if(!modelIndex.isValid()) {
        return m_root;
    }
    auto item = reinterpret_cast<ModelsTreeItemBase*>(modelIndex.internalPointer());
    Q_ASSERT(item->GetParent() != nullptr);
    return item->GetParent()->GetChildPtr(item);
}
