#include "core/feedsmodelrootitem.h"


FeedsModelRootItem::FeedsModelRootItem()
  : FeedsModelItem() {
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDebug("Destroying FeedsModelRootItem instance.");
  qDeleteAll(m_childItems);
}

FeedsModelItem *FeedsModelRootItem::parent() {
  return NULL;
}

FeedsModelItem *FeedsModelRootItem::child(int row) {
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
