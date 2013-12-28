#include "gui/feedsview.h"

#include "core/defs.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/feedsmodelrootitem.h"
#include "gui/formmain.h"

#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>


FeedsView::FeedsView(QWidget *parent) : QTreeView(parent), m_contextMenu(NULL) {
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

QList<FeedsModelFeed *> FeedsView::selectedFeeds() const {
  QModelIndexList selection = selectionModel()->selectedRows();
  QModelIndexList mapped_selection = m_proxyModel->mapListToSource(selection);

  return m_sourceModel->feedsForIndexes(mapped_selection);
}

QList<FeedsModelFeed *> FeedsView::allFeeds() const {
  return m_sourceModel->getAllFeeds();
}

void FeedsView::setSelectedFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(selectedFeeds(), clear);
  updateCountsOfSelectedFeeds();

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::clearSelectedFeeds() {
  setSelectedFeedsClearStatus(1);
}

void FeedsView::markSelectedFeedsReadStatus(int read) {
  m_sourceModel->markFeedsRead(selectedFeeds(), read);
  updateCountsOfSelectedFeeds(false);

  emit feedsNeedToBeReloaded(read);
}

void FeedsView::markSelectedFeedsRead() {
  markSelectedFeedsReadStatus(1);
}

void FeedsView::markSelectedFeedsUnread() {
  markSelectedFeedsReadStatus(0);
}

void FeedsView::updateCountsOfSelectedFeeds(bool update_total_too) {
  foreach (FeedsModelFeed *feed, selectedFeeds()) {
    feed->updateCounts(update_total_too);
  }

  // Make sure that selected view reloads changed indexes.
  m_sourceModel->reloadChangedLayout(m_proxyModel->mapListToSource(selectionModel()->selectedRows()));
}

void FeedsView::updateCountsOfAllFeeds(bool update_total_too) {
  foreach (FeedsModelFeed *feed, allFeeds()) {
    feed->updateCounts(update_total_too);
  }

  // Make sure that all views reloads its data.
  m_sourceModel->reloadWholeLayout();
}

void FeedsView::initializeContextMenu() {
  m_contextMenu = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenu->addActions(QList<QAction*>() <<
                            FormMain::getInstance()->m_ui->m_actionUpdateSelectedFeeds <<
                            FormMain::getInstance()->m_ui->m_actionMarkFeedsAsRead <<
                            FormMain::getInstance()->m_ui->m_actionMarkFeedsAsUnread);
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
  setIndentation(10);
  sortByColumn(0, Qt::AscendingOrder);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  // TODO: Check if stylesheets or drawBranches(...) reimplementation
  // is better for hiding the branches of the view.
  setRootIsDecorated(false);
}

void FeedsView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);

  m_selectedFeeds.clear();

  foreach (FeedsModelFeed *feed, selectedFeeds()) {
    m_selectedFeeds << feed->id();
  }

  emit feedsSelected(m_selectedFeeds);
}

void FeedsView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    qDebug("Context menu for FeedsView will not be shown because "
           "user clicked on invalid item.");
    return;
  }

  if (m_contextMenu == NULL) {
    // Context menu is not initialized, initialize.
    initializeContextMenu();
  }

  m_contextMenu->exec(event->globalPos());
}
