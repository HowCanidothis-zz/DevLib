#include "delegates.h"

#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QApplication>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QWidgetAction>
#include <QTableView>
#include <QMenu>
#include <QTextDocument>
#include <QDesktopWidget>
#include <QAbstractTextDocumentLayout>
#include <SharedModule/internal.hpp>

#include "WidgetsModule/Widgets/widgetsspinboxwithcustomdisplay.h"
#include "WidgetsModule/Utils/widgethelpers.h"
#include "WidgetsModule/Widgets/DateTime/widgetsdatetimepopuppicker.h"
#include "WidgetsModule/Widgets/DateTime/widgetsdatetimewidget.h"
#include "WidgetsModule/Widgets/DateTime/widgetstimewidget.h"
#include "WidgetsModule/Widgets/DateTime/widgetsdatetimeedit.h"

DelegatesComboboxCustomViewModel::DelegatesComboboxCustomViewModel(const ModelGetter& getter, QObject* parent)
    : Super([]()-> QStringList { return {}; }, parent)
    , m_getter(getter)
{}

QWidget* DelegatesComboboxCustomViewModel::createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex&) const
{
    QComboBox* comboBox = new QComboBox(parent);
    auto* model = m_getter();
    WidgetComboboxWrapper(comboBox).Make([this, comboBox, model](const WidgetComboboxWrapper& wrapper){
        wrapper.CreateCompleter(model, [this, comboBox](const QModelIndex&){
            if(comboBox->lineEdit() != nullptr) {
                return;
            }
            auto* nonConstThis = const_cast<DelegatesComboboxCustomViewModel*>(this);
            emit nonConstThis->commitData(comboBox);
            emit nonConstThis->closeEditor(comboBox);
        });
        wrapper.BlockWheel();
    });
    return comboBox;
}

DelegatesCombobox::DelegatesCombobox(QObject* parent)
    : DelegatesCombobox([]{ return QStringList(); }, parent)
{

}

DelegatesCombobox::DelegatesCombobox(const std::function<QStringList ()>& valuesExtractor, QObject* parent)
    : Super(parent)
    , m_valuesExtractor(valuesExtractor)
    , m_aligment(Qt::AlignCenter)
    , m_drawCombobox(false)
    , m_initializeHandler([](QComboBox* , const QModelIndex& ){ return false; })
{}

QWidget* DelegatesCombobox::createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex& index) const
{
    QComboBox* comboBox = new QComboBox(parent);
    WidgetWrapper(comboBox).BlockWheel();
    if(!InitializeHandler(comboBox, index)){
        comboBox->addItems(m_valuesExtractor());
    }
    for (int i = 0; i < comboBox->count() ; ++i) {
        comboBox->setItemData(i, m_aligment, Qt::TextAlignmentRole);
    }
    auto commitData = [this, comboBox](QAbstractItemDelegate::EndEditHint hints){
        if(comboBox->lineEdit() != nullptr) {
            return;
        }
        auto* nonConstThis = const_cast<DelegatesCombobox*>(this);
        emit nonConstThis->commitData(comboBox);
        emit nonConstThis->closeEditor(comboBox, hints);
    };
    connect(comboBox, QOverload<qint32>::of(&QComboBox::activated), [commitData]{
        commitData(QAbstractItemDelegate::NoHint);
    });
    WidgetWrapper(comboBox->view()).AddEventFilter([commitData, parent, comboBox](QObject*, QEvent* e){
        if(e->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(e);
            if(ke->key() == Qt::Key_Tab) {
                comboBox->setCurrentIndex(comboBox->view()->currentIndex().row());
                if(ke->modifiers() & Qt::ShiftModifier) {
                    commitData(QAbstractItemDelegate::EditPreviousItem);
                } else {
                    commitData(QAbstractItemDelegate::EditNextItem);
                }
                return true;
            }
        }
        return false;
    });
    WidgetWrapper(comboBox).AddEventFilter([commitData, parent, comboBox](QObject*, QEvent* e){
        if(e->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(e);
            switch(ke->key()) {
            case Qt::Key_Up:
                comboBox->setCurrentIndex(quint32(comboBox->currentIndex() - 1) % comboBox->count());
                return true;
            case Qt::Key_Down:
                comboBox->setCurrentIndex((comboBox->currentIndex() + 1) % comboBox->count());
                return true;
            case Qt::Key_Tab:
                if(ke->modifiers() & Qt::ShiftModifier) {
                    commitData(QAbstractItemDelegate::EditPreviousItem);
                } else {
                    commitData(QAbstractItemDelegate::EditNextItem);
                }
                return true;
            default: break;
            }
        }
        return false;
    });
    return comboBox;
}

