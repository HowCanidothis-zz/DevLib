#include "widgetsglobaltableactionsscope.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QAction>

#include <ActionsModule/internal.hpp>

#include "WidgetsModule/Attachments/widgetsactivetableattachment.h"
#include "WidgetsModule/Utils/widgethelpers.h"

struct WidgetsGlobalTableActionsScopeHandlerData
{
    FAction Action;
    LocalPropertyBool Visibility;
    LocalPropertyBool Enablity;
    TranslatedString Text;

    WidgetsGlobalTableActionsScopeHandlerData(const FAction& action)
        : Action(action)
        , Visibility(false)
        , Enablity(false)
    {

    }
};

WidgetsGlobalTableActionsScopeHandler& WidgetsGlobalTableActionsScopeHandler::SetVisible(bool visible)
{
    m_data->Visibility = visible;
    m_data->Enablity = visible;
    return *this;
}

WidgetsGlobalTableActionsScopeHandler::WidgetsGlobalTableActionsScopeHandler(const FAction& action)
    : m_data(::make_shared<WidgetsGlobalTableActionsScopeHandlerData>(action))
{
}

LocalPropertyBool& WidgetsGlobalTableActionsScopeHandler::Visibility() const { return m_data->Visibility; }
LocalPropertyBool& WidgetsGlobalTableActionsScopeHandler::Enablity() const { return m_data->Enablity; }
TranslatedString& WidgetsGlobalTableActionsScopeHandler::Text() const { return m_data->Text; }

WidgetsGlobalTableActionsScopeHandler& WidgetsGlobalTableActionsScopeHandler::SetAction(const FAction& action)
{
    m_data->Action = action;
    return *this;
}

static QVector<QStringList> clipboardData()
{
    QVector<QStringList> ret;

    QClipboard* clipboard = qApp->clipboard();
    auto rows = clipboard->text().split('\n');
    for(const auto& row : rows){
        ret.append(row.split('\t'));
    }
    return ret;
}

WidgetsGlobalTableActionsScope::WidgetsGlobalTableActionsScope()
    : Super("TableEdit")
    , Singletone<WidgetsGlobalTableActionsScope>(this)
{
    if(!WidgetsGlobalTableActionId::m_delayedRegistration.isEmpty()) {
        for(const auto& [id, mode] : WidgetsGlobalTableActionId::m_delayedRegistration) {
            registerAction(id, mode);
        }

        WidgetsGlobalTableActionId::m_delayedRegistration.clear();
    }

    auto updateActiveTable = [this]{
        auto& activeTable = WidgetsActiveTableViewAttachment::GetInstance()->ActiveTable;

        if(activeTable == nullptr) {
            m_currentHandlers = nullptr;
            return;
        }
        m_currentHandlers = WidgetWrapper(activeTable).Injected<WidgetsGlobalTableActionsScopeHandlers>("a_tableActionsHandlers");
    };

    auto updateHandlers = [this]{
        m_currentHandlersConnections.clear();

        auto offAction = [](QAction* action) {
            ActionWrapper(action).ActionVisibility() = false;
            ActionWrapper(action).ActionEnablity() = false;
        };

        if(m_currentHandlers == nullptr) {
            for(auto* action : GetActions()) {
                offAction(action);
            }
            return;
        }

        for(auto* action : GetActions()) {
            auto handler = m_currentHandlers->Handlers.find(action);
            if(handler == m_currentHandlers->Handlers.end()) {
                offAction(action);
                continue;
            }

            (*ActionWrapper(action).Injected<FConnector>("a_connector"))(*handler);
        }
    };

    m_currentHandlers.Subscribe(updateHandlers);
    WidgetsActiveTableViewAttachment::GetInstance()->ActiveTable.SetAndSubscribe(updateActiveTable);
}

QVector<std::pair<Latin1Name, WidgetsGlobalTableActionsScope::EnableIfMode>> WidgetsGlobalTableActionId::m_delayedRegistration;

