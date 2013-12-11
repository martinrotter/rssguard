#include <QDebug>

#include "core/feedsmodelitem.h"


FeedsModelItem::FeedsModelItem() {
}

FeedsModelItem::~FeedsModelItem() {
  qDebug("Destroying BaseFeedsModelItem instance.");
}
