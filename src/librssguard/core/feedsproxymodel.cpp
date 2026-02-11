// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsproxymodel.h"

#include "core/feedsmodel.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "gui/feedsview.h"
#include "miscellaneous/application.h"
#include "miscellaneous/regexfactory.h"
#include "miscellaneous/settings.h"
#include "services/abstract/rootitem.h"

#include <QMimeData>
#include <QTimer>

using RootItemPtr = RootItem*;

FeedsProxyModel::FeedsProxyModel(FeedsModel* source_model, QObject* parent)
  : QSortFilterProxyModel(parent), m_sourceModel(source_model), m_selectedItem(nullptr), m_sortAlphabetically(false),
    m_filter(FeedListFilter::NoFiltering) {
  setObjectName(QSL("FeedsProxyModel"));

  initializeFilters();

  setSortRole(Qt::ItemDataRole::EditRole);
  setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
  setRecursiveFilteringEnabled(true);
  setFilterKeyColumn(FDS_MODEL_TITLE_INDEX);
  setFilterRole(Qt::ItemDataRole::EditRole);
  setDynamicSortFilter(true);
  setSourceModel(m_sourceModel);

  // Describes priorities of node types for sorting.
  // Smaller index means that item is "smaller" which
  // means it should be more on top when sorting
  // in ascending order.
  m_priorities = {RootItem::Kind::Category,
                  RootItem::Kind::Feed,
                  RootItem::Kind::Labels,
                  RootItem::Kind::Probes,
                  RootItem::Kind::Important,
                  RootItem::Kind::Unread,
                  RootItem::Kind::Bin};
}

FeedsProxyModel::~FeedsProxyModel() {
  qDebugNN << LOGSEC_FEEDMODEL << "Destroying FeedsProxyModel instance";
}

QModelIndexList FeedsProxyModel::match(const QModelIndex& start,
                                       int role,
                                       const QVariant& value,
                                       int hits,
                                       Qt::MatchFlags flags) const {
  QModelIndexList result;
  const int match_type = flags & 0x0F;
  const Qt::CaseSensitivity cs = Qt::CaseSensitivity::CaseInsensitive;
  const bool recurse = Globals::hasFlag(flags, Qt::MatchFlag::MatchRecursive);
  const bool wrap = Globals::hasFlag(flags, Qt::MatchFlag::MatchWrap);
  const bool all_hits = (hits == -1);
  QString entered_text;
  const QModelIndex p = parent(start);
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
      if (match_type == Qt::MatchFlag::MatchExactly) {
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
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
          case Qt::MatchFlag::MatchRegularExpression:
#else
          case Qt::MatchFlag::MatchRegExp:
#endif
            if (QRegularExpression(entered_text,
                                   QRegularExpression::PatternOption::CaseInsensitiveOption |
                                     QRegularExpression::PatternOption::UseUnicodePropertiesOption)
                  .match(item_text)
                  .hasMatch()) {
              result.append(idx);
            }

            break;

          case Qt::MatchFlag::MatchWildcard:
            if (QRegularExpression(RegexFactory::wildcardToRegularExpression(entered_text),
                                   QRegularExpression::PatternOption::CaseInsensitiveOption |
                                     QRegularExpression::PatternOption::UseUnicodePropertiesOption)
                  .match(item_text)
                  .hasMatch()) {
              result.append(idx);
            }

            break;

          case Qt::MatchFlag::MatchStartsWith:
            if (item_text.startsWith(entered_text, cs)) {
              result.append(idx);
            }

            break;

          case Qt::MatchFlag::MatchEndsWith:
            if (item_text.endsWith(entered_text, cs)) {
              result.append(idx);
            }

            break;

          case Qt::MatchFlag::MatchFixedString:
            if (item_text.compare(entered_text, cs) == 0) {
              result.append(idx);
            }

            break;

          case Qt::MatchFlag::MatchContains:
          default:
            if (item_text.contains(entered_text, cs)) {
              result.append(idx);
            }

            break;
        }
      }

      if (recurse && hasChildren(idx)) {
        result += match(index(0, idx.column(), idx),
                        role,
                        (entered_text.isEmpty() ? value : entered_text),
                        (all_hits ? -1 : hits - result.count()),
                        flags);
      }
    }

    from = 0;
    to = start.row();
  }

  return result;
}

