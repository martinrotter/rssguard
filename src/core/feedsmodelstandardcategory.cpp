#include <QVariant>

#include "core/feedsmodelstandardcategory.h"


FeedsModelStandardCategory::FeedsModelStandardCategory(FeedsModelRootItem *parent_item)
  : FeedsModelCategory(parent_item) {
}

FeedsModelStandardCategory::~FeedsModelStandardCategory() {
  qDebug("Destroying FeedsModelStandardCategory instance.");
}

QVariant FeedsModelStandardCategory::data(int column, int role) const {
  if (role == Qt::DisplayRole) {
    return "aaa";
  }
  else {
    return QVariant();
  }
}
