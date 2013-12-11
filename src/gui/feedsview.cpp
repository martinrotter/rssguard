#include "gui/feedsview.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"


FeedsView::FeedsView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}