void DelegatesCombobox::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QString data = index.model()->data(index, Qt::EditRole).toString();

    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    Q_ASSERT(comboBox != nullptr);
    OnEditorAboutToBeShown(comboBox, index);

    comboBox->setCurrentText(data);
}

void DelegatesCombobox::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QComboBox* combo = static_cast<QComboBox*>(editor);
    const QString& text = combo->currentText();
    OnAboutToSetModelData(combo, index);
    model->setData(index, text, Qt::EditRole);
}

void DelegatesCombobox::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const {
    editor->setGeometry(option.rect);
}

void DelegatesCombobox::paint(QPainter* painter, const QStyleOptionViewItem& inOption, const QModelIndex& index) const
{
    if(!m_drawCombobox && !m_drawRichText) {
        Super::paint(painter, inOption, index);
        return ;
    }
    if(m_drawRichText) {
        QStyleOptionViewItem option = inOption;
        initStyleOption(&option, index);

        QStyle* style = option.widget? option.widget->style() : QApplication::style();

        QTextOption textOption;
        textOption.setAlignment(option.displayAlignment);
        textOption.setWrapMode(QTextOption::NoWrap);

        QTextDocument doc;
        option.font.setPixelSize(option.font.pixelSize() + 1);
        doc.setDefaultFont(option.font);
        doc.setDefaultTextOption(textOption);
        doc.setTextWidth(option.rect.width());
        doc.setIndentWidth(0.0);
        doc.setHtml(option.text);

        /// Painting item without text
        option.text = QString();
        option.state &= ~QStyle::State_HasFocus;
        style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

        QAbstractTextDocumentLayout::PaintContext ctx;

        ctx.palette = option.palette;

        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
        painter->save();
        painter->translate(textRect.topLeft());
        painter->setClipRect(textRect.translated(-textRect.topLeft()));
        doc.documentLayout()->draw(painter, ctx);
        painter->restore();
    } else {
        QStyleOptionComboBox opt;
        static QComboBox cb;

        opt.initFrom(&cb);
        opt.editable = false;
        opt.currentText = index.data().toString();
        opt.rect = inOption.rect;
        opt.state = inOption.state;

        auto style = cb.style();
        style->drawComplexControl(QStyle::CC_ComboBox, &opt, painter, &cb);
        style->drawControl(QStyle::CE_ComboBoxLabel, &opt, painter, &cb);
    }
}

DelegatesIntSpinBox::DelegatesIntSpinBox(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_editHandler([](QAbstractItemModel*, const QModelIndex&)->bool {return true;})//todo static default
{

}

QWidget* DelegatesIntSpinBox::createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex& index) const
{
    auto* spin = new WidgetsSpinBoxWithCustomDisplay(parent);
    OnEditorAboutToBeShown(spin, index);
    connect(spin, QOverload<qint32>::of(&QSpinBox::valueChanged), [this, index](int value){
        OnEditorValueChanged(value, index);
    });
    return spin;
}

void DelegatesIntSpinBox::setEditorData(QWidget* editor, const QModelIndex& index) const {
    auto data = index.model()->data(index, Qt::EditRole).toInt();
    auto* spin = qobject_cast<QSpinBox*>(editor);
    Q_ASSERT(spin != nullptr);
    spin->setValue(data);
}

void DelegatesIntSpinBox::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    auto* spin = static_cast<QSpinBox*>(editor);
    const auto& val = ::clamp(spin->value(), spin->minimum(), spin->maximum());
    
    bool accept = true;
    OnEditingFinished(val, index, accept);
    if(accept) {
        model->setData(index, val, Qt::EditRole);
    }
}

void DelegatesIntSpinBox::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const {
    editor->setGeometry(option.rect);
}

bool DelegatesIntSpinBox::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if(event->type() == QEvent::MouseButtonDblClick){
        auto isEditable = m_editHandler(model, index);
        if (!isEditable){///block
            return true;
        }
    }
    return Super::editorEvent(event, model, option, index);
}

void DelegatesIntSpinBox::SetEditHandler(const std::function<bool (QAbstractItemModel*, const QModelIndex&)>& handler){
    m_editHandler = handler;
}

DelegatesDoubleSpinBox::DelegatesDoubleSpinBox(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_editHandler([](QAbstractItemModel*, const QModelIndex&)->bool {return true;})//todo static default
{
}

QWidget* DelegatesDoubleSpinBox::createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex& index) const
{
    auto* spin = new WidgetsDoubleSpinBoxWithCustomDisplay (parent);
    spin->MakeOptional();
    OnEditorAboutToBeShown(spin, index);
    connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, index](double value){
        OnEditorValueChanged(value, index);
    });
    return spin;
}

