#include "core/feedsmodelfeed.h"


FeedsModelFeed::FeedsModelFeed(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item) {
}

FeedsModelFeed::~FeedsModelFeed() {
  qDebug("Destroying FeedsModelFeed instance.");
}

int FeedsModelFeed::childCount() const {
  // Because feed has no children.
  return 0;
}
