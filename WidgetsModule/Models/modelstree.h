#ifndef MODELSTREE_H
#define MODELSTREE_H

#include <SharedModule/internal.hpp>

#include "wrappers.h"

class ModelsTreeItemBase;

class ModelsTree : public ModelsTreeWrapper
{
public:

    ModelsTree(ModelsTreeItemBase* root = nullptr);

    void SetRoot(const SharedPointer<ModelsTreeItemBase>& root);

    void Clear();
    void Add(const SharedPointer<ModelsTreeItemBase>& item, ModelsTreeItemBase* parent);
    void Update(const std::function<void ()>& predicate);
    void Remove(ModelsTreeItemBase* item);
    void Remove(const std::function<bool (ModelsTreeItemBase*)>& predicate) { remove(m_root.get(), predicate); }
    void SetChecked(ModelsTreeItemBase* item, Qt::CheckState checked);

    const SharedPointer<ModelsTreeItemBase>& GetRootPtr() const { return m_root; }

    ModelsTreeItemBase* GetRoot() override { return m_root.get(); }

    ModelsTreeItemBase* ItemFromModelIndex(const QModelIndex& modelIndex);
    const SharedPointer<ModelsTreeItemBase>& ItemPtrFromModelIndex(const QModelIndex& modelIndex);

private:
    void remove(ModelsTreeItemBase* parent, const std::function<bool (ModelsTreeItemBase*)>& removePredicate);

private:
    SharedPointer<ModelsTreeItemBase> m_root;
};

using ModelsTreePtr = SharedPointer<ModelsTree>;

#endif // MODELSTREE_H