bool FeedsProxyModel::canDropMimeData(const QMimeData* data,
                                      Qt::DropAction action,
                                      int row,
                                      int column,
                                      const QModelIndex& parent) const {
  if (action != Qt::DropAction::MoveAction) {
    return false;
  }

  QByteArray dragged_items_data = data->data(QSL(MIME_TYPE_ITEM_POINTER));
  QDataStream stream(&dragged_items_data, QIODevice::OpenModeFlag::ReadOnly);
  const bool order_change = row >= 0 && !m_sortAlphabetically;
  const QModelIndex target_parent = mapToSource(parent);

  if (stream.atEnd()) {
    return false;
  }

  quintptr pointer_to_item;
  stream >> pointer_to_item;

  RootItem* dragged_item = RootItemPtr(pointer_to_item);

  // Dragged item must service root, feed or category.
  //
  // If row is less than zero, it means we are moving dragged item into new parent. If row is at least
  // zero, then we are sorting the dragged item.
  //
  // Otherwise the target row identifies the item just below the drop target placement insertion line.
  QModelIndex target_idx = order_change ? mapToSource(index(row, 0, parent)) : target_parent;
  RootItem* target_item = m_sourceModel->itemForIndex(target_idx);
  RootItem* target_parent_item = m_sourceModel->itemForIndex(target_parent);

  if (target_item != nullptr) {
    qDebugNN << LOGSEC_FEEDMODEL << "Considering target for drop operation:" << QUOTE_W_SPACE(target_item->title())
             << "with index" << QUOTE_W_SPACE(target_idx)
             << "and target parent:" << QUOTE_W_SPACE_DOT(target_parent_item->title());

    switch (dragged_item->kind()) {
      case RootItem::Kind::Feed:
      case RootItem::Kind::Category:
        // Feeds can be reordered or inserted under service root or category.
        // Categories can be reordered or inserted under service root or another category.
        return target_parent_item->kind() == RootItem::Kind::Category ||
               target_parent_item->kind() == RootItem::Kind::ServiceRoot;

      case RootItem::Kind::ServiceRoot:
        // Service root cannot be inserted under different parent, can only be reordered.
        if (!order_change) {
          return false;
        }
        else {
          return target_parent_item->kind() == RootItem::Kind::Root;
        }

      default:
        break;
    }
  }

  return false;
}

bool FeedsProxyModel::dropMimeData(const QMimeData* data,
                                   Qt::DropAction action,
                                   int row,
                                   int column,
                                   const QModelIndex& parent) {
  Q_UNUSED(column)

  if (action == Qt::DropAction::IgnoreAction) {
    return true;
  }
  else if (action != Qt::DropAction::MoveAction) {
    return false;
  }

  QByteArray dragged_items_data = data->data(QSL(MIME_TYPE_ITEM_POINTER));

  if (dragged_items_data.isEmpty()) {
    return false;
  }
  else {
    QDataStream stream(&dragged_items_data, QIODevice::OpenModeFlag::ReadOnly);
    const bool order_change = row >= 0 && !m_sortAlphabetically;
    const QModelIndex source_parent = mapToSource(parent);

    while (!stream.atEnd()) {
      quintptr pointer_to_item;
      stream >> pointer_to_item;

      // We have item we want to drag, we also determine the target item.
      RootItemPtr dragged_item = RootItemPtr(pointer_to_item);
      RootItem* target_item = m_sourceModel->itemForIndex(source_parent);
      ServiceRoot* dragged_item_root = dragged_item->account();
      ServiceRoot* target_item_root = target_item->account();

      if ((dragged_item == target_item || dragged_item->parent() == target_item) && !order_change) {
        qDebugNN << LOGSEC_FEEDMODEL
                 << "Dragged item is equal to target item or its parent is equal to target item. Cancelling drag-drop "
                    "action.";
        return false;
      }

      if (dragged_item_root != target_item_root) {
        // Transferring of items between different accounts is not possible.
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Cannot perform drag & drop operation"),
                              tr("You can't transfer dragged item into different account, this is not supported."),
                              QSystemTrayIcon::MessageIcon::Critical});
        qDebugNN << LOGSEC_FEEDMODEL
                 << "Dragged item cannot be dragged into different account. Cancelling drag-drop action.";
        return false;
      }

      if (dragged_item != target_item && dragged_item->parent() != target_item &&
          dragged_item->performDragDropChange(target_item)) {
        // Drag & drop is supported by the dragged item and was
        // completed on data level and in item hierarchy.
        emit requireItemValidationAfterDragDrop(m_sourceModel->indexForItem(dragged_item));

        qDebugNN << LOGSEC_FEEDMODEL << "Dropping item" << QUOTE_W_SPACE(dragged_item->title()) << "under new parent"
                 << QUOTE_W_SPACE_DOT(target_item->title());
      }

      if (order_change) {
        RootItem* place_above_item = m_sourceModel->itemForIndex(mapToSource(index(row, 0, parent)));
        int target_sort_order = place_above_item->sortOrder();

        qDebugNN << LOGSEC_FEEDMODEL << "Resorting/placing item" << QUOTE_W_SPACE(dragged_item->title())
                 << "with sord order" << QUOTE_W_SPACE(dragged_item->sortOrder()) << "above item"
                 << QUOTE_W_SPACE(place_above_item->title()) << "with new sort order"
                 << QUOTE_W_SPACE_DOT(target_sort_order);

        if (target_sort_order > dragged_item->sortOrder()) {
          target_sort_order--;
        }

        qApp->database()->worker()->write([&](const QSqlDatabase& db) {
          DatabaseQueries::moveItem(dragged_item, false, false, target_sort_order, db);
        });
      }

      invalidate();
    }

    return true;
  }

  return false;
}

