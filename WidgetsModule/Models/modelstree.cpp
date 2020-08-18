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
        std::remove_if(childs.begin(), childs.end(), [&toRemove](const SharedPointer<ModelsTreeItemBase>& child){
            return toRemove.contains(child.get());
        });
        childs.resize(newSize);
        OnRowsRemoved();
    }
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

void ModelsTree::Update(const std::function<void ()>& predicate)
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

void ModelsTree::SetChecked(ModelsTreeItemBase* item, Qt::CheckState checked) {
    if (item->Checked() == checked) {
        return;
    }

    auto func = [this](ModelsTreeItemBase* item){ OnTreeValueChanged(item, {Qt::CheckStateRole}); };

    if (checked == Qt::Checked) {
        QVector<ModelsTreeItemBase*> stack;
        auto p = item->GetParent();
        while (p && p->Checked() == item->Checked()) {
            stack.append(p);
            p = p->GetParent();
        }

        ///делаем активными всех парентов
        for (int i=stack.size()-1; i>=0; --i) {
            stack[i]->SetChecked(Qt::Checked, ModelsTreeItemBase::CurrentNode, func);
        }
        ///делаем активными текущую ноду и всех чилдов
        item->SetChecked(checked, ModelsTreeItemBase::CurrentWithChilds, func);
    } else {
        bool isStop = false;
        QVector<ModelsTreeItemBase*> stack;

        auto p = item;
        while (p->GetParent()) {
            for (auto ch : p->GetParent()->GetChilds()) {
                if (ch->Id() == p->Id()) {
                    continue;
                }
                //если другой узел активен, перестаем выключать парентов
                if (ch->Checked() == Qt::Checked) {
                    isStop = true;
                    break;
                }
            }
            if (isStop) {
                break;
            }
            stack.append(p->GetParent());
            p = p->GetParent();
        }
        ///делаем неактивными текущую ноду и всех чилдов
        item->SetChecked(checked, ModelsTreeItemBase::CurrentWithChilds, func);
        ///делаем неактивными парентов
        for (int i=0; i<stack.size(); ++i){
            stack[i]->SetChecked(checked, ModelsTreeItemBase::CurrentNode, func);
        }
    }
    OnChanged();
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
    const auto& childs = item->GetParent()->m_childs;
    auto foundIt = std::find_if(childs.begin(), childs.end(), [item](const SharedPointer<ModelsTreeItemBase>& containerItem){
        return containerItem.get() == item;
    });
    Q_ASSERT(foundIt != childs.end());
    return *foundIt;
}
