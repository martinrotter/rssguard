#include "core/feedsmodelcategory.h"


FeedsModelCategory::FeedsModelCategory(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item) {
}

FeedsModelCategory::~FeedsModelCategory() {
}

int FeedsModelCategory::countOfAllMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfAllMessages();
  }

  return total_count;
}

int FeedsModelCategory::countOfUnreadMessages() const {
  int total_count = 0;

  foreach (FeedsModelRootItem *child_item, m_childItems) {
    total_count += child_item->countOfUnreadMessages();
  }

  return total_count;
}

FeedsModelCategory:: Type FeedsModelCategory::type() const {
  return m_type;
}

void FeedsModelCategory::setType(const Type &type) {
  m_type = type;
}

QString FeedsModelCategory::title() const {
  return m_title;
}

void FeedsModelCategory::setTitle(const QString &title) {
  m_title = title;
}
