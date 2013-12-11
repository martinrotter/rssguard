#include "core/feedsmodelnonrootitem.h"


FeedsModelNonRootItem::FeedsModelNonRootItem(BaseFeedsModelItem *parent_item)
  : FeedsModelRootItem(), m_parentItem(parent_item) {
}

FeedsModelNonRootItem::~FeedsModelNonRootItem() {
  qDebug("Destroying FeedsModelNonRootItem instance.");
}

BaseFeedsModelItem *FeedsModelNonRootItem::parent() {
  return m_parentItem;
}