void DelegatesDoubleSpinBox::setEditorData(QWidget* editor, const QModelIndex& index) const {
    double data = index.model()->data(index, Qt::EditRole).toDouble();

    QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(editor);
    Q_ASSERT(spin != nullptr);
    spin->setValue(data);
}

void DelegatesDoubleSpinBox::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    WidgetsDoubleSpinBoxWithCustomDisplay* spin = static_cast<WidgetsDoubleSpinBoxWithCustomDisplay*>(editor);
    double val = spin->value();

    bool accept = true;
    OnEditingFinished(val, index, accept);
    if(accept) {
        model->setData(index, spin->IsValid() ? val : QVariant(), Qt::EditRole);
    }
}

void DelegatesDoubleSpinBox::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const {
    editor->setGeometry(option.rect);
}

bool DelegatesDoubleSpinBox::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if(event->type() == QEvent::MouseButtonDblClick){
        auto isEditable = m_editHandler(model, index);
        if (!isEditable){///block
            return true;
        }
    }
    return Super::editorEvent(event, model, option, index);
}

void DelegatesDoubleSpinBox::SetEditHandler(const std::function<bool (QAbstractItemModel*, const QModelIndex&)>& handler){
    m_editHandler = handler;
}

DelegatesDateTime::DelegatesDateTime(QObject* parent)
    : QStyledItemDelegate(parent)
{
    
}

QWidget* DelegatesDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& , const QModelIndex& index) const
{
    auto* editor = new QDateTimeEdit(parent);
	OnEditorAboutToBeShown(editor, index);
    connect(editor,&QDateTimeEdit::dateTimeChanged, [this, index](const QDateTime&dateTime){
        OnEditorValueChanged(dateTime, index);
    });
    return editor;
}

void DelegatesDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const {
    const auto& dateTime = index.model()->data(index, Qt::EditRole).toDateTime();
	
    QDateTimeEdit* dt = qobject_cast<QDateTimeEdit*>(editor);
    Q_ASSERT(dt != nullptr);
	dt->setTimeSpec(dateTime.timeSpec());
    dt->setDateTime(dateTime);
}

void DelegatesDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QDateTimeEdit* dt = static_cast<QDateTimeEdit*>(editor);
    model->setData(index, dt->dateTime(), Qt::EditRole);
}

void DelegatesDateTime::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const {
    editor->setGeometry(option.rect);
}

QString DelegatesDateTime::displayText(const QVariant& value, const QLocale&) const
{
    return value.toString();
}

DelegatesCheckBox::DelegatesCheckBox(QObject* parent)
    : QStyledItemDelegate(parent)
{

}

void DelegatesCheckBox::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value = index.data(Qt::EditRole);
    if(value.isNull() || index.data(Qt::CheckStateRole).isValid()){
        Super::paint(painter, option, index);
        return;
    }
    QStyleOptionButton checkboxstyle;
    QRect checkbox_rect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator,&checkboxstyle, option.widget);
    
    checkboxstyle.rect = option.rect;
    checkboxstyle.rect.setLeft(option.rect.x() - checkbox_rect.x() +
                                 option.rect.width()/2 - checkbox_rect.width()/2);

    if(value.toBool()) {
        checkboxstyle.state = QStyle::State_On|QStyle::State_Enabled;
    } else {
        checkboxstyle.state = QStyle::State_Off|QStyle::State_Enabled;
    }
    Super::paint(painter, option, index);
    QApplication::style()->drawControl(QStyle::CE_CheckBox,&checkboxstyle, painter, option.widget);
}

bool DelegatesCheckBox::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if(index.flags().testFlag(Qt::ItemIsEditable)) {
        if(event->type() == QEvent::MouseButtonRelease){
            model->setData(index, !model->data(index, Qt::EditRole).toBool());
            event->accept();
        } else if(event->type() == QEvent::MouseButtonDblClick){
            event->accept();
            return true;
        }
    }
    return false;
}

DelegatesDateTimePicker::DelegatesDateTimePicker(QObject* parent)
    : Super(parent)
{

}

