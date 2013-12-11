#include "core/feedsmodelfeed.h"


FeedsModelFeed::FeedsModelFeed(FeedsModelItem *parent_item)
  :FeedsModelNonRootItem(parent_item) {
}

FeedsModelFeed::~FeedsModelFeed() {
  qDebug("Destroying FeedsModelFeed instance.");
}

int FeedsModelFeed::childCount() const {
  // Because feed has no children.
  return 0;
}
