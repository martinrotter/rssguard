// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/feedsproxymodel.h"

#include "core/feedsmodel.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "gui/feedsview.h"
#include "miscellaneous/application.h"
#include "miscellaneous/regexfactory.h"
#include "services/abstract/rootitem.h"

#include <QMimeData>
#include <QTimer>

using RootItemPtr = RootItem*;

FeedsProxyModel::FeedsProxyModel(FeedsModel* source_model, QObject* parent)
  : QSortFilterProxyModel(parent), m_sourceModel(source_model), m_view(nullptr), m_selectedItem(nullptr),
    m_showUnreadOnly(false), m_sortAlphabetically(false) {
  setObjectName(QSL("FeedsProxyModel"));

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

bool FeedsProxyModel::canDropMimeData(const QMimeData* data,
                                      Qt::DropAction action,
                                      int row,
                                      int column,
                                      const QModelIndex& parent) const {

  auto src_idx = row < 0 ? mapToSource(parent) : mapToSource(index(row, column, parent));
  auto* src_item = m_sourceModel->itemForIndex(src_idx);

  if (src_item != nullptr) {
    auto can_drop = src_item->kind() == RootItem::Kind::ServiceRoot || src_item->kind() == RootItem::Kind::Category ||
                    src_item->kind() == RootItem::Kind::Feed;

    return QSortFilterProxyModel::canDropMimeData(data, action, row, column, parent) && can_drop;
  }
  else {
    return false;
  }
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
      auto* dragged_item = RootItemPtr(pointer_to_item);
      RootItem* target_item = m_sourceModel->itemForIndex(source_parent);
      ServiceRoot* dragged_item_root = dragged_item->getParentServiceRoot();
      ServiceRoot* target_item_root = target_item->getParentServiceRoot();

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
      }

      if (order_change) {
        auto db = qApp->database()->driver()->connection(metaObject()->className());

        if (row > dragged_item->sortOrder()) {
          row--;
        }

        DatabaseQueries::moveItem(dragged_item, false, false, row, db);
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
      if (m_sortAlphabetically) {
        // Both items are of the same type.
        if (left.column() == FDS_MODEL_COUNTS_INDEX) {
          // User wants to sort according to counts.
          return left_item->countOfUnreadMessages() < right_item->countOfUnreadMessages();
        }
        else {
          // In other cases, sort by title.
          return QString::localeAwareCompare(left_item->title().toLower(), right_item->title().toLower()) < 0;
        }
      }
      else {
        // We sort some types with sort order, other alphabetically.
        switch (left_item->kind()) {
          case RootItem::Kind::Feed:
          case RootItem::Kind::Category:
          case RootItem::Kind::ServiceRoot:
            return sortOrder() == Qt::SortOrder::AscendingOrder ? left_item->sortOrder() < right_item->sortOrder()
                                                                : left_item->sortOrder() > right_item->sortOrder();

          default:
            return sortOrder() == Qt::SortOrder::AscendingOrder
                     ? QString::localeAwareCompare(left_item->title().toLower(), right_item->title().toLower()) < 0
                     : QString::localeAwareCompare(left_item->title().toLower(), right_item->title().toLower()) > 0;
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

  /*
  qDebugNN << LOGSEC_CORE << "Filter accepts row"
           << QUOTE_W_SPACE(m_sourceModel->itemForIndex(m_sourceModel->index(source_row, 0, source_parent))->title())
           << "and filter result is:" << QUOTE_W_SPACE_DOT(should_show);
  */

  /*
     if (should_show && (!filterRegularExpression().pattern().isEmpty() ||
                      m_showUnreadOnly)) {
     emit expandAfterFilterIn(m_sourceModel->index(source_row, 0, source_parent));
     }
   */

  if (should_show && m_hiddenIndices.contains(QPair<int, QModelIndex>(source_row, source_parent))) {
    qDebugNN << LOGSEC_CORE << "Item was previously hidden and now shows up, expand.";

    const_cast<FeedsProxyModel*>(this)->m_hiddenIndices.removeAll(QPair<int, QModelIndex>(source_row, source_parent));

    // Now, item now should be displayed and previously it was not.
    // Expand!
    emit expandAfterFilterIn(m_sourceModel->index(source_row, 0, source_parent));
  }

  if (!should_show) {
    const_cast<FeedsProxyModel*>(this)->m_hiddenIndices.append(QPair<int, QModelIndex>(source_row, source_parent));
  }

  return should_show;
}

bool FeedsProxyModel::filterAcceptsRowInternal(int source_row, const QModelIndex& source_parent) const {
  const QModelIndex idx = m_sourceModel->index(source_row, 0, source_parent);

  if (!idx.isValid()) {
    return false;
  }

  const RootItem* item = m_sourceModel->itemForIndex(idx);

  if (item->kind() != RootItem::Kind::Category && item->kind() != RootItem::Kind::Feed &&
      item->kind() != RootItem::Kind::Label) {
    // Some items are always visible.
    return true;
  }

  if (!m_showUnreadOnly) {
    // Take only regexp filtering into account.
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
  }
  else {
    // NOTE: If item has < 0 of unread messages it may mean, that the count
    // of unread messages is not (yet) known, display that item too.
    //
    // Also, the actual selected item should not be filtered out too.
    // This is primarily to make sure that the selection does not "vanish", this
    // particularly manifests itself if user uses "next unread item" action and
    // "show unread only" is enabled too and user for example selects last unread
    // article in a feed -> then the feed would disappear from list suddenly.
    return m_selectedItem == item ||
           (item->countOfUnreadMessages() != 0 && QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent));
  }
}

bool FeedsProxyModel::sortAlphabetically() const {
  return m_sortAlphabetically;
}

void FeedsProxyModel::sort(int column, Qt::SortOrder order) {
  QSortFilterProxyModel::sort(column, order);
}

void FeedsProxyModel::setView(FeedsView* newView) {
  m_view = newView;
}

const RootItem* FeedsProxyModel::selectedItem() const {
  return m_selectedItem;
}

void FeedsProxyModel::setSelectedItem(const RootItem* selected_item) {
  m_selectedItem = selected_item;
}

bool FeedsProxyModel::showUnreadOnly() const {
  return m_showUnreadOnly;
}

void FeedsProxyModel::invalidateReadFeedsFilter(bool set_new_value, bool show_unread_only) {
  if (set_new_value) {
    setShowUnreadOnly(show_unread_only);
  }

  QTimer::singleShot(0, this, &FeedsProxyModel::invalidateFilter);
}

void FeedsProxyModel::setShowUnreadOnly(bool show_unread_only) {
  m_showUnreadOnly = show_unread_only;
  qApp->settings()->setValue(GROUP(Feeds), Feeds::ShowOnlyUnreadFeeds, show_unread_only);
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

  for (const QModelIndex& index : indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}
