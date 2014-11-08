// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/feedsview.h"

#include "definitions/definitions.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelrecyclebin.h"
#include "core/feedsmodelfeed.h"
#include "miscellaneous/systemfactory.h"
#include "gui/formmain.h"
#include "gui/formcategorydetails.h"
#include "gui/formfeeddetails.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"
#include "gui/styleditemdelegatewithoutfocus.h"

#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QPointer>
#include <QPainter>
#include <QTimer>


FeedsView::FeedsView(QWidget *parent)
  : QTreeView(parent),
    m_contextMenuCategoriesFeeds(NULL),
    m_contextMenuEmptySpace(NULL),
    m_contextMenuRecycleBin(NULL),
    m_autoUpdateTimer(new QTimer(this)) {
  setObjectName("FeedsView");

  // Allocate models.
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Connections.
  connect(m_sourceModel, SIGNAL(requireItemValidationAfterDragDrop(QModelIndex)), this, SLOT(validateItemAfterDragDrop(QModelIndex)));
  connect(m_autoUpdateTimer, SIGNAL(timeout()), this, SLOT(executeNextAutoUpdate()));

  setModel(m_proxyModel);
  setupAppearance();

  // Setup the timer.
  updateAutoUpdateStatus();
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}

void FeedsView::quit() {
  if (m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->stop();
  }
}

void FeedsView::updateAutoUpdateStatus() {
  // Restore global intervals.
  // NOTE: Specific per-feed interval are left intact.
  m_globalAutoUpdateInitialInterval = qApp->settings()->value(GROUP(Feeds), "auto_update_interval", DEFAULT_AUTO_UPDATE_INTERVAL).toInt();
  m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  m_globalAutoUpdateEnabled = qApp->settings()->value(GROUP(Feeds), "auto_update_enabled", false).toBool();

  // Start global auto-update timer if it is not running yet.
  // NOTE: The timer must run even if global auto-update
  // is not enabled because user can still enable auto-update
  // for individual feeds.
  if (!m_autoUpdateTimer->isActive()) {
    m_autoUpdateTimer->setInterval(AUTO_UPDATE_INTERVAL);
    m_autoUpdateTimer->start();

    qDebug("Auto-update timer started with interval %d.", m_autoUpdateTimer->interval());
  }
  else {
    qDebug("Auto-update timer is already running.");
  }
}

QList<FeedsModelFeed*> FeedsView::selectedFeeds() const {
  QModelIndex current_index = currentIndex();

  if (current_index.isValid()) {
    return m_sourceModel->feedsForIndex(m_proxyModel->mapToSource(current_index));
  }
  else {
    return QList<FeedsModelFeed*>();
  }
}

QList<FeedsModelFeed*> FeedsView::allFeeds() const {
  return m_sourceModel->allFeeds();
}

FeedsModelRootItem *FeedsView::selectedItem() const {
  QModelIndexList selected_rows = selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) {
    return NULL;
  }

  FeedsModelRootItem *selected_item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(selected_rows.at(0)));
  return selected_item == m_sourceModel->rootItem() ? NULL : selected_item;
}

FeedsModelCategory *FeedsView::selectedCategory() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->categoryForIndex(current_mapped);
}

FeedsModelFeed *FeedsView::selectedFeed() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->feedForIndex(current_mapped);
}

FeedsModelRecycleBin *FeedsView::selectedRecycleBin() const{
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->recycleBinForIndex(current_mapped);
}

void FeedsView::saveExpandedStates() {
  Settings *settings = qApp->settings();

  // Iterate all categories and save their expand statuses.
  foreach (FeedsModelCategory *category, sourceModel()->allCategories().values()) {
    settings->setValue(GROUP(Categories),
                       QString::number(category->id()),
                       isExpanded(model()->mapFromSource(sourceModel()->indexForItem(category))));
  }
}