QAction* WidgetsGlobalTableActionsScope::registerAction(const Latin1Name& id, EnableIfMode mode)
{
    Q_ASSERT(FindAction(id) == nullptr);
    auto* action = createAction(id, [this, id]{
        auto* action = FindAction(id);
        if(m_currentHandlers == nullptr) {
            Q_ASSERT(false);
            return;
        }
        auto foundIt = m_currentHandlers->Handlers.find(action);
        if(foundIt == m_currentHandlers->Handlers.end()) {
            return;
        }
        foundIt.value().m_data->Action();
    });
    switch(mode)
    {
    case EIM_TableSelectionOnlyOne:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                auto& hasSelection = WidgetsActiveTableViewAttachment::GetInstance()->SelectedRowsCount;
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, [handler]{
                    return WidgetsActiveTableViewAttachment::GetInstance()->SelectedRowsCount == 1 && handler.Enablity();
                }, { &handler.Enablity().OnChanged, &hasSelection.OnChanged }).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
    case EIM_TableSelectionOnlyOneAndIsEditable:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                auto& isReadOnly = m_currentHandlers->IsReadOnly;
                auto& selectedRowsCount = WidgetsActiveTableViewAttachment::GetInstance()->SelectedRowsCount;
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, [this, handler]{
                    return !m_currentHandlers->IsReadOnly && handler.Enablity() && WidgetsActiveTableViewAttachment::GetInstance()->SelectedRowsCount == 1;
                }, { &handler.Enablity().OnChanged, &selectedRowsCount.OnChanged, &isReadOnly.OnChanged }).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
    case EIM_TableHasSelectionAndIsEditable:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                auto& isReadOnly = m_currentHandlers->IsReadOnly;
                auto& hasSelection = WidgetsActiveTableViewAttachment::GetInstance()->HasSelection;
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, [this, handler]{
                    return !m_currentHandlers->IsReadOnly && handler.Enablity() && WidgetsActiveTableViewAttachment::GetInstance()->HasSelection.Native();
                }, { &handler.Enablity().OnChanged, &hasSelection.OnChanged, &isReadOnly.OnChanged }).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
    case EIM_TableIsEditable:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                auto& hasSelection = m_currentHandlers->IsReadOnly;
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, [this, handler]{
                    return !m_currentHandlers->IsReadOnly && handler.Enablity();
                }, { &handler.Enablity().OnChanged, &hasSelection.OnChanged }).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
    case EIM_TableHasSelection:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                auto& hasSelection = WidgetsActiveTableViewAttachment::GetInstance()->HasSelection;
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, [handler]{
                    return WidgetsActiveTableViewAttachment::GetInstance()->HasSelection.Native() && handler.Enablity();
                }, { &handler.Enablity().OnChanged, &hasSelection.OnChanged }).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
    default:
        ActionWrapper(action).Injected<FConnector>("a_connector", [this, action]() -> FConnector* {
            return new FConnector([this, action](const WidgetsGlobalTableActionsScopeHandler& handler){
                ActionWrapper(action).ActionVisibility().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Visibility()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionEnablity().ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Enablity()).MakeSafe(m_currentHandlersConnections);
                ActionWrapper(action).ActionText()->ConnectFrom(CONNECTION_DEBUG_LOCATION, handler.Text()).MakeSafe(m_currentHandlersConnections);
            });
        });
        break;
    }

    return action;
}

WidgetsGlobalTableActionsScopeHandlersPtr WidgetsGlobalTableActionsScope::EditHandlers(QTableView* table)
{
    return WidgetWrapper(table).Injected<WidgetsGlobalTableActionsScopeHandlers>("a_tableActionsHandlers", []{
        return new WidgetsGlobalTableActionsScopeHandlers();
    });
}

WidgetsGlobalTableActionsScopeHandlersPtr WidgetsGlobalTableActionsScope::AddDefaultHandlers(QTableView* table)
{
    auto result = WidgetWrapper(table).Injected<WidgetsGlobalTableActionsScopeHandlers>("a_tableActionsHandlers", []{
        return new WidgetsGlobalTableActionsScopeHandlers();
    });

    Q_ASSERT(result->Handlers.isEmpty());

    auto action = GetInstance().FindAction(GlobalActionCopyWithHeadersId);
    result->AddHandler([]{ return tr("Copy with headers"); }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), action, [table]{
        WidgetTableViewWrapper(table).CopySelectedTableContentsToClipboard(true);
    });

    action = GetInstance().FindAction(GlobalActionCopyId);
    auto copyHandler = [table]{
        WidgetTableViewWrapper(table).CopySelectedTableContentsToClipboard();
    };

    result->AddHandler([]{ return tr("Copy"); }, QKeySequence(Qt::CTRL + Qt::Key_C), action, copyHandler);

    action = GetInstance().FindAction(GlobalActionPasteId);
    result->AddHandler([]{ return tr("Paste"); }, QKeySequence(Qt::CTRL + Qt::Key_V), action, [table]{
        auto setModelData = [](QTableView* tableView, const QList<int>& rowIdexs, const QVector<QStringList>& data){
            auto* model = tableView->model();
            const auto* header = tableView->horizontalHeader();

            auto dataRowIterator = data.begin();
            for(auto row : rowIdexs){
                if(dataRowIterator == data.end()){
                    break;
                }
                const auto& insertData = *dataRowIterator;
                qint32 column = 0;
                for(const auto& value : insertData) {
                    if(header->isSectionHidden(column)){
                        column++;
                        continue;
                    }
                    model->setData(model->index(row, column), value);
                    column++;
                }

                ++dataRowIterator;
            }
        };
        setModelData(table, WidgetTableViewWrapper(table).SelectedRowsSorted(), clipboardData());
    });


    action = GetInstance().FindAction(GlobalActionDeleteId);
    auto deleteHandler = [table]{
        auto* model = table->model();
        auto indexs = WidgetTableViewWrapper(table).SelectedRowsSorted();
        if(indexs.isEmpty()){
            return ;
        }
        int startSeries = indexs.first();
        int counter = startSeries + 1;

        QVector<std::pair<qint32,qint32>> toRemove; // first - from, second - count
        for(const auto& index : adapters::range(indexs.begin() + 1, indexs.end())){
            if(counter != index){
                toRemove.append({startSeries, counter - startSeries});
                startSeries = index;
                counter = startSeries + 1;
            } else {
                ++counter;
            }
        }
        toRemove.append({startSeries, counter - startSeries});
        while(!toRemove.isEmpty()){
            const auto& info = toRemove.takeLast();
            model->removeRows(info.first, info.second);
        }
    };

    result->AddHandler([]{ return tr("Delete Row(s)"); }, QKeySequence(Qt::SHIFT + Qt::Key_Delete), action, deleteHandler);

    action = GetInstance().FindAction(GlobalActionCutId);
    result->AddHandler([]{ return tr("Cut"); }, QKeySequence(Qt::CTRL + Qt::Key_X), action, [copyHandler, deleteHandler]{
        copyHandler();
        deleteHandler();
    });

    action = GetInstance().FindAction(GlobalActionInsertId);
    result->AddHandler([]{ return tr("Insert"); }, QKeySequence(Qt::CTRL + Qt::Key_Insert), action, [table]{
        auto* model = table->model();
        auto selectedRows = WidgetTableViewWrapper(table).SelectedRowsSorted();
        int rowIndex = model->rowCount();

        if(!selectedRows.isEmpty()){
            rowIndex = selectedRows.last();
        }

        model->insertRow(rowIndex);
    });

    return result;
}

