#include "gui/feedsview.h"

#include "core/defs.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelstandardcategory.h"
#include "gui/formmain.h"
#include "gui/formcategorydetails.h"

#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QPointer>
#include <QPainter>


FeedsView::FeedsView(QWidget *parent)
  : QTreeView(parent),
    m_contextMenuCategoriesFeeds(NULL),
    m_contextMenuEmptySpace(NULL) {
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
  return m_sourceModel->allFeeds();
}

FeedsModelCategory *FeedsView::isCurrentIndexCategory() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->categoryForIndex(current_mapped);
}

FeedsModelFeed *FeedsView::isCurrentIndexFeed() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->feedForIndex(current_mapped);
}

void FeedsView::setSelectedFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(selectedFeeds(), clear);
  updateCountsOfSelectedFeeds();

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::clearSelectedFeeds() {
  setSelectedFeedsClearStatus(1);
}

void FeedsView::addNewCategory() {
  QPointer<FormCategoryDetails> form_pointer = new FormCategoryDetails(m_sourceModel, this);
  FormCategoryDetailsAnswer answer = form_pointer.data()->exec(NULL);

  if (answer.m_dialogCode == QDialog::Accepted) {
    // User submitted some new category and
    // it now resides in output_item pointer,
    // parent_item contains parent_that user selected for
    // new category.

    // TODO: Add new category to the model and to the database.
  }

  delete form_pointer.data();
}

void FeedsView::editSelectedItem() {
  // TODO: Implement this.
  FeedsModelCategory *category;
  FeedsModelFeed *feed;

  if ((category = isCurrentIndexCategory()) != NULL) {
    // Category is selected.
  }
  else if ((feed = isCurrentIndexFeed()) != NULL) {
    // Feed is selected.
  }

}

void FeedsView::deleteSelectedItem() {
  QModelIndex current_index = currentIndex();
  QItemSelectionModel *selection_model = selectionModel();

  if (!current_index.isValid()) {
    return;
  }

  if (selection_model->selectedRows().size() > 1) {
    // pridat BalloonTip z qonverteru a tady
    // ho odpalit pokud todle nastane
    // s hlaskou "More than one item selected, removing
    // only current one."
    selection_model->clearSelection();
    selection_model->select(current_index, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
  }

  m_sourceModel->removeItem(m_proxyModel->mapToSource(current_index));
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

void FeedsView::markAllFeedsReadStatus(int read) {
  m_sourceModel->markFeedsRead(allFeeds(), read);
  updateCountsOfAllFeeds(false);

  emit feedsNeedToBeReloaded(read);
}

void FeedsView::markAllFeedsRead() {
  markAllFeedsReadStatus(1);
}

void FeedsView::openSelectedFeedsInNewspaperMode() {
  QList<Message> messages = m_sourceModel->messagesForFeeds(selectedFeeds());

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);
  }
}

void FeedsView::updateCountsOfSelectedFeeds(bool update_total_too) {
  QList<FeedsModelFeed*> selected_feeds = selectedFeeds();

  if (!selected_feeds.isEmpty()) {
    foreach (FeedsModelFeed *feed, selected_feeds) {
      feed->updateCounts(update_total_too);
    }

    // Make sure that selected view reloads changed indexes.
    m_sourceModel->reloadChangedLayout(m_proxyModel->mapListToSource(selectionModel()->selectedRows()));

    notifyWithCounts();
  }
}

void FeedsView::updateCountsOfAllFeeds(bool update_total_too) {
  foreach (FeedsModelFeed *feed, allFeeds()) {
    feed->updateCounts(update_total_too);
  }

  // Make sure that all views reloads its data.
  m_sourceModel->reloadWholeLayout();

  notifyWithCounts();
}

void FeedsView::updateCountsOfParticularFeed(FeedsModelFeed *feed,
                                             bool update_total_too) {
  QModelIndex index = m_sourceModel->indexForItem(feed);

  if (index.isValid()) {
    feed->updateCounts(update_total_too);
    m_sourceModel->reloadChangedLayout(QModelIndexList() << index);
  }

  notifyWithCounts();
}

void FeedsView::initializeContextMenuCategoriesFeeds() {
  m_contextMenuCategoriesFeeds = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenuCategoriesFeeds->addActions(QList<QAction*>() <<
                                           FormMain::instance()->m_ui->m_actionUpdateSelectedFeedsCategories <<
                                           FormMain::instance()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                           FormMain::instance()->m_ui->m_actionMarkFeedsAsRead <<
                                           FormMain::instance()->m_ui->m_actionMarkFeedsAsUnread);
}

void FeedsView::initializeContextMenuEmptySpace() {
  m_contextMenuEmptySpace = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenuEmptySpace->addActions(QList<QAction*>() <<
                                      FormMain::instance()->m_ui->m_actionUpdateAllFeeds);

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
  setAnimated(true);
  setSortingEnabled(true);
  setItemsExpandable(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setIndentation(10);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setRootIsDecorated(false);

  // Sort in ascending order, that is categories are
  // "bigger" than feeds.
  sortByColumn(0, Qt::AscendingOrder);
}

void FeedsView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);

  m_selectedFeeds.clear();

  foreach (FeedsModelFeed *feed, selectedFeeds()) {
#if defined(DEBUG)
    QModelIndex index_for_feed = m_sourceModel->indexForItem(feed);

    qDebug("Selecting feed '%s' (source index [%d, %d]).",
           qPrintable(feed->title()), index_for_feed.row(), index_for_feed.column());
#endif

    m_selectedFeeds << feed->id();
  }

  emit feedsSelected(m_selectedFeeds);
}

void FeedsView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedItem();
  }
}

void FeedsView::contextMenuEvent(QContextMenuEvent *event) {
  if (indexAt(event->pos()).isValid()) {
    // Display context menu for categories.
    if (m_contextMenuCategoriesFeeds == NULL) {
      // Context menu is not initialized, initialize.
      initializeContextMenuCategoriesFeeds();
    }

    m_contextMenuCategoriesFeeds->exec(event->globalPos());
  }
  else {
    // Display menu for empty space.
    if (m_contextMenuEmptySpace == NULL) {
      // Context menu is not initialized, initialize.
      initializeContextMenuEmptySpace();
    }

    m_contextMenuEmptySpace->exec(event->globalPos());
  }
}
