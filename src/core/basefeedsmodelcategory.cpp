#include "core/basefeedsmodelcategory.h"


BaseFeedsModelCategory::BaseFeedsModelCategory(BaseFeedsModelItem *parent_item)
  : FeedsModelNonRootItem(parent_item) {
}

BaseFeedsModelCategory::~BaseFeedsModelCategory() {
  qDebug("Destroying BaseFeedsModelCategory instance.");
}
