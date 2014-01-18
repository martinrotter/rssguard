#include "core/messagesproxymodel.h"

#include "core/messagesmodel.h"


MessagesProxyModel::MessagesProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent) {
  m_sourceModel = new MessagesModel(this);

  setObjectName("MessagesProxyModel");
  setSortRole(Qt::EditRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setFilterKeyColumn(-1);
  setFilterRole(Qt::EditRole);
  setDynamicSortFilter(false);
  setSourceModel(m_sourceModel);
}

MessagesProxyModel::~MessagesProxyModel() {
  qDebug("Destroying MessagesProxyModel instance.");
}

bool MessagesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  // TODO: Maybe use QString::localeAwareCompare() here for
  // title at least, but this will be probably little slower
  // than default implementation.
  return QSortFilterProxyModel::lessThan(left, right);
}

QModelIndexList MessagesProxyModel::mapListFromSource(const QModelIndexList &indexes, bool deep) {
  QModelIndexList mapped_indexes;

  foreach (const QModelIndex &index, indexes) {
    if (deep) {
      // Construct new source index.
      mapped_indexes << mapFromSource(m_sourceModel->index(index.row(), index.column()));
    }
    else {
      mapped_indexes << mapFromSource(index);
    }
  }

  return mapped_indexes;
}

QModelIndexList MessagesProxyModel::mapListToSource(const QModelIndexList &indexes) {
  QModelIndexList source_indexes;

  foreach (const QModelIndex &index, indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}
