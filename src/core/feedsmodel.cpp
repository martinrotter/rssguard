#include "core/feedsmodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelnonrootitem.h"


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  m_rootItem = new FeedsModelRootItem();

  m_rootItem->m_childItems.append(new FeedsModelNonRootItem(m_rootItem));

}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");
  delete m_rootItem;
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const {
  return QVariant();
}

QVariant FeedsModel::headerData(int section,
                                Qt::Orientation orientation,
                                int role) const {
  if (orientation == Qt::Horizontal) {
    switch (role) {
      case Qt::DisplayRole:
        return "aaa";

      default:
        return QVariant();
    }
  }
  else {
    return QVariant();
  }
}

QModelIndex FeedsModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  FeedsModelItem *parent_item;

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelItem*>(parent.internalPointer());
  }

  FeedsModelItem *child_item = parent_item->child(row);

  if (child_item) {
    return createIndex(row, column, child_item);
  }
  else {
    return QModelIndex();
  }
}

QModelIndex FeedsModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  FeedsModelItem *child_item = static_cast<FeedsModelItem*>(child.internalPointer());
  FeedsModelItem *parent_item = child_item->parent();

  if (parent_item == m_rootItem) {
    return QModelIndex();
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int FeedsModel::rowCount(const QModelIndex &parent) const {
  FeedsModelItem *parent_item;

  if (parent.column() > 0) {
    return 0;
  }

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelItem*>(parent.internalPointer());
  }

  return parent_item->childCount();
}

int FeedsModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return static_cast<FeedsModelItem*>(parent.internalPointer())->columnCount();
  }
  else {
    return m_rootItem->columnCount();
  }
}
