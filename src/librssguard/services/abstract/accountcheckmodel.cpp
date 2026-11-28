// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/accountcheckmodel.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/feed.h"

#include <utility>

AccountCheckModel::AccountCheckModel(QObject* parent)
  : QAbstractItemModel(parent), m_rootItem(nullptr), m_recursiveChange(false) {}

RootItem* AccountCheckModel::itemForIndex(const QModelIndex& index) const {
  if (index.isValid() && index.model() == this) {
    return static_cast<RootItem*>(index.internalPointer());
  }
  else {
    return m_rootItem;
  }
}

RootItem* AccountCheckModel::rootItem() const {
  return m_rootItem;
}

void AccountCheckModel::setRootItem(RootItem* root_item, bool delete_previous_root, bool with_layout_change) {
  if (with_layout_change) {
    emit layoutAboutToBeChanged();
  }

  beginResetModel();

  if (delete_previous_root && m_rootItem != nullptr) {
    m_rootItem->deleteLater();
  }

  m_checkStates.clear();
  m_unavailableItems.clear();
  m_rootItem = root_item;

  endResetModel();

  if (with_layout_change) {
    emit layoutChanged();
  }
}

void AccountCheckModel::checkAllItems() {
  if (m_rootItem != nullptr) {
    auto chi = m_rootItem->childItems();

    for (RootItem* root_child : std::as_const(chi)) {
      if (root_child->kind() == RootItem::Kind::Feed || root_child->kind() == RootItem::Kind::Category) {
        setItemChecked(root_child, Qt::CheckState::Checked);
      }
    }
  }
}

void AccountCheckModel::uncheckAllItems() {
  if (m_rootItem != nullptr) {
    auto chi = m_rootItem->childItems();

    for (RootItem* root_child : std::as_const(chi)) {
      if (root_child->kind() == RootItem::Kind::Feed || root_child->kind() == RootItem::Kind::Category) {
        setData(indexForItem(root_child), Qt::CheckState::Unchecked, Qt::ItemDataRole::CheckStateRole);
      }
    }
  }
}

QModelIndex AccountCheckModel::index(int row, int column, const QModelIndex& parent) const {
  if (!hasIndex(row, column, parent)) {
    return {};
  }

  RootItem* parent_item = itemForIndex(parent);
  RootItem* child_item = parent_item->child(row);

  if (child_item != nullptr) {
    return createIndex(row, column, child_item);
  }
  else {
    return {};
  }
}

QModelIndex AccountCheckModel::indexForItem(RootItem* item) const {
  if (item == nullptr || item->kind() == RootItem::Kind::ServiceRoot || item->kind() == RootItem::Kind::Root) {
    // Root item lies on invalid index.
    return QModelIndex();
  }

  QList<QModelIndex> parents;

  // Start with root item (which obviously has invalid index).
  parents << indexForItem(m_rootItem);

  while (!parents.isEmpty()) {
    QModelIndex active_index = parents.takeFirst();
    int row_count = rowCount(active_index);

    if (row_count > 0) {
      // This index has children.
      // Lets take a look if our target item is among them.
      RootItem* active_item = itemForIndex(active_index);
      int candidate_index = active_item->childItems().indexOf(item);

      if (candidate_index >= 0) {
        // We found our item.
        return index(candidate_index, 0, active_index);
      }
      else {
        // Item is not found, add all "categories" from active_item.
        for (int i = 0; i < row_count; i++) {
          RootItem* possible_category = active_item->child(i);

          if (possible_category->kind() == RootItem::Kind::Category) {
            parents << index(i, 0, active_index);
          }
        }
      }
    }
  }

  return QModelIndex();
}

QModelIndex AccountCheckModel::parent(const QModelIndex& child) const {
  if (!child.isValid()) {
    return {};
  }

  RootItem* child_item = itemForIndex(child);
  RootItem* parent_item = child_item->parent();

  if (parent_item == m_rootItem || parent_item == nullptr) {
    return {};
  }
  else {
    return createIndex(parent_item->row(), 0, parent_item);
  }
}

int AccountCheckModel::rowCount(const QModelIndex& parent) const {
  if (parent.column() > 0) {
    return 0;
  }
  else {
    RootItem* item = itemForIndex(parent);

    if (item != nullptr) {
      return item->childCount();
    }
    else {
      return 0;
    }
  }
}

int AccountCheckModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent)
  return 1;
}

