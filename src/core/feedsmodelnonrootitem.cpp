#include "core/feedsmodelnonrootitem.h"


FeedsModelNonRootItem::FeedsModelNonRootItem(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(), m_parentItem(parent_item) {
}

FeedsModelNonRootItem::~FeedsModelNonRootItem() {
  qDebug("Destroying FeedsModelNonRootItem instance.");
}

FeedsModelRootItem *FeedsModelNonRootItem::parent() {
  return m_parentItem;
}

int FeedsModelNonRootItem::row() const {
  if (m_parentItem) {
    return static_cast<FeedsModelRootItem*>(m_parentItem)->m_childItems.indexOf((FeedsModelRootItem*) this);
  }
  else {
    return 0;
  }
}
