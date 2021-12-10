#include "modelsvacabulary.h"

#include <QCompleter>

#include "modelsfiltermodelbase.h"
#include "modelslistbase.h"

#ifdef UNITS_MODULE_LIB
#include <UnitsModule/internal.hpp>
#endif

ModelsVocabulary::ModelsVocabulary(const HeaderData& dictionary)
    : m_header(dictionary)
{
#ifdef UNITS_MODULE_LIB
    QSet<Name> measurements;
    for(const auto& value : m_header) {
        if(!value.Measurement.IsNull()) {
            MeasurementTranslatedString::AttachToTranslatedString(*value.Label, value.Label->GetTranslationHandler(), { value.Measurement });
            measurements.insert(value.Measurement);
        }
    }
    for(const auto& measurementName : measurements) {
        const auto& measurement = MeasurementManager::GetInstance().GetMeasurement(measurementName);
        measurement->OnChanged.Connect(this, [this]{ UpdateUi([]{}); }).MakeSafe(m_connections);
    }

#endif
}

const QVariant& ModelsVocabulary::SelectValue(const Name& name, const QHash<Name, QVariant>& row)
{
    static QVariant result;
    auto foundIt = row.find(name);
    if(foundIt != row.end()) {
        return foundIt.value();
    }
    return result;
}

const ModelsVocabulary::HeaderDataValue& ModelsVocabulary::GetHeader(qint32 column) const
{
    static ModelsVocabulary::HeaderDataValue result = { Name(), ::make_shared<TranslatedString>([]{ return QString(); }), Name()};
    if(column < 0 || column >= m_header.size()) {
        return result;
    }
    return m_header.at(column);
}

TModelsListBase<ModelsVocabulary>* ModelsVocabulary::CreateListModel(qint32 column, QObject* parent)
{
    return new TModelsListBase<ModelsVocabulary>(parent, [column](const SharedPointer<ModelsVocabulary>& ptr, const QModelIndex& index, qint32 role) -> QVariant {
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
            if(index.row() == 0) {
                return "";
            }

#ifdef UNITS_MODULE_LIB
            const auto& header = ptr->GetHeader(column);
            QVariant value = ptr->SelectValue(header.ColumnKey, ptr->At(index.row() - 1));
            if(value.isValid() && !header.Measurement.IsNull()) {
                value = MeasurementManager::GetInstance().GetCurrentUnit(header.Measurement)->GetBaseToUnitConverter()(value.toDouble());
            }
            return value;
#else
            return ptr->SelectValue(header.ColumnKey, ptr->At(index.row() - 1));
#endif

        } else if(role == Qt::UserRole) {
            return QVariant(index.row() - 1);
        }
        return QVariant();
    }, [](const SharedPointer<ModelsVocabulary>& ptr){
        return ptr->GetSize() + 1;
    });
}

ModelsVocabularyViewModel::ModelsVocabularyViewModel(QObject* parent)
    : Super(parent)
{
    auto editRoleHandler = [this](qint32 row, qint32 column) -> QVariant {
        auto ret = GetData()->At(row)[GetData()->GetHeader(column).ColumnKey];
        auto foundIt = GetterDelegates.find(column);
        if(foundIt != GetterDelegates.end()) {
            foundIt.value()(ret);
        }
        return ret;
    };

    auto displayRoleHandlers = [this](qint32 row, qint32 column) -> QVariant {
        auto ret = GetData()->At(row)[GetData()->GetHeader(column).ColumnKey];
        auto foundIt = GetterDisplayDelegates.find(column);
        if(foundIt != GetterDisplayDelegates.end()) {
            foundIt.value()(ret);
        }
        return ret;
    };

    m_roleDataHandlers.insert(Qt::EditRole, editRoleHandler);
    m_roleDataHandlers.insert(Qt::DisplayRole, displayRoleHandlers);

    m_roleHorizontalHeaderDataHandlers.insert(Qt::DisplayRole, [this](qint32 section){
        return GetData()->GetHeader(section).Label->Native();
    });

    m_roleSetDataHandlers.insert(Qt::EditRole, [this](qint32 row, qint32 column, const QVariant& value) -> bool {
        QVariant concreteValue = value;
        auto foundIt = SetterDelegates.find(column);
        if(foundIt != SetterDelegates.end()) {
            foundIt.value()(concreteValue);
        }
        return GetData()->EditWithCheck(row, [&](QHash<Name, QVariant>& row) -> FAction {
            const auto& key = GetData()->GetHeader(column).ColumnKey;
            if(row[key] == concreteValue) {
                return nullptr;
            }
            return [&]{ row.insert(key, concreteValue); };
        }, { column });
});
}

bool ModelsVocabularyViewModel::setData(const QModelIndex& index, const QVariant& value, qint32 role)
{
    if(!index.isValid()) {
        return false;
    }

    if(isLastEditRow(index)) {
        if(role == Qt::EditRole) {
            QHash<Name, QVariant> newRow;
            GetData()->Append(newRow);
        } else {
            return false;
        }
    }

    return Super::setData(index, value, role);
}