void WidgetsGlobalTableActionsScopeHandlers::ShowAll()
{
    for(const auto& handler : Handlers) {
        handler.Visibility() = true;
        handler.Enablity() = true;
    }
}

void WidgetsGlobalTableActionsScopeHandlers::SetActionsVisible(const QVector<Latin1Name>& ids, bool visible)
{
    for(const auto& id : ids) {
        auto handler = FindHandler(id);
        handler.SetVisible(visible);
    }
}

WidgetsGlobalTableActionsScopeHandler WidgetsGlobalTableActionsScopeHandlers::AddHandler(const FTranslationHandler& handler, const QKeySequence& sequence, QAction* action, const FAction& actionHandler)
{
    auto ret = *Handlers.insert(action, actionHandler);
    action->setShortcut(sequence);
    ret.Text().SetTranslationHandler(handler);
    return ret;
}

WidgetsGlobalTableActionsScopeHandler WidgetsGlobalTableActionsScopeHandlers::AddHandler(const FTranslationHandler& handler, const QKeySequence& sequence, const class WidgetsGlobalTableActionId& globalActionId, const FAction& actionHandler)
{
    return AddHandler(handler, sequence, WidgetsGlobalTableActionsScope::GetInstance().FindAction(globalActionId), actionHandler);
}

WidgetsGlobalTableActionsScopeHandler WidgetsGlobalTableActionsScopeHandlers::FindHandler(const Latin1Name& id)
{
    auto* action = WidgetsGlobalTableActionsScope::GetInstance().FindAction(id);
    Q_ASSERT(action != nullptr);
    return Handlers[action];
}

WidgetsGlobalTableActionId::WidgetsGlobalTableActionId(const char* id, WidgetsGlobalTableActionsScope::EnableIfMode mode)
    : Super(id)
{
    if(WidgetsGlobalTableActionsScope::IsInitialized()) {
        WidgetsGlobalTableActionsScope::GetInstance().registerAction(*this, mode);
    } else {
        m_delayedRegistration.append(std::make_pair(*this, mode));
    }
}

IMPLEMENT_GLOBAL_ACTION(GlobalActionCopyId, WidgetsGlobalTableActionsScope::EIM_TableHasSelection)
IMPLEMENT_GLOBAL_ACTION(GlobalActionCopyWithHeadersId, WidgetsGlobalTableActionsScope::EIM_TableHasSelection)
IMPLEMENT_GLOBAL_ACTION(GlobalActionCutId, WidgetsGlobalTableActionsScope::EIM_TableHasSelectionAndIsEditable)
IMPLEMENT_GLOBAL_ACTION(GlobalActionPasteId, WidgetsGlobalTableActionsScope::EIM_TableIsEditable)
IMPLEMENT_GLOBAL_ACTION(GlobalActionDeleteId, WidgetsGlobalTableActionsScope::EIM_TableHasSelectionAndIsEditable)
IMPLEMENT_GLOBAL_ACTION(GlobalActionInsertId, WidgetsGlobalTableActionsScope::EIM_TableIsEditable)
