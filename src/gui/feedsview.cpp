#include "gui/feedsview.h"


FeedsView::FeedsView(QWidget *parent) : QTreeView(parent) {
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}