QWidget* DelegatesDateTimePicker::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto widget = new WidgetsDatetimePopupPicker(parent);
    widget->GetDateTimeWidget()->CurrentDateTime = index.model()->data(index, Qt::EditRole).toDateTime();
    OnEditorAboutToBeShown(widget, index);
    widget->OnDataCommit.Connect(CONNECTION_DEBUG_LOCATION, [this, widget]{
        auto* nonConst = const_cast<DelegatesDateTimePicker*>(this);
        nonConst->emit commitData(widget);
    });

    widget->OnCloseEditor.Connect(CONNECTION_DEBUG_LOCATION, [this, widget]{
        auto* nonConst = const_cast<DelegatesDateTimePicker*>(this);
        nonConst->emit closeEditor(widget);
    });
    auto* le = widget->GetLineEdit();
    WidgetWrapper(le).AddEventFilter([this, widget, le](QObject*, QEvent* e){
        if(e->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(e);
            auto* nonConst = const_cast<DelegatesDateTimePicker*>(this);
            switch(ke->key()) {
            case Qt::Key_Backtab:
                if(ke->modifiers() & Qt::ShiftModifier) {
                    if(le->currentSectionIndex() == 0) {
                        emit nonConst->commitData(widget);
                        emit nonConst->closeEditor(widget, QAbstractItemDelegate::EditPreviousItem);
                        return true;
                    }
                }
            case Qt::Key_Tab: {
                if(le->currentSectionIndex() == (le->sectionCount() - 1)) {
                    emit nonConst->commitData(widget);
                    emit nonConst->closeEditor(widget, QAbstractItemDelegate::EditNextItem);
                    return true;
                }
                break;
            }
            default: break;
            }
        }
        return false;
    });
    return widget;
}

void DelegatesDateTimePicker::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
    editor->setGeometry(option.rect);
}

void DelegatesDateTimePicker::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* widget = dynamic_cast<WidgetsDatetimePopupPicker*>(editor);
    Q_ASSERT(widget != nullptr);
    model->setData(index, widget->GetDateTimeWidget()->CurrentDateTime.Native(), Qt::EditRole);
}

QWidget* DelegatesTimePicker::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto* ac = new QWidgetAction(parent);
    auto* menu = MenuWrapper::CreatePreventedFromClosingMenu(tr("Time"));

    m_editor = new WidgetsTimeWidget(parent);
    m_editor->CurrentTime.SetMinMax(QTime::fromMSecsSinceStartOfDay(0), QTime::fromMSecsSinceStartOfDay(24*3600*1000-1));
    m_editor->CurrentTime = index.model()->data(index, Qt::EditRole).toTime();
    OnEditorAboutToBeShown(m_editor, index);

    ac->setDefaultWidget(m_editor);
    menu->addAction(ac);
    return menu;
}

void DelegatesTimePicker::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
    auto* parentWidget = option.widget;
    QDesktopWidget *desktop = QApplication::desktop();

    auto* table = qobject_cast<QTableView*>(const_cast<QWidget*>(option.widget));
    if(table != nullptr) {
        parentWidget = table->viewport();
    }
    auto screenSize = desktop->screenGeometry(parentWidget);
    auto mappedPos = parentWidget->mapToGlobal(option.rect.bottomLeft());
    auto editorHeight = editor->sizeHint().height();
    if((editorHeight + mappedPos.y()) > screenSize.height()) {
        mappedPos -= QPoint(0, editorHeight + option.rect.height());
    }
    editor->move(mappedPos);
}

void DelegatesTimePicker::setModelData(QWidget*, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto dt = model->data(index, Qt::EditRole);
    dt = m_editor->CurrentTime.Native();
    OnAboutToSetData(m_editor->CurrentTime, dt, index);
    model->setData(index, dt);
}

void DateTimeRangeAttachment::Attach(DelegatesDateTimePicker* delegate, const QPair<int,int>& columns, LocalPropertyDateTime* start, LocalPropertyDateTime* stop)
{
    delegate->OnEditorAboutToBeShown.Connect(CONNECTION_DEBUG_LOCATION, [columns, start, stop](QWidget* w, const QModelIndex& index){
        auto* picker = reinterpret_cast<WidgetsDatetimePopupPicker*>(w);
        auto& currentDateTime = picker->GetDateTimeWidget()->CurrentDateTime;
        currentDateTime = index.data(Qt::EditRole).toDateTime();

        if(currentDateTime.IsRealTime()) {
            if(index.column() == columns.second) {
                auto startDateTime = index.model()->index(index.row(), columns.first).data(Qt::EditRole).toDateTime();
                currentDateTime = startDateTime.isValid() ? startDateTime : QDateTime::currentDateTime();
            } else {
                auto startDateTime = index.model()->index(index.row() - 1, columns.second).data(Qt::EditRole).toDateTime();
                currentDateTime = startDateTime.isValid() ? startDateTime : QDateTime::currentDateTime();
            }
        }
        currentDateTime.SetMinMax(*start, *stop);
    });
}
