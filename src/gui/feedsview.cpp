#include <QHeaderView>

#include "gui/feedsview.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/defs.h"


FeedsView::FeedsView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);

#if QT_VERSION >= 0x050000
  // Setup column resize strategies.
  header()->setSectionResizeMode(FDS_TITLE_INDEX, QHeaderView::Stretch);
  header()->setSectionResizeMode(FDS_COUNTS_INDEX, QHeaderView::ResizeToContents);
#else
  // Setup column resize strategies.
  header()->setResizeMode(FDS_TITLE_INDEX, QHeaderView::Stretch);
  header()->setResizeMode(FDS_COUNTS_INDEX, QHeaderView::ResizeToContents);
#endif

  header()->setStretchLastSection(false);
  setUniformRowHeights(true);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}