bool FeedsProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  if (left.isValid() && right.isValid()) {
    // Make necessary castings.
    const RootItem* left_item = m_sourceModel->itemForIndex(left);
    const RootItem* right_item = m_sourceModel->itemForIndex(right);

    // NOTE: Here we want to accomplish that ALL
    // categories are queued one after another and all
    // feeds are queued one after another too.
    // Moreover, sort everything alphabetically or
    // by item counts, depending on the sort column.
    if (left_item->keepOnTop()) {
      return sortOrder() == Qt::SortOrder::AscendingOrder;
    }
    else if (right_item->keepOnTop()) {
      return sortOrder() == Qt::SortOrder::DescendingOrder;
    }
    else if (left_item->kind() == right_item->kind()) {
      if (left.column() == FDS_MODEL_COUNTS_INDEX) {
        // We sort according to counts, no matter of alphabetical or manual sorting is enabled.
        return left_item->countOfUnreadMessages() < right_item->countOfUnreadMessages();
      }

      if (m_sortAlphabetically) {
        return QString::localeAwareCompare(left_item->title().toLower(), right_item->title().toLower()) < 0;
      }
      else {
        switch (left_item->kind()) {
          case RootItem::Kind::Feed:
          case RootItem::Kind::Category:
          case RootItem::Kind::ServiceRoot:
            return left_item->sortOrder() < right_item->sortOrder();

          default:
            return QString::localeAwareCompare(left_item->title().toLower(), right_item->title().toLower()) < 0;
        }
      }
    }
    else {
      // We sort using priorities.
      auto left_priority = m_priorities.indexOf(left_item->kind());
      auto right_priority = m_priorities.indexOf(right_item->kind());

      return sortOrder() == Qt::SortOrder::AscendingOrder ? left_priority < right_priority
                                                          : right_priority < left_priority;
    }
  }
  else {
    return false;
  }
}

bool FeedsProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  bool should_show = filterAcceptsRowInternal(source_row, source_parent);

  if (should_show && m_hiddenIndices.contains(QPair<int, QModelIndex>(source_row, source_parent))) {
    qDebugNN << LOGSEC_CORE << "Item"
             << QUOTE_W_SPACE(m_sourceModel
                                ->data(m_sourceModel->index(source_row, 0, source_parent), Qt::ItemDataRole::EditRole)
                                .toString())
             << "was previously hidden and now shows up, expand.";

    const_cast<FeedsProxyModel*>(this)->m_hiddenIndices.removeAll(QPair<int, QModelIndex>(source_row, source_parent));

    // Now, item now should be displayed and previously it was not.
    // Expand!
    emit indexNotFilteredOutAnymore(m_sourceModel->index(source_row, 0, source_parent));
  }

  if (!should_show) {
    const_cast<FeedsProxyModel*>(this)->m_hiddenIndices.append(QPair<int, QModelIndex>(source_row, source_parent));
  }

  return should_show;
}