QVariant AccountCheckModel::data(const QModelIndex& index, int role) const {
  if (index.column() != 0) {
    return QVariant();
  }

  RootItem* item = itemForIndex(index);

  if (role == Qt::ItemDataRole::CheckStateRole) {
    if (!m_unavailableItems.contains(item) && m_checkStates.contains(item)) {
      return m_checkStates.value(item);
    }

    return static_cast<int>(Qt::CheckState::Unchecked);
  }
  else if (role == Qt::ItemDataRole::DecorationRole) {
    return item->data(0, Qt::ItemDataRole::DecorationRole);
  }
  else if (role == Qt::ItemDataRole::EditRole) {
    return QVariant::fromValue(item);
  }
  else if (role == Qt::ItemDataRole::ForegroundRole) {
    if (item->kind() == RootItem::Kind::Feed && Feed::isErrorStatus(item->toFeed()->status())) {
      return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);
    }
  }
  else if (role == Qt::ItemDataRole::DisplayRole) {
    switch (item->kind()) {
      case RootItem::Kind::Category:
        return QVariant(item->data(index.column(), role).toString() + QSL(" ") + tr("(folder)"));

      case RootItem::Kind::Feed:
        return QVariant(item->data(index.column(), role).toString() + QSL(" ") + tr("(feed)"));

      default:
        return item->title();
    }
  }
  else if (role == Qt::ItemDataRole::ToolTipRole) {
    const auto reason = m_unavailableItems.value(item);

    if (item->kind() == RootItem::Kind::Feed) {
      auto desc = item->description();
      auto url = item->toFeed()->source();

      QString txt = url;

      if (!desc.isEmpty()) {
        txt += QSL("\n\n%1").arg(desc);
      }

      return reason.isEmpty() ? txt : QSL("%1\n\n%2").arg(reason, txt);
    }

    if (!reason.isEmpty()) {
      return reason;
    }
  }

  return QVariant();
}

bool AccountCheckModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (index.isValid() && index.column() == 0 && role == Qt::CheckStateRole) {
    RootItem* item = itemForIndex(index);
    const Qt::CheckState check_state = static_cast<Qt::CheckState>(value.toInt());

    if (item == m_rootItem || m_unavailableItems.contains(item)) {
      return false;
    }

    if (!m_recursiveChange && m_checkStateChangeInterceptor && !m_checkStateChangeInterceptor(item, check_state)) {
      return false;
    }

    // Change data for the actual item.
    m_checkStates[item] = check_state;
    emit dataChanged(index, index);
    emit checkStateChanged(item, m_checkStates[item]);

    if (m_recursiveChange) {
      return true;
    }

    // Set new data for all descendants of this actual item.
    auto chi = item->childItems();

    for (RootItem* child : std::as_const(chi)) {
      setData(indexForItem(child), value, Qt::CheckStateRole);
    }

    // Now we need to change new data to all parents.
    QModelIndex parent_index = index;

    m_recursiveChange = true;

    // Iterate all valid parents.
    while ((parent_index = parent_index.parent()).isValid()) {
      item = item->parent();
      setData(parent_index, childCheckState(item), Qt::CheckStateRole);
    }

    m_recursiveChange = false;
    return true;
  }

  return false;
}

Qt::CheckState AccountCheckModel::childCheckState(const RootItem* item) const {
  bool all_checked = true;
  bool all_unchecked = true;

  for (RootItem* child : item->childItems()) {
    if (m_unavailableItems.contains(child)) {
      continue;
    }

    const auto state = m_checkStates.value(child, Qt::CheckState::Unchecked);
    all_checked &= state == Qt::CheckState::Checked;
    all_unchecked &= state == Qt::CheckState::Unchecked;
  }

  if (all_unchecked) {
    return Qt::CheckState::Unchecked;
  }

  return all_checked ? Qt::CheckState::Checked : Qt::CheckState::PartiallyChecked;
}

void AccountCheckModel::setUnavailableItems(QHash<RootItem*, QString> items) {
  if (m_unavailableItems != items) {
    m_unavailableItems.swap(items);
    refreshAvailability({}, items);
  }
}

void AccountCheckModel::refreshAvailability(const QModelIndex& current, const QHash<RootItem*, QString>& previous) {
  const auto count = rowCount(current);

  for (int row = 0; row < count; ++row) {
    refreshAvailability(index(row, 0, current), previous);
  }

  if (current.isValid()) {
    auto* item = itemForIndex(current);
    const auto old_state = m_checkStates.value(item, Qt::CheckState::Unchecked);
    const bool unavailable = m_unavailableItems.contains(item);
    const auto state = count > 0 && !unavailable ? childCheckState(item) : old_state;

    if (old_state != state) {
      m_checkStates[item] = state;
    }

    if (old_state != state || unavailable != previous.contains(item) ||
        previous.value(item) != m_unavailableItems.value(item)) {
      emit dataChanged(current, current);
    }
  }
}

