#include "core/feedsmodel.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  m_rootItem = new FeedsModelRootItem(NULL);

  FeedsModelStandardCategory *cat1 = new FeedsModelStandardCategory(m_rootItem);
  FeedsModelStandardCategory *cat2 = new FeedsModelStandardCategory(cat1);
  FeedsModelStandardFeed *feed1 = new FeedsModelStandardFeed(cat1);
  FeedsModelStandardFeed *feed2 = new FeedsModelStandardFeed(cat1);
  FeedsModelStandardFeed *feed3 = new FeedsModelStandardFeed(m_rootItem);
  FeedsModelStandardFeed *feed4 = new FeedsModelStandardFeed(cat2);
  FeedsModelStandardFeed *feed5 = new FeedsModelStandardFeed(cat2);

  cat1->appendChild(feed1);
  cat1->appendChild(feed2);
  cat1->appendChild(cat2);

  cat2->appendChild(feed4);
  cat2->appendChild(feed5);

  m_rootItem->appendChild(cat1);
  m_rootItem->appendChild(feed3);
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");
  delete m_rootItem;
}

QVariant FeedsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  FeedsModelRootItem *item = static_cast<FeedsModelRootItem*>(index.internalPointer());

  return item->data(index.column(), Qt::DisplayRole);
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

  FeedsModelRootItem *parent_item;

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelRootItem*>(parent.internalPointer());
  }

  FeedsModelRootItem *child_item = parent_item->child(row);

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

  FeedsModelRootItem *child_item = static_cast<FeedsModelRootItem*>(child.internalPointer());
  FeedsModelRootItem *parent_item = child_item->parent();

  if (parent_item == m_rootItem) {
    return QModelIndex();
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int FeedsModel::rowCount(const QModelIndex &parent) const {
  FeedsModelRootItem *parent_item;

  if (parent.column() > 0) {
    return 0;
  }

  if (!parent.isValid()) {
    parent_item = m_rootItem;
  }
  else {
    parent_item = static_cast<FeedsModelRootItem*>(parent.internalPointer());
  }

  return parent_item->childCount();
}

int FeedsModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return static_cast<FeedsModelRootItem*>(parent.internalPointer())->columnCount();
  }
  else {
    return m_rootItem->columnCount();
  }
}