QVariant ModelsVocabularyViewModel::data(const QModelIndex& index, qint32 role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    if(isLastEditRow(index)) {
        return QVariant();
    }

    return Super::data(index, role);
}

qint32 ModelsVocabularyViewModel::rowCount(const QModelIndex&) const
{
    return GetData() == nullptr ? 0 : (GetData()->GetSize() + 1);
}

qint32 ModelsVocabularyViewModel::columnCount(const QModelIndex&) const
{
    return GetData() != nullptr ? GetData()->GetColumnsCount() : 0;
}

Qt::ItemFlags ModelsVocabularyViewModel::flags(const QModelIndex& index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

ModelsVocabularyManager::ModelsVocabularyManager()
{}

void ModelsVocabularyManager::RegisterModel(const Name& modelName, const ModelsVocabularyPtr& vacabulary)
{
    Q_ASSERT(!m_models.contains(modelName));
    m_models.insert(modelName, vacabulary);
}

ModelsVocabularyManager& ModelsVocabularyManager::GetInstance()
{
    static ModelsVocabularyManager result;
    return result;
}

const ModelsVocabularyPtr& ModelsVocabularyManager::GetModel(const Name& modelName)
{
    Q_ASSERT(m_models.contains(modelName));
    return m_models[modelName];
}

const ModelsVocabularyManager::ViewModelDataPtr& ModelsVocabularyManager::CreateViewModel(const Name& modelName, qint32 columnIndex)
{
    Q_ASSERT(m_models.contains(modelName));
    auto data = ::make_shared<ViewModelData>();
    auto* sortModel = new ModelsFilterModelBase(nullptr);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    data->SortedModel = sortModel;
    const auto& model = m_models[modelName];

    if(columnIndex == -1) {
        sortModel->LessThan = [sortModel](const QModelIndex& f, const QModelIndex& s) {
            if(sortModel->IsLastEditRow(f)) {
                return sortModel->sortOrder() == Qt::AscendingOrder ? false : true;
            }
            if(sortModel->IsLastEditRow(s)) {
                return sortModel->sortOrder() == Qt::AscendingOrder ? true : false;
            }
            return sortModel->DefaultLessThan(f, s);
        };

        auto* sourceModel = new ModelsVocabularyViewModel(nullptr);
#ifdef UNITS_MODULE_LIB
        qint32 i(0);
        for(const auto& header : model->GetHeader()) {
            if(!header.Measurement.IsNull()) {
                auto measurement = header.Measurement;
                sourceModel->SetterDelegates.insert(i, [measurement](QVariant& value){
                    if(value.isValid()) {
                        value = MeasurementManager::GetInstance().GetCurrentUnit(measurement)->GetUnitToBaseConverter()(value.toDouble());
                    }
                });
                sourceModel->GetterDelegates.insert(i, [measurement](QVariant& value){
                    if(value.isValid()) {
                        value = MeasurementManager::GetInstance().GetCurrentUnit(measurement)->GetBaseToUnitConverter()(value.toDouble());
                    }
                });
                sourceModel->GetterDisplayDelegates.insert(i, [measurement](QVariant& value){
                    if(value.isValid()) {
                        value = MeasurementManager::GetInstance().FromBaseToUnitUi(measurement, value.toDouble());
                    }
                });
            }
            ++i;
        }
#endif
        sourceModel->SetData(model);
        data->SourceModel = sourceModel;
    } else {
        auto* listModel = model->CreateListModel(columnIndex, nullptr);
        listModel->SetData(model);
        data->SourceModel = listModel;
        auto* pData = data.get();
        sortModel->setDynamicSortFilter(false);
        listModel->GetData()->OnChanged += { this, [sortModel, pData]{
            pData->Sorter.Call([sortModel]{
                sortModel->sort(0, sortModel->sortOrder());
            });
        }};
    }
    data->SortedModel->setSourceModel(data->SourceModel);


    return m_cache[modelName].insert(columnIndex, data).value();
}

const ModelsVocabularyManager::ViewModelDataPtr& ModelsVocabularyManager::GetViewModel(const Name& modelName, qint32 column)
{
    return m_cache[modelName][column];
}

QCompleter* ModelsVocabularyManager::CreateCompleter(const Name& modelName, qint32 column, QObject* parent, ModelsVocabularyRequest* dispatcher)
{
    auto* completer = new QCompleter(parent);
    completer->setCompletionRole(Qt::DisplayRole);
    completer->setCompletionColumn(0);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setModel(m_cache[modelName][column]->SortedModel);
    if(dispatcher != nullptr) {
        completer->connect(completer, QOverload<const QModelIndex&>::of(&QCompleter::activated), [modelName, dispatcher](const QModelIndex& index){
            dispatcher->Invoke(index.data(Qt::UserRole).toInt());
        });
    }
    return completer;
}