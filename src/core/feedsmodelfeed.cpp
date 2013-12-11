#include "core/feedsmodelfeed.h"


FeedsModelFeed::FeedsModelFeed(BaseFeedsModelItem *parent_item)
  :FeedsModelNonRootItem(parent_item) {
}

FeedsModelFeed::~FeedsModelFeed() {
  qDebug("Destroying FeedsModelFeed instance.");
}
