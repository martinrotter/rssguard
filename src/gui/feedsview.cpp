#include <QHeaderView>

#include "core/feedsmodelfeed.h"
#include "gui/feedsview.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/defs.h"


FeedsView::FeedsView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);
  setupAppearance();
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}

void FeedsView::setSortingEnabled(bool enable) {
  QTreeView::setSortingEnabled(enable);
  header()->setSortIndicatorShown(false);
}

void FeedsView::setupAppearance() {
#if QT_VERSION >= 0x050000
  // Setup column resize strategies.
  header()->setSectionResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::Stretch);
  header()->setSectionResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeToContents);
#else
  // Setup column resize strategies.
  header()->setResizeMode(FDS_MODEL_TITLE_INDEX, QHeaderView::Stretch);
  header()->setResizeMode(FDS_MODEL_COUNTS_INDEX, QHeaderView::ResizeToContents);
#endif

  header()->setStretchLastSection(false);
  setUniformRowHeights(true);
  setAcceptDrops(false);
  setDragEnabled(false);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void FeedsView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected) {
  QModelIndexList selection = selectionModel()->selectedRows();
  QModelIndexList mapped_selection = m_proxyModel->mapListToSource(selection);
  QList<FeedsModelFeed*> selected_feeds = m_sourceModel->feedsForIndexes(mapped_selection);
  QList<int> feed_ids;

  foreach (FeedsModelFeed *feed, selected_feeds) {
    feed_ids << feed->id();
  }

  emit feedsSelected(feed_ids);
}
