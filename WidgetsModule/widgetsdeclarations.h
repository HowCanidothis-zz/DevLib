#ifndef WIDGETSDECLARATIONS_H
#define WIDGETSDECLARATIONS_H

#include <QHash>

struct DescColumnsParams
{
    struct ColumnParam
    {
        bool Visible = true;
        bool CanBeHidden = false;

        ColumnParam& SetVisible(bool visible) { Visible = visible; return *this; }
        ColumnParam& SetCanBeHidden(bool canBeHidden) { CanBeHidden = canBeHidden; return *this; }
        ColumnParam& HideColumn() { Visible = false; CanBeHidden = true; return *this;  }
        ColumnParam& RemoveColumn() { Visible = false; CanBeHidden = false; return *this;  }
    };

    DescColumnsParams(const QSet<qint32>& ignoreColumns);
    DescColumnsParams() {}

    DescColumnsParams& SetColumnParam(qint32 column, const ColumnParam& param) { ColumnsParams.insert(column, param); return *this; }
    DescColumnsParams& SetStateTag(const Latin1Name& stateTag) { StateTag = stateTag; return *this; }

    QHash<qint32, ColumnParam> ColumnsParams;
    Latin1Name StateTag;
};

inline DescColumnsParams::DescColumnsParams(const QSet<qint32>& ignoreColumns)
{
    for(const auto& column : ignoreColumns) {
        ColumnsParams.insert(column, ColumnParam());
    }
}

#endif // WIDGETSDECLARATIONS_H
