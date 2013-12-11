#include "core/feedsmodelcategory.h"


FeedsModelCategory::FeedsModelCategory(FeedsModelItem *parent_item)
  : FeedsModelNonRootItem(parent_item) {
}

FeedsModelCategory::~FeedsModelCategory() {
  qDebug("Destroying BaseFeedsModelCategory instance.");
}
