#include <QVariant>

#include "feedsmodelstandardfeed.h"


FeedsModelStandardFeed::FeedsModelStandardFeed(FeedsModelRootItem *parent_item)
  : FeedsModelFeed(parent_item) {
}

FeedsModelStandardFeed::~FeedsModelStandardFeed() {
  qDebug("Destroying FeedsModelStandardFeed instance.");
}

QVariant FeedsModelStandardFeed::data(int column, int role) const {
  if (role == Qt::DisplayRole) {
    return "bbb";
  }
  else {
    return QVariant();
  }
}
