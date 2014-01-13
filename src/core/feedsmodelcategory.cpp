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