void FeedsView::loadExpandedStates() {
  Settings *settings = qApp->settings();

  // Iterate all categories and save their expand statuses.
  foreach (FeedsModelCategory *category, sourceModel()->allCategories().values()) {
    setExpanded(model()->mapFromSource(sourceModel()->indexForItem(category)),
                settings->value(GROUP(Categories), QString::number(category->id()), true).toBool());
  }
}

void FeedsView::updateAllFeeds() {
  if (qApp->closeLock()->tryLock()) {
    emit feedsUpdateRequested(allFeeds());
  }
  else {
    qApp->showGuiMessage(tr("Cannot update all items"),
                         tr("You cannot update all items because another feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());
  }
}

void FeedsView::updateAllFeedsOnStartup() {
  if (qApp->settings()->value(GROUP(Feeds), "feeds_update_on_startup", false).toBool()) {
    qDebug("Requesting update for all feeds on application startup.");
    QTimer::singleShot(STARTUP_UPDATE_DELAY, this, SLOT(updateAllFeeds()));
  }
}

void FeedsView::updateSelectedFeeds() {
  if (qApp->closeLock()->tryLock()) {
    emit feedsUpdateRequested(selectedFeeds());
  }
  else {
    qApp->showGuiMessage(tr("Cannot update selected items"),
                         tr("You cannot update selected items because another feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());
  }
}

void FeedsView::executeNextAutoUpdate() {
  if (!qApp->closeLock()->tryLock()) {
    qDebug("Delaying scheduled feed auto-updates for one minute due to another running update.");

    // Cannot update, quit.
    return;
  }

  // If global auto-update is enabled
  // and its interval counter reached zero,
  // then we need to restore it.
  if (m_globalAutoUpdateEnabled && --m_globalAutoUpdateRemainingInterval < 0) {
    // We should start next auto-update interval.
    m_globalAutoUpdateRemainingInterval = m_globalAutoUpdateInitialInterval;
  }

  qDebug("Starting auto-update event, pass %d/%d.", m_globalAutoUpdateRemainingInterval, m_globalAutoUpdateInitialInterval);

  // Pass needed interval data and lets the model decide which feeds
  // should be updated in this pass.
  QList<FeedsModelFeed*> feeds_for_update = m_sourceModel->feedsForScheduledUpdate(m_globalAutoUpdateEnabled &&
                                                                                   m_globalAutoUpdateRemainingInterval == 0);

  if (feeds_for_update.isEmpty()) {
    // No feeds are scheduled for update now, unlock the master lock.
    qApp->closeLock()->unlock();
  }
  else {
    // Request update for given feeds.
    emit feedsUpdateRequested(feeds_for_update);

    // NOTE: OSD/bubble informing about performing
    // of scheduled update can be shown now.
  }
}

void FeedsView::setSelectedFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(selectedFeeds(), clear, 0);
  updateCountsOfSelectedFeeds(true);

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::setAllFeedsClearStatus(int clear) {
  m_sourceModel->markFeedsDeleted(allFeeds(), clear, 0);
  updateCountsOfAllFeeds(true);

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::clearSelectedFeeds() {
  setSelectedFeedsClearStatus(1);
}

void FeedsView::clearAllFeeds() {
  setAllFeedsClearStatus(1);
}

void FeedsView::addNewCategory() {
  if (!qApp->closeLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot add standard category"),
                         tr("You cannot add new standard category now because feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());
    return;
  }

  QPointer<FormCategoryDetails> form_pointer = new FormCategoryDetails(m_sourceModel, this);

  form_pointer.data()->exec(NULL, selectedItem());

  delete form_pointer.data();

  // Changes are done, unlock the update master lock.
  qApp->closeLock()->unlock();
}

void FeedsView::editCategory(FeedsModelCategory *category) {
  QPointer<FormCategoryDetails> form_pointer = new FormCategoryDetails(m_sourceModel, this);

  form_pointer.data()->exec(category, NULL);

  delete form_pointer.data();
}

void FeedsView::addNewFeed() {
  if (!qApp->closeLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot add standard feed"),
                         tr("You cannot add new standard feed now because feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());
    return;
  }

  QPointer<FormFeedDetails> form_pointer = new FormFeedDetails(m_sourceModel, this);

  form_pointer.data()->exec(NULL, selectedItem());

  delete form_pointer.data();

  // Changes are done, unlock the update master lock.
  qApp->closeLock()->unlock();
}

void FeedsView::editFeed(FeedsModelFeed *feed) {
  QPointer<FormFeedDetails> form_pointer = new FormFeedDetails(m_sourceModel, this);

  form_pointer.data()->exec(feed, NULL);

  delete form_pointer.data();
}

void FeedsView::receiveMessageCountsChange(MessagesModel::MessageMode mode,
                                           bool total_msg_count_changed,
                                           bool any_msg_restored) {
  // If the change came from recycle bin mode, then:
  // a) total count of message was changed AND no message was restored - some messages
  // were permanently deleted from recycle bin --> we need to update counts of
  // just recycle bin, including total counts.
  // b) total count of message was changed AND some message was restored - some messages
  // were restored --> we need to update counts from all items and bin, including total counts.
  // c) total count of message was not changed - state of some messages was switched, no
  // deletings or restorings were made --> update counts of just recycle bin, excluding total counts.
  //
  // If the change came from feed mode, then:
  // a) total count of message was changed - some messages were deleted --> we need to update
  // counts of recycle bin, including total counts and update counts of selected feeds, including
  // total counts.
  // b) total count of message was not changed - some messages switched state --> we need to update
  // counts of just selected feeds.

  if (mode == MessagesModel::MessagesFromRecycleBin) {
    if (total_msg_count_changed) {
      if (any_msg_restored) {
        updateCountsOfAllFeeds(true);
      }
      else {
        updateCountsOfRecycleBin(true);
      }
    }
    else {
      updateCountsOfRecycleBin(false);
    }
  }
  else {
    updateCountsOfSelectedFeeds(total_msg_count_changed);
  }
}

void FeedsView::editSelectedItem() {
  if (!qApp->closeLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot edit item"),
                         tr("Selected item cannot be edited because feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());

    // Thus, cannot delete and quit the method.
    return;
  }

  FeedsModelCategory *category;
  FeedsModelFeed *feed;

  if ((category = selectedCategory()) != NULL) {
    editCategory(category);
  }
  else if ((feed = selectedFeed()) != NULL) {
    editFeed(feed);
  }

  // Changes are done, unlock the update master lock.
  qApp->closeLock()->unlock();
}

void FeedsView::deleteSelectedItem() {
  if (!qApp->closeLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot delete item"),
                         tr("Selected item cannot be deleted because feed update is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm());

    // Thus, cannot delete and quit the method.
    return;
  }

  QModelIndex current_index = currentIndex();

  if (!current_index.isValid()) {
    // Changes are done, unlock the update master lock and exit.
    qApp->closeLock()->unlock();
    return;
  }

  if (MessageBox::show(qApp->mainForm(), QMessageBox::Question, tr("Deleting feed or category"),
                       tr("You are about to delete selected feed or category."), tr("Do you really want to delete selected item?"),
                       QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
    // User changed his mind.
    qApp->closeLock()->unlock();
    return;
  }

  if (m_sourceModel->removeItem(m_proxyModel->mapToSource(current_index))) {
    // Item WAS removed, update counts.
    notifyWithCounts();
  }
  else {
    // Item WAS NOT removed, either database-related error occurred
    // or update is undergoing.
    qApp->showGuiMessage(tr("Deletion of item failed."),
                         tr("Selected item was not deleted due to error."),
                         QSystemTrayIcon::Warning, qApp->mainForm());
  }

  // Changes are done, unlock the update master lock.
  qApp->closeLock()->unlock();
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

void FeedsView::clearAllReadMessages() {
  m_sourceModel->markFeedsDeleted(allFeeds(), 1, 1);
}

void FeedsView::openSelectedFeedsInNewspaperMode() {
  QList<Message> messages = m_sourceModel->messagesForFeeds(selectedFeeds());

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);
    QTimer::singleShot(0, this, SLOT(markSelectedFeedsRead()));
  }
}

void FeedsView::emptyRecycleBin() {
  if (MessageBox::show(qApp->mainForm(), QMessageBox::Question, tr("Permanently delete messages"),
                       tr("You are about to permanenty delete all messages from your recycle bin."),
                       tr("Do you really want to empty your recycle bin?"),
                       QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
    // User changed his mind.
    return;
  }

  m_sourceModel->recycleBin()->empty();
  updateCountsOfSelectedFeeds(true);

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::restoreRecycleBin() {
  m_sourceModel->recycleBin()->restore();
  updateCountsOfAllFeeds(true);

  emit feedsNeedToBeReloaded(1);
}

void FeedsView::updateCountsOfSelectedFeeds(bool update_total_too) { 
  foreach (FeedsModelFeed *feed, selectedFeeds()) {
    feed->updateCounts(update_total_too);
  }

  QModelIndexList selected_indexes = m_proxyModel->mapListToSource(selectionModel()->selectedRows());

  if (update_total_too) {
    // Number of items in recycle bin has changed.
    m_sourceModel->recycleBin()->updateCounts(true);

    // We need to refresh data for recycle bin too.
    selected_indexes.append(m_sourceModel->indexForItem(m_sourceModel->recycleBin()));
  }

  // Make sure that selected view reloads changed indexes.
  m_sourceModel->reloadChangedLayout(selected_indexes);
  notifyWithCounts();
}

void FeedsView::updateCountsOfRecycleBin(bool update_total_too) {
  m_sourceModel->recycleBin()->updateCounts(update_total_too);
  m_sourceModel->reloadChangedLayout(QModelIndexList() << m_sourceModel->indexForItem(m_sourceModel->recycleBin()));
  notifyWithCounts();
}

void FeedsView::updateCountsOfAllFeeds(bool update_total_too) {
  foreach (FeedsModelFeed *feed, allFeeds()) {
    feed->updateCounts(update_total_too);
  }

  if (update_total_too) {
    // Number of items in recycle bin has changed.
    m_sourceModel->recycleBin()->updateCounts(true);
  }

  // Make sure that all views reloads its data.
  m_sourceModel->reloadWholeLayout();
  notifyWithCounts();
}

void FeedsView::updateCountsOfParticularFeed(FeedsModelFeed *feed, bool update_total_too) {
  QModelIndex index = m_sourceModel->indexForItem(feed);

  if (index.isValid()) {
    feed->updateCounts(update_total_too, false);
    m_sourceModel->reloadChangedLayout(QModelIndexList() << index);
  }

  notifyWithCounts();
}

void FeedsView::selectNextItem() {
  if (!isExpanded(currentIndex())) {
    expand(currentIndex());
  }

  QModelIndex index_next = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);
    selectionModel()->select(index_next, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    setFocus();
  }
}

void FeedsView::selectPreviousItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (!isExpanded(index_previous)) {
    expand(index_previous);
  }

  index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    selectionModel()->select(index_previous, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    setFocus();
  }
}

void FeedsView::initializeContextMenuCategoriesFeeds() {
  m_contextMenuCategoriesFeeds = new QMenu(tr("Context menu for feeds"), this);
  m_contextMenuCategoriesFeeds->addActions(QList<QAction*>() <<
                                           qApp->mainForm()->m_ui->m_actionUpdateSelectedFeedsCategories <<
                                           qApp->mainForm()->m_ui->m_actionEditSelectedFeedCategory <<
                                           qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                           qApp->mainForm()->m_ui->m_actionMarkSelectedFeedsAsRead <<
                                           qApp->mainForm()->m_ui->m_actionMarkSelectedFeedsAsUnread <<
                                           qApp->mainForm()->m_ui->m_actionDeleteSelectedFeedCategory);
  m_contextMenuCategoriesFeeds->addSeparator();
  m_contextMenuCategoriesFeeds->addActions(QList<QAction*>() <<
                                           qApp->mainForm()->m_ui->m_actionAddCategory <<
                                           qApp->mainForm()->m_ui->m_actionAddFeed);
}

void FeedsView::initializeContextMenuEmptySpace() {
  m_contextMenuEmptySpace = new QMenu(tr("Context menu for empty space"), this);
  m_contextMenuEmptySpace->addAction(qApp->mainForm()->m_ui->m_actionUpdateAllFeeds);
  m_contextMenuEmptySpace->addSeparator();
  m_contextMenuEmptySpace->addActions(QList<QAction*>() <<
                                      qApp->mainForm()->m_ui->m_actionAddCategory <<
                                      qApp->mainForm()->m_ui->m_actionAddFeed);
}

void FeedsView::initializeContextMenuRecycleBin() {
  m_contextMenuRecycleBin = new QMenu(tr("Context menu for recycle bin"), this);
  m_contextMenuRecycleBin->addActions(QList<QAction*>() <<
                                      qApp->mainForm()->m_ui->m_actionRestoreRecycleBin <<
                                      qApp->mainForm()->m_ui->m_actionRestoreSelectedMessagesFromRecycleBin <<
                                      qApp->mainForm()->m_ui->m_actionEmptyRecycleBin);
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

  setUniformRowHeights(true);
  setAnimated(true);
  setSortingEnabled(true);
  setItemsExpandable(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setIndentation(FEEDS_VIEW_INDENTATION);
  setAcceptDrops(false);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setAllColumnsShowFocus(false);
  setRootIsDecorated(false);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setItemDelegate(new StyledItemDelegateWithoutFocus(this));
  header()->setStretchLastSection(false);
  header()->setSortIndicatorShown(false);

  // Sort in ascending order, that is categories are
  // "bigger" than feeds.
  sortByColumn(FDS_MODEL_TITLE_INDEX, Qt::AscendingOrder);
}

void FeedsView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);

  QList<FeedsModelFeed*> selected_feeds = selectedFeeds();
  QList<int> selected_ids;

  if (!selected_feeds.isEmpty()) {
    foreach (FeedsModelFeed *feed, selected_feeds) {
#if defined(DEBUG)
      QModelIndex index_for_feed = m_sourceModel->indexForItem(feed);

      qDebug("Selecting feed '%s' (source index [%d, %d]).", qPrintable(feed->title()), index_for_feed.row(), index_for_feed.column());
#endif

      selected_ids << feed->id();
    }
  }
  else if (selectedRecycleBin() != NULL) {
    selected_ids << ID_RECYCLE_BIN;
  }

  emit feedsSelected(selected_ids);
}

void FeedsView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedItem();
  }
}

void FeedsView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex clicked_index = indexAt(event->pos());

  if (clicked_index.isValid()) {
    QModelIndex mapped_index = model()->mapToSource(clicked_index);
    FeedsModelRootItem *clicked_item = sourceModel()->itemForIndex(mapped_index);

    if (clicked_item->kind() == FeedsModelRootItem::Category || clicked_item->kind() == FeedsModelRootItem::Feed) {
      // Display context menu for categories.
      if (m_contextMenuCategoriesFeeds == NULL) {
        // Context menu is not initialized, initialize.
        initializeContextMenuCategoriesFeeds();
      }

      m_contextMenuCategoriesFeeds->exec(event->globalPos());
    }
    else if (clicked_item->kind() == FeedsModelRootItem::RecycleBin) {
      // Display context menu for recycle bin.
      if (m_contextMenuRecycleBin == NULL) {
        initializeContextMenuRecycleBin();
      }

      m_contextMenuRecycleBin->exec(event->globalPos());
    }
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

void FeedsView::validateItemAfterDragDrop(const QModelIndex &source_index) {
  QModelIndex mapped = m_proxyModel->mapFromSource(source_index);

  if (mapped.isValid()) {
    setExpanded(mapped, true);
    setCurrentIndex(mapped);
  }
}
