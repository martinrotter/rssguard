#include <QVariant>

#include "core/feedsmodelrootitem.h"


FeedsModelRootItem::FeedsModelRootItem(FeedsModelRootItem *parent_item)
  : m_parentItem(parent_item) {
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDebug("Destroying FeedsModelRootItem instance.");
  qDeleteAll(m_childItems);
}

FeedsModelRootItem *FeedsModelRootItem::parent() {
  return m_parentItem;
}

void FeedsModelRootItem::setParent(FeedsModelRootItem *parent_item) {
  m_parentItem = parent_item;
}

FeedsModelRootItem *FeedsModelRootItem::child(int row) {
  return m_childItems.value(row);
}

void FeedsModelRootItem::appendChild(FeedsModelRootItem *child) {
  m_childItems.append(child);
  child->setParent(this);
}

int FeedsModelRootItem::columnCount() const {
  return 2;
}

int FeedsModelRootItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(const_cast<FeedsModelRootItem*>(this));
  }
  else {
    return 0;
  }
}

int FeedsModelRootItem::childCount() const {
  return m_childItems.count();
}

QVariant FeedsModelRootItem::data(int column, int role) const {
  Q_UNUSED(column)
  Q_UNUSED(role)

  return QVariant();
}
