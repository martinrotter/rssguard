#include "core/feedsmodel.h"
#include "core/feedsmodelrootitem.h"


FeedsModel::FeedsModel(QObject *parent) : QAbstractItemModel(parent) {
  m_rootItem = new FeedsModelRootItem();
}

FeedsModel::~FeedsModel() {
  qDebug("Destroying FeedsModel instance.");
  delete m_rootItem;
}
