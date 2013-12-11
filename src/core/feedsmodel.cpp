#include "core/feedsmodel.h"


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");
}
