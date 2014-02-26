// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

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



bool FeedsProxyModel::lessThan(const QModelIndex &left,
                               const QModelIndex &right) const {
  if (left.isValid() && right.isValid()) {
    // Make necessary castings.
    FeedsModelRootItem *left_item = m_sourceModel->itemForIndex(left);
    FeedsModelRootItem *right_item = m_sourceModel->itemForIndex(right);

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
        return QString::localeAwareCompare(left_item->title(),
                                           right_item->title()) < 0;
      }
    }
    else if (left_item->kind() == FeedsModelRootItem::Feed) {
      // Left item is feed, right item is category.
      return false;
    }
    else {
      // Left item is category, right item is feed.
      // NOTE: Category is in fact "more" than feed but
      // we consider it to be "less" because it should be "placed"
      // above the "smalles" feed when ascending sort is used.
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
