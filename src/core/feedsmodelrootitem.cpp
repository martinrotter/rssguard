#include "core/feedsmodelrootitem.h"

#include "qtsingleapplication/qtsingleapplication.h"

#include <QVariant>


FeedsModelRootItem::FeedsModelRootItem(FeedsModelRootItem *parent_item)
  : m_kind(FeedsModelRootItem::RootItem),
    m_parentItem(parent_item) {
}

FeedsModelRootItem::~FeedsModelRootItem() {
  qDebug("Destroying FeedsModelRootItem instance.");

  qDeleteAll(m_childItems);
}



int FeedsModelRootItem::row() const {
  if (m_parentItem) {
    return m_parentItem->m_childItems.indexOf(const_cast<FeedsModelRootItem*>(this));
  }
  else {
    // This item has no parent. Therefore, its row index is 0.
    return 0;
  }
}

QVariant FeedsModelRootItem::data(int column, int role) const {
  Q_UNUSED(column)
  Q_UNUSED(role)

  // Do not return anything for the root item.
  return QVariant();
}

int FeedsModelRootItem::countOfAllMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfAllMessages();
  }

  return total_count;
}

int FeedsModelRootItem::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfUnreadMessages();
  }

  return total_count;
}

bool FeedsModelRootItem::removeChild(int index) {
  if (index >= 0 && index < m_childItems.size()) {
    m_childItems.removeAt(index);
    return true;
  }
  else {
    return false;
  }
}

bool FeedsModelRootItem::isEqual(FeedsModelRootItem *lhs,
                                 FeedsModelRootItem *rhs) {
  return (lhs->kind() == rhs->kind()) && (lhs->id() == rhs->id());
}

bool FeedsModelRootItem::lessThan(FeedsModelRootItem *lhs,
                                  FeedsModelRootItem *rhs) {
  if (lhs->kind() == rhs->kind()) {
    return lhs->id() < rhs->id();
  }
  else {
    return false;
  }
}
