// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "core/feedsmodel.h"
#include "core/rootitem.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"

#include <QTimer>


FeedsProxyModel::FeedsProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent), m_selectedItem(NULL), m_showUnreadOnly(false) {
  m_sourceModel = new FeedsModel(this);

  setObjectName(QSL("FeedsProxyModel"));
  setSortRole(Qt::EditRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setFilterKeyColumn(-1);
  setFilterRole(Qt::EditRole);
  setDynamicSortFilter(false);
  setSourceModel(m_sourceModel);

  connect(m_sourceModel, SIGNAL(readFeedsFilterInvalidationRequested()), this, SLOT(invalidateReadFeedsFilter()));
}

FeedsProxyModel::~FeedsProxyModel() {
  qDebug("Destroying FeedsProxyModel instance");
}

QModelIndexList FeedsProxyModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const {
  QModelIndexList result;
  uint match_type = flags & 0x0F;
  Qt::CaseSensitivity cs = Qt::CaseInsensitive;
  bool recurse = flags & Qt::MatchRecursive;
  bool wrap = flags & Qt::MatchWrap;
  bool all_hits = (hits == -1);
  QString entered_text;
  QModelIndex p = parent(start);
  int from = start.row();
  int to = rowCount(p);

  for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); ++i) {
    for (int r = from; (r < to) && (all_hits || result.count() < hits); ++r) {
      QModelIndex idx = index(r, start.column(), p);

      if (!idx.isValid()) {
        continue;
      }

      QModelIndex mapped_idx = mapToSource(idx);
      QVariant item_value = m_sourceModel->itemForIndex(mapped_idx)->title();

      // QVariant based matching.
      if (match_type == Qt::MatchExactly) {
        if (value == item_value) {
          result.append(idx);
        }
      }
      // QString based matching.
      else {
        if (entered_text.isEmpty()) {
          entered_text = value.toString();
        }

        QString item_text = item_value.toString();

        switch (match_type) {
          case Qt::MatchRegExp:
            if (QRegExp(entered_text, cs).exactMatch(item_text)) {
              result.append(idx);
            }
            break;

          case Qt::MatchWildcard:
            if (QRegExp(entered_text, cs, QRegExp::Wildcard).exactMatch(item_text)) {
              result.append(idx);
            }
            break;

          case Qt::MatchStartsWith:
            if (item_text.startsWith(entered_text, cs)) {
              result.append(idx);
            }
            break;

          case Qt::MatchEndsWith:
            if (item_text.endsWith(entered_text, cs)) {
              result.append(idx);
            }
            break;

          case Qt::MatchFixedString:
            if (item_text.compare(entered_text, cs) == 0) {
              result.append(idx);
            }
            break;

          case Qt::MatchContains:
          default:
            if (item_text.contains(entered_text, cs)) {
              result.append(idx);
            }
            break;
        }
      }

      if (recurse && hasChildren(idx)) {
        result += match(index(0, idx.column(), idx), role, (entered_text.isEmpty() ? value : entered_text), (all_hits ? -1 : hits - result.count()), flags);
      }
    }

    from = 0;
    to = start.row();
  }

  return result;
}

bool FeedsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  if (left.isValid() && right.isValid()) {
    // Make necessary castings.
    RootItem *left_item = m_sourceModel->itemForIndex(left);
    RootItem *right_item = m_sourceModel->itemForIndex(right);

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
        return QString::localeAwareCompare(left_item->title(), right_item->title()) < 0;
      }
    }
    else if (left_item->kind() == RootItemKind::Bin) {
      // Left item is recycle bin. Make sure it is "biggest" item if we have selected ascending order.
      return sortOrder() == Qt::DescendingOrder;
    }
    else if (right_item->kind() == RootItemKind::Bin) {
      // Right item is recycle bin. Make sure it is "smallest" item if we have selected descending order.
      return sortOrder() == Qt::AscendingOrder;
    }
    else if (left_item->kind() == RootItemKind::Feed) {
      // Left item is feed, right item is category.
      return false;
    }
    else {
      // Left item is category, right item is feed.
      // NOTE: Category is in fact "more" than feed but we consider it to be "less" because it should be "placed"
      // above the "smalles" feed when ascending sort is used.
      // NOTE: We need to keep recycle bin in first position.
      return true;
    }
  }
  else {
    return false;
  }
}

bool FeedsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  if (!m_showUnreadOnly) {
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
  }

  QModelIndex idx = m_sourceModel->index(source_row, 0, source_parent);

  if (!idx.isValid()) {
    return false;
  }

  RootItem *item = m_sourceModel->itemForIndex(idx);

  if (item->kind() == RootItemKind::Bin || item->kind() == RootItemKind::ServiceRoot) {
    // Recycle bin is always displayed.
    return true;
  }
  else if (item->isParentOf(m_selectedItem)/* || item->isChildOf(m_selectedItem)*/ || m_selectedItem == item) {
    // Currently selected item and all its parents and children must be displayed.
    return true;
  }
  else {
    // NOTE: If item has < 0 of unread message it may mean, that the count
    // of unread messages is not (yet) known, display that item too.
    return item->countOfUnreadMessages() != 0;
  }
}

RootItem *FeedsProxyModel::selectedItem() const {
  return m_selectedItem;
}

void FeedsProxyModel::setSelectedItem(RootItem *selected_item) {
  m_selectedItem = selected_item;
}

bool FeedsProxyModel::showUnreadOnly() const {
  return m_showUnreadOnly;
}

void FeedsProxyModel::invalidateReadFeedsFilter(bool set_new_value, bool show_unread_only) {
  if (set_new_value) {
    setShowUnreadOnly(show_unread_only);
  }

  QTimer::singleShot(0, this, SLOT(invalidateFilter()));
}

void FeedsProxyModel::setShowUnreadOnly(bool show_unread_only) {
  m_showUnreadOnly = show_unread_only;
  qApp->settings()->setValue(GROUP(Feeds), Feeds::ShowOnlyUnreadFeeds, show_unread_only);
}

QModelIndexList FeedsProxyModel::mapListToSource(const QModelIndexList &indexes) {
  QModelIndexList source_indexes;

  foreach (const QModelIndex &index, indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}

void FeedsProxyModel::invalidateFilter() {
  QSortFilterProxyModel::invalidateFilter();
}
