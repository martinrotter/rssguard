#include "core/feedsproxymodel.h"

#include "core/defs.h"
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
  setFilterKeyColumn(0);
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
    // Make necessary castings.
    FeedsModelRootItem *left_item = static_cast<FeedsModelRootItem*>(left.internalPointer());
    FeedsModelRootItem *right_item = static_cast<FeedsModelRootItem*>(right.internalPointer());

    // NOTE: Here we want to accomplish that ALL
    // categories are queued one after another and all
    // feeds are queued one after another too.
    // Moreover, sort everything alphabetically or
    // by item counts, depending on the sort column.

    if (left_item->kind() == right_item->kind()) {
      // Both items are feeds or both items are categories.
      if (left.column() == FDS_MODEL_COUNTS_INDEX) {
        // User wants to sort according to counts.
        return left_item->countOfUnreadMessages() < right_item->countOfUnreadMessages();
      }
      else {
        // In other cases, sort by title.
        return left_item->title() < right_item->title();
      }
    }
    else if (left_item->kind() == FeedsModelRootItem::Feed) {
      // Left item is feed, right item is category.
      return false;
    }
    else {
      // Left item is category, right item is feed.*/
      return true;
    }
  }
  else {
    return false;
  }
}

QModelIndexList FeedsProxyModel::mapListFromSource(const QModelIndexList &indexes) {
  QModelIndexList mapped_indexes;

  foreach (const QModelIndex &index, indexes) {
    mapped_indexes << mapFromSource(index);
  }

  return mapped_indexes;
}

QModelIndexList FeedsProxyModel::mapListToSource(const QModelIndexList &indexes) {
  QModelIndexList source_indexes;

  foreach (const QModelIndex &index, indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}
