// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/filteringproxymodel.h"

#include <utility>

#include <QTimer>

FilteringProxyModel::FilteringProxyModel(QObject* parent)
  : QSortFilterProxyModel(parent), m_hasFilteredOutItems(false), m_refreshScheduled(false) {
  connectModelSignals(this);
}

bool FilteringProxyModel::hasFilteredOutItems() const {
  return m_hasFilteredOutItems;
}

void FilteringProxyModel::setSourceModel(QAbstractItemModel* source_model) {
  clearSourceConnections();

  QSortFilterProxyModel::setSourceModel(source_model);

  if (source_model != nullptr) {
    m_sourceConnections = connectModelSignals(source_model);
  }

  scheduleFilteredOutItemsRefresh();
}

void FilteringProxyModel::scheduleFilteredOutItemsRefresh() {
  if (m_refreshScheduled) {
    return;
  }

  m_refreshScheduled = true;

  QTimer::singleShot(0, this, [this]() {
    m_refreshScheduled = false;
    refreshFilteredOutItemsState();
  });
}

bool FilteringProxyModel::containsFilteredOutItems(const QModelIndex& source_parent,
                                                   const QModelIndex& proxy_parent) const {
  const QAbstractItemModel* src_model = sourceModel();

  if (src_model == nullptr) {
    return false;
  }

  const int source_rows = src_model->rowCount(source_parent);
  const int proxy_rows = rowCount(proxy_parent);

  if (source_rows != proxy_rows) {
    if (!canHideRowsWithoutFilteringIndicator()) {
      return true;
    }

    for (int row = 0; row < source_rows; ++row) {
      const QModelIndex source_child = src_model->index(row, 0, source_parent);
      const QModelIndex proxy_child = mapFromSource(source_child);

      if (!proxy_child.isValid()) {
        if (!isRowHiddenWithoutFilteringIndicator(source_child)) {
          return true;
        }

        continue;
      }

      if (isHierarchicalFilteringModel() && containsFilteredOutItems(source_child, proxy_child)) {
        return true;
      }
    }

    return false;
  }

  if (!isHierarchicalFilteringModel()) {
    return false;
  }

  for (int row = 0; row < proxy_rows; ++row) {
    const QModelIndex proxy_child = index(row, 0, proxy_parent);
    const QModelIndex source_child = mapToSource(proxy_child);

    if (containsFilteredOutItems(source_child, proxy_child)) {
      return true;
    }
  }

  return false;
}

bool FilteringProxyModel::isHierarchicalFilteringModel() const {
  return false;
}

bool FilteringProxyModel::canHideRowsWithoutFilteringIndicator() const {
  return false;
}

bool FilteringProxyModel::isRowHiddenWithoutFilteringIndicator(const QModelIndex& source_index) const {
  Q_UNUSED(source_index)

  return false;
}

void FilteringProxyModel::refreshFilteredOutItemsState() {
  const bool has_filtered_out_items = containsFilteredOutItems();

  if (has_filtered_out_items != m_hasFilteredOutItems) {
    m_hasFilteredOutItems = has_filtered_out_items;
    emit filteredOutItemsChanged(m_hasFilteredOutItems);
  }
}

QList<QMetaObject::Connection> FilteringProxyModel::connectModelSignals(const QAbstractItemModel* model) {
  if (model == nullptr) {
    return {};
  }

  return {
    connect(model, &QAbstractItemModel::rowsInserted, this, &FilteringProxyModel::scheduleFilteredOutItemsRefresh),
    connect(model, &QAbstractItemModel::rowsRemoved, this, &FilteringProxyModel::scheduleFilteredOutItemsRefresh),
    connect(model, &QAbstractItemModel::modelReset, this, &FilteringProxyModel::scheduleFilteredOutItemsRefresh),
    connect(model, &QAbstractItemModel::layoutChanged, this, &FilteringProxyModel::scheduleFilteredOutItemsRefresh),
    connect(model, &QAbstractItemModel::dataChanged, this, &FilteringProxyModel::scheduleFilteredOutItemsRefresh)};
}

void FilteringProxyModel::clearSourceConnections() {
  for (const QMetaObject::Connection& connection : std::as_const(m_sourceConnections)) {
    disconnect(connection);
  }

  m_sourceConnections.clear();
}
