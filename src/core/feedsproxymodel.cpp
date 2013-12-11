#include "core/feedsproxymodel.h"


FeedsProxyModel::FeedsProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent) {
}

FeedsProxyModel::~FeedsProxyModel() {
  qDebug("Destroying FeedsProxyModel instance");
}
