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
    m_sourceConnections << connect(source_model,
                                   &QAbstractItemModel::rowsInserted,
                                   this,
                                   &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
    m_sourceConnections << connect(source_model,
                                   &QAbstractItemModel::rowsRemoved,
                                   this,
                                   &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
    m_sourceConnections << connect(source_model,
                                   &QAbstractItemModel::modelReset,
                                   this,
                                   &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
    m_sourceConnections << connect(source_model,
                                   &QAbstractItemModel::layoutChanged,
                                   this,
                                   &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
    m_sourceConnections << connect(source_model,
                                   &QAbstractItemModel::dataChanged,
                                   this,
                                   &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
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

int FilteringProxyModel::recursiveRowCount(const QAbstractItemModel* model, const QModelIndex& parent) const {
  if (model == nullptr) {
    return 0;
  }

  const int child_count = model->rowCount(parent);
  int rows = child_count;

  for (int row = 0; row < child_count; ++row) {
    rows += recursiveRowCount(model, model->index(row, 0, parent));
  }

  return rows;
}

void FilteringProxyModel::refreshFilteredOutItemsState() {
  const bool has_filtered_out_items =
    sourceModel() != nullptr && recursiveRowCount(sourceModel()) > recursiveRowCount(this);

  if (has_filtered_out_items != m_hasFilteredOutItems) {
    m_hasFilteredOutItems = has_filtered_out_items;
    emit filteredOutItemsChanged(m_hasFilteredOutItems);
  }
}

void FilteringProxyModel::connectModelSignals(const QAbstractItemModel* model) {
  connect(model,
          &QAbstractItemModel::rowsInserted,
          this,
          &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
  connect(model,
          &QAbstractItemModel::rowsRemoved,
          this,
          &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
  connect(model,
          &QAbstractItemModel::modelReset,
          this,
          &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
  connect(model,
          &QAbstractItemModel::layoutChanged,
          this,
          &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
  connect(model,
          &QAbstractItemModel::dataChanged,
          this,
          &FilteringProxyModel::scheduleFilteredOutItemsRefresh);
}

void FilteringProxyModel::clearSourceConnections() {
  for (const QMetaObject::Connection& connection : std::as_const(m_sourceConnections)) {
    disconnect(connection);
  }

  m_sourceConnections.clear();
}
