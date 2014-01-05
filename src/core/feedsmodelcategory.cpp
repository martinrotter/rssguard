#include "core/feedsmodelcategory.h"

#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodelstandardfeed.h"


FeedsModelCategory::FeedsModelCategory(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item) {
  m_kind = FeedsModelRootItem::Category;
}

FeedsModelCategory::FeedsModelCategory(const FeedsModelCategory &other)
  : FeedsModelRootItem(NULL) {
  m_kind = other.kind();
  m_title = other.title();
  m_id = other.id();
  m_icon = other.icon();
  m_childItems = other.childItems();
  m_parentItem = other.parent();
  m_type = other.type();
  m_creationDate = other.creationDate();
  m_description = other.description();
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
