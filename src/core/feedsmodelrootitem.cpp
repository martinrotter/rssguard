#include <QVariant>

#include "core/feedsmodelrootitem.h"


FeedsModelRootItem::FeedsModelRootItem() {
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDebug("Destroying FeedsModelRootItem instance.");
  qDeleteAll(m_childItems);
}

FeedsModelRootItem *FeedsModelRootItem::parent() {
  return NULL;
}

FeedsModelRootItem *FeedsModelRootItem::child(int row) {
  return m_childItems.at(0);
}

int FeedsModelRootItem::columnCount() const {
  return 2;
}

int FeedsModelRootItem::row() const {
  return 0;
}

int FeedsModelRootItem::childCount() const {
  return m_childItems.count();
}

QVariant FeedsModelRootItem::data(int column, int role) const {
  if (role == Qt::DisplayRole) {
    return "aaa";
  }

  return QVariant();
}
