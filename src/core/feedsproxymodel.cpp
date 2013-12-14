#include "core/feedsproxymodel.h"
#include "core/feedsmodel.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodelrootitem.h"


FeedsProxyModel::FeedsProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent) {
  m_sourceModel = new FeedsModel(this);



  setObjectName("FeedsProxyModel");
  setSortRole(Qt::EditRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setFilterKeyColumn(-1);
  setFilterRole(Qt::EditRole);
  setDynamicSortFilter(true);
  setSourceModel(m_sourceModel);
}

FeedsProxyModel::~FeedsProxyModel() {
  qDebug("Destroying FeedsProxyModel instance");
}

FeedsModel *FeedsProxyModel::sourceModel() {
  return m_sourceModel;
}

bool FeedsProxyModel::lessThan(const QModelIndex &left,
                               const QModelIndex &right) const {
  if (left.isValid() && right.isValid()) {
    FeedsModelRootItem *left_item = static_cast<FeedsModelRootItem*>(left.internalPointer());
    FeedsModelRootItem *right_item = static_cast<FeedsModelRootItem*>(right.internalPointer());

    FeedsModelFeed *left_feed = dynamic_cast<FeedsModelFeed*>(left_item);
    FeedsModelFeed *right_feed = dynamic_cast<FeedsModelFeed*>(right_item);

    FeedsModelCategory *left_category = dynamic_cast<FeedsModelCategory*>(left_item);
    FeedsModelCategory *right_category = dynamic_cast<FeedsModelCategory*>(right_item);

    if (left_feed != NULL && right_feed != NULL) {
      // Both items are feeds.
      return left_feed->title() < right_feed->title();
    }
    else if (left_category != NULL && right_category != NULL) {
      // Both items are categories.
      return left_category->title() < right_category->title();
    }
    else if (left_feed != NULL) {
      // Left item is feed, right item is category.
      return false;
    }
    else {
      // Left item is category, right item is feed.
      return true;
    }


    return true;
  }
  else {
    return false;
  }
}