void FeedsProxyModel::initializeFilters() {
  m_filters[FeedListFilter::ShowEmpty] = [](const RootItem* item) {
    return item->countOfAllMessages() == 0;
  };

  m_filters[FeedListFilter::ShowNonEmpty] = [](const RootItem* item) {
    return item->countOfAllMessages() != 0;
  };

  m_filters[FeedListFilter::ShowQuiet] = [](const RootItem* item) {
    Feed* feed = item->toFeed();

    return feed != nullptr && feed->isQuiet();
  };

  m_filters[FeedListFilter::ShowSwitchedOff] = [](const RootItem* item) {
    Feed* feed = item->toFeed();

    return feed != nullptr && feed->isSwitchedOff();
  };

  m_filters[FeedListFilter::ShowUnread] = [](const RootItem* item) {
    return item->countOfUnreadMessages() > 0;
  };

  m_filters[FeedListFilter::ShowWithArticleFilters] = [](const RootItem* item) {
    Feed* feed = item->toFeed();

    return feed != nullptr && !feed->messageFilters().isEmpty();
  };

  m_filters[FeedListFilter::ShowWithError] = [](const RootItem* item) {
    Feed* feed = item->toFeed();

    return feed != nullptr && Feed::isErrorStatus(feed->status());
  };

  m_filters[FeedListFilter::ShowWithNewArticles] = [](const RootItem* item) {
    Feed* feed = item->toFeed();

    return feed != nullptr && feed->status() == Feed::Status::NewMessages;
  };

  m_filterKeys = m_filters.keys();
}

bool FeedsProxyModel::filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const {
  const QModelIndex idx = m_sourceModel->index(source_row, 0, source_parent);

  if (!idx.isValid()) {
    return false;
  }

  const RootItem* item = m_sourceModel->itemForIndex(idx);

  if (m_selectedItem != nullptr && m_selectedItem == item) {
    return true;
  }

  ServiceRoot* par_account = item->account();

  if (item->kind() == RootItem::Kind::Important && !par_account->nodeShowImportant()) {
    return false;
  }

  if (item->kind() == RootItem::Kind::Unread && !par_account->nodeShowUnread()) {
    return false;
  }

  if (item->kind() == RootItem::Kind::Probes && !par_account->nodeShowProbes()) {
    return false;
  }

  if (item->kind() == RootItem::Kind::Labels && !par_account->nodeShowLabels()) {
    return false;
  }

  if (item->kind() != RootItem::Kind::Category && item->kind() != RootItem::Kind::Feed &&
      item->kind() != RootItem::Kind::Label) {
    // Some items are always visible.
    return true;
  }

  bool should_show = QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

  for (FeedListFilter val : m_filterKeys) {
    if (Globals::hasFlag(m_filter, val)) {
      // This particular filter is enabled.
      if (m_filters[val](item)) {
        // The item matches the feed filter.
        // Display it if it matches internal string-based filter too.
        return should_show;
      }
    }
  }

  if (m_filter != FeedListFilter::NoFiltering) {
    // Some filter is enabled but this item does not meet it.
    return false;
  }

  return should_show;
}

bool FeedsProxyModel::sortAlphabetically() const {
  return m_sortAlphabetically;
}

void FeedsProxyModel::sort(int column, Qt::SortOrder order) {
  QSortFilterProxyModel::sort(column, order);
}

const RootItem* FeedsProxyModel::selectedItem() const {
  return m_selectedItem;
}

void FeedsProxyModel::setSelectedItem(const RootItem* selected_item) {
  auto* previous_selected_item = m_selectedItem;
  m_selectedItem = selected_item;

  if (previous_selected_item != nullptr && !previous_selected_item->isAboutToBeDeleted()) {
    auto source_idx = m_sourceModel->indexForItem(previous_selected_item);

    if (source_idx.isValid()) {
      m_sourceModel->reloadChangedLayout({source_idx});
    }
  }
}

void FeedsProxyModel::setSortAlphabetically(bool sort_alphabetically) {
  if (sort_alphabetically != m_sortAlphabetically) {
    m_sortAlphabetically = sort_alphabetically;
    qApp->settings()->setValue(GROUP(Feeds), Feeds::SortAlphabetically, sort_alphabetically);
    invalidate();
  }
}

QModelIndexList FeedsProxyModel::mapListToSource(const QModelIndexList& indexes) const {
  QModelIndexList source_indexes;
  source_indexes.reserve(indexes.size());

  for (const QModelIndex& index : indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}

void FeedsProxyModel::setFeedListFilter(FeedListFilter filter) {
#if QT_VERSION_MAJOR == 5
  m_filter = filter;
  invalidateFilter();
#elif QT_VERSION >= 0x060A00 // Qt > 6.9.0
  beginFilterChange();
  m_filter = filter;
  endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
  m_filter = filter;
  invalidateRowsFilter();
#endif
}
