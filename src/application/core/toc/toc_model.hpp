#pragma once
#include <QAbstractItemModel>
#include "mupdf/classes.h"
#include "toc_item.hpp"

namespace application::core
{

class TOCModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TOCModel(fz_outline* outline, QObject* parent = nullptr);
    ~TOCModel();

    enum Roles
    {
        TitleRole = Qt::DisplayRole,
        PageNumberRole,
    };

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    int columnCount(const QModelIndex& parent) const override;

private:
    void setupModelData(fz_outline* outline);
    TOCItem* getTOCItemFromOutline(fz_outline* outline);

    TOCItem* m_rootItem;
};

}  // namespace application::core