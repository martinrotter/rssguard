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
