#include "core/feedsproxymodel.h"
#include "core/feedsmodel.h"


FeedsProxyModel::FeedsProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent) {
  m_sourceModel = new FeedsModel(this);




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
  return QSortFilterProxyModel::lessThan(left, right);
}
