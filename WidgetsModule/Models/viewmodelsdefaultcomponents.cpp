#include "viewmodelsdefaultcomponents.h"

#ifdef UNITS_MODULE_LIB
#include <UnitsModule/internal.hpp>
#endif

std::function<QVariant(const QTime&)> TViewModelsColumnComponentsBuilderBase::TimeConveerterHandler([](const QTime& dt)->QVariant { return dt.toString(); });
std::function<QVariant(const QDateTime&)> TViewModelsColumnComponentsBuilderBase::DateTimeConveerterHandler([](const QDateTime& dt)->QVariant { return dt.toString(); });

TViewModelsColumnComponentsBuilderBase& TViewModelsColumnComponentsBuilderBase::AddDefaultColors(LocalPropertyColor* enabledCellColor, LocalPropertyColor* disabledCellColor,
                               LocalPropertyColor* enabledTextColor, LocalPropertyColor* disabledTextColor)
{
    auto* model = m_viewModel;
    m_viewModel->ColumnComponents.AddComponent(Qt::BackgroundRole, -1, ViewModelsTableColumnComponents::ColumnComponentData()
                                         .SetGetter([model, enabledCellColor, disabledCellColor](const QModelIndex& index) {
                                            if(model->flags(index).testFlag(Qt::ItemIsEditable)) {
                                                return enabledCellColor->Native();
                                            }
                                            return disabledCellColor->Native();
                                         }));
    m_viewModel->ColumnComponents.AddComponent(Qt::TextColorRole, -1, ViewModelsTableColumnComponents::ColumnComponentData()
                                         .SetGetter([model, enabledTextColor, disabledTextColor](const QModelIndex& index) {
                                            if(model->flags(index).testFlag(Qt::ItemIsEditable)) {
                                                return enabledTextColor->Native();
                                            }
                                            return disabledTextColor->Native();
                                         }));
    return *this;
}
