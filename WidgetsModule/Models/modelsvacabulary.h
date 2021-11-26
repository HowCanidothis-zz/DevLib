#ifndef MODELSVACABULARY_H
#define MODELSVACABULARY_H

#include <PropertiesModule/internal.hpp>

#include "modelstablebase.h"

template<class T>
class TModelsListBase;

using ModelsVocabularyRequest = CommonDispatcher<qint32>;

class ModelsVocabulary : public TModelsTableWrapper<QVector<QHash<Name, QVariant>>>
{
    using Super = TModelsTableWrapper<QVector<QHash<Name, QVariant>>>;
public:
    struct HeaderDataValue
    {
        Name ColumnKey;
        TranslatedStringPtr Label;
        Name Measurement;
    };
    using HeaderData = QVector<HeaderDataValue>;

    ModelsVocabulary(const HeaderData& dictionary);

    template<class Property>
    DispatcherConnection CreatePropertyConnection(LocalPropertyOptional<Property>* property, const Name& name, ModelsVocabularyRequest* indexDispatcher)
    {
        return indexDispatcher->Connect(this, [this, name, property](qint32 index){
            if(!IsValidRow(index)) {
                return;
            }
            property->FromVariant(SelectValue(name, At(index)));
        });
    }

    static const QVariant& SelectValue(const Name& name, const QHash<Name, QVariant>& row);

    qint32 GetColumnsCount() const { return m_header.size(); }
    const HeaderDataValue& GetHeader(qint32 column) const;
    const QVector<HeaderDataValue>& GetHeader() const { return m_header; }

    static TModelsListBase<ModelsVocabulary>* CreateListModel(qint32 column, QObject* parent);

private:
    QVector<HeaderDataValue> m_header;
    DispatcherConnectionsSafe m_connections;
};

using ModelsVocabularyPtr = SharedPointer<ModelsVocabulary>;

class ModelsVocabularyViewModel : public TModelsTableBase<ModelsVocabulary>
{
    using Super = TModelsTableBase<ModelsVocabulary>;
public:
    ModelsVocabularyViewModel(QObject* parent = nullptr);

    bool setData(const QModelIndex& index, const QVariant& value, qint32 role) override;
    QVariant data(const QModelIndex& index, qint32 role) const override;
    qint32 rowCount(const QModelIndex&) const override;
    qint32 columnCount(const QModelIndex&) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QHash<qint32, std::function<void (QVariant&)>> GetterDisplayDelegates;
    QHash<qint32, std::function<void (QVariant&)>> GetterDelegates;
    QHash<qint32, std::function<void (QVariant&)>> SetterDelegates;
};

class ModelsVocabularyManager
{
    ModelsVocabularyManager();
public:
    struct ViewModelData
    {
        class ModelsFilterModelBase* SortedModel;
        QAbstractItemModel* SourceModel;
        DelayedCallObject Sorter;

        ViewModelData()
            : SortedModel(nullptr)
            , SourceModel(nullptr)
            , Sorter(1000)
        {}
    };
    using ViewModelDataPtr = SharedPointer<ViewModelData>;

    static ModelsVocabularyManager& GetInstance();

    void RegisterModel(const Name& modelName, const ModelsVocabularyPtr& vacabulary);
    const ModelsVocabularyPtr& GetModel(const Name& modelName);
    const ViewModelDataPtr& CreateViewModel(const Name& modelName, qint32 columnIndex);
    const ViewModelDataPtr& GetViewModel(const Name& modelName, qint32 column);
    class QCompleter* CreateCompleter(const Name& modelName, qint32 column, QObject* parent, ModelsVocabularyRequest* dispatcher);

    LocalPropertyLocale Locale;

private:
    QHash<Name, ModelsVocabularyPtr> m_models;
    QHash<Name, QHash<qint32, ViewModelDataPtr>> m_cache;
};

#endif // MODELSVACABULARY_H