Qt::ItemFlags AccountCheckModel::flags(const QModelIndex& index) const {
  if (!index.isValid() || (itemForIndex(index)->kind() != RootItem::Kind::Category &&
                           itemForIndex(index)->kind() != RootItem::Kind::Feed)) {
    return Qt::ItemFlag::NoItemFlags;
  }

  Qt::ItemFlags flags = Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;

  if (m_unavailableItems.contains(itemForIndex(index))) {
    return Qt::ItemFlag::NoItemFlags;
  }

  if (index.column() == 0) {
    flags |= Qt::ItemFlag::ItemIsUserCheckable;
  }

  return flags;
}

QList<RootItem*> AccountCheckModel::checkedItems() const {
  auto keys = m_checkStates.keys();
  auto res = qlinq::from(keys)
               .where([&](const auto& key) {
                  return !m_unavailableItems.contains(key) && m_checkStates.value(key) == Qt::CheckState::Checked;
               })
               .toList();

  return res;
}

void AccountCheckModel::removeCheckState(RootItem* item) {
  for (RootItem* it : item->getSubTree<RootItem>()) {
    m_checkStates.remove(it);
  }
}

bool AccountCheckModel::isItemChecked(RootItem* item) const {
  return !m_unavailableItems.contains(item) &&
         m_checkStates.value(item, Qt::CheckState::Unchecked) != Qt::CheckState::Unchecked;
}

bool AccountCheckModel::setItemChecked(RootItem* item, Qt::CheckState check) {
  return setData(indexForItem(item), check, Qt::ItemDataRole::CheckStateRole);
}

void AccountCheckModel::setCheckStateChangeInterceptor(std::function<bool(RootItem*, Qt::CheckState)> interceptor) {
  m_checkStateChangeInterceptor = std::move(interceptor);
}

AccountCheckSortedModel::AccountCheckSortedModel(QObject* parent)
  : QSortFilterProxyModel(parent), m_sourceModel(new AccountCheckModel(parent)) {
  setDynamicSortFilter(false);
  setSourceModel(m_sourceModel);
  sort(0, Qt::SortOrder::AscendingOrder);
}

bool AccountCheckSortedModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
  auto* lhs = m_sourceModel->itemForIndex(source_left);
  auto* rhs = m_sourceModel->itemForIndex(source_right);

  if (lhs != nullptr && rhs != nullptr) {
    QList<RootItem::Kind> priorities = {RootItem::Kind::Category,
                                        RootItem::Kind::Feed,
                                        RootItem::Kind::Labels,
                                        RootItem::Kind::Important,
                                        RootItem::Kind::Unread,
                                        RootItem::Kind::Bin};

    const bool left_pinned = lhs->keepOnTop();
    const bool right_pinned = rhs->keepOnTop();
    if (left_pinned != right_pinned) {
      return sortOrder() == Qt::SortOrder::AscendingOrder ? left_pinned : right_pinned;
    }

    auto left_priority = priorities.indexOf(lhs->kind());
    auto right_priority = priorities.indexOf(rhs->kind());

    if (left_priority == right_priority) {
      return QString::localeAwareCompare(lhs->title().toLower(), rhs->title().toLower()) < 0;
    }
    else {
      return sortOrder() == Qt::SortOrder::AscendingOrder ? left_priority < right_priority
                                                          : right_priority < left_priority;
    }
  }

  return false;
}

AccountCheckModel* AccountCheckSortedModel::sourceModel() const {
  return m_sourceModel;
}

void AccountCheckSortedModel::setRootItem(RootItem* root_item, bool delete_previous_root, bool with_layout_change) {
  setSourceModel(nullptr);
  m_sourceModel->setRootItem(root_item, delete_previous_root, with_layout_change);
  setSourceModel(m_sourceModel);
  sort(0, Qt::SortOrder::AscendingOrder);
}

void AccountCheckSortedModel::checkAllItems() {
  m_sourceModel->checkAllItems();
}

void AccountCheckSortedModel::uncheckAllItems() {
  m_sourceModel->uncheckAllItems();
}

bool AccountCheckSortedModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  auto kind = m_sourceModel->itemForIndex(m_sourceModel->index(source_row, 0, source_parent))->kind();

  return kind == RootItem::Kind::Root || kind == RootItem::Kind::ServiceRoot || kind == RootItem::Kind::Category ||
         kind == RootItem::Kind::Feed;
}
