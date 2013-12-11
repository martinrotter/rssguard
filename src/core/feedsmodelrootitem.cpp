#include "core/feedsmodelrootitem.h"


FeedsModelRootItem::FeedsModelRootItem()
  : BaseFeedsModelItem() {
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDebug("Destroying FeedsModelRootItem instance.");
  qDeleteAll(m_childItems);
}

BaseFeedsModelItem *FeedsModelRootItem::parent() {
  return NULL;
}

int FeedsModelRootItem::columnCount() const {
  return 2;
}

int FeedsModelRootItem::childCount() const {
  return m_childItems.count();
}
