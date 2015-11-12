// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "core/feedsmodel.h"
#include "core/feedsproxymodel.h"
#include "core/rootitem.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/mutex.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"
#include "gui/styleditemdelegatewithoutfocus.h"
#include "gui/dialogs/formmain.h"
#include "services/abstract/feed.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"
#include "services/standard/gui/formstandardcategorydetails.h"
#include "services/standard/gui/formstandardfeeddetails.h"

#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QPointer>
#include <QPainter>
#include <QTimer>


FeedsView::FeedsView(QWidget *parent)
  : QTreeView(parent),
    m_contextMenuCategories(NULL),
    m_contextMenuFeeds(NULL),
    m_contextMenuEmptySpace(NULL) {
  setObjectName(QSL("FeedsView"));

  // Allocate models.
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Connections.
  connect(m_sourceModel, SIGNAL(feedsUpdateRequested(QList<Feed*>)), this, SIGNAL(feedsUpdateRequested(QList<Feed*>)));
  connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveSortState(int,Qt::SortOrder)));

  setModel(m_proxyModel);
  setupAppearance();
}

FeedsView::~FeedsView() {
  qDebug("Destroying FeedsView instance.");
}

void FeedsView::setSortingEnabled(bool enable) {
  disconnect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveSortState(int,Qt::SortOrder)));
  QTreeView::setSortingEnabled(enable);
  connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(saveSortState(int,Qt::SortOrder)));
}

QList<Feed*> FeedsView::selectedFeeds() const {
  QModelIndex current_index = currentIndex();

  if (current_index.isValid()) {
    return m_sourceModel->feedsForIndex(m_proxyModel->mapToSource(current_index));
  }
  else {
    return QList<Feed*>();
  }
}

QList<Feed*> FeedsView::allFeeds() const {
  return m_sourceModel->allFeeds();
}

RootItem *FeedsView::selectedItem() const {
  QModelIndexList selected_rows = selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) {
    return NULL;
  }
  else {
    RootItem *selected_item = m_sourceModel->itemForIndex(m_proxyModel->mapToSource(selected_rows.at(0)));
    return selected_item == m_sourceModel->rootItem() ? NULL : selected_item;
  }
}

Category *FeedsView::selectedCategory() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->categoryForIndex(current_mapped);
}

Feed *FeedsView::selectedFeed() const {
  QModelIndex current_mapped = m_proxyModel->mapToSource(currentIndex());
  return m_sourceModel->feedForIndex(current_mapped);
}

void FeedsView::saveExpandedStates() {
  Settings *settings = qApp->settings();
  QList<RootItem*> expandable_items;

  expandable_items.append(sourceModel()->rootItem()->getSubTree(RootItemKind::Category));

  // Iterate all categories and save their expand statuses.
  foreach (RootItem *item, expandable_items) {
    QString setting_name = QString::number(qHash(item->title())) + QL1S("-") + QString::number(item->id());

    settings->setValue(GROUP(Categories),
                       setting_name,
                       isExpanded(model()->mapFromSource(sourceModel()->indexForItem(item))));
  }
}

void FeedsView::loadExpandedStates() {
  Settings *settings = qApp->settings();
  QList<RootItem*> expandable_items;

  expandable_items.append(sourceModel()->rootItem()->getSubTree(RootItemKind::Category | RootItemKind::ServiceRoot));

  // Iterate all categories and save their expand statuses.
  foreach (RootItem *item, expandable_items) {
    QString setting_name = QString::number(qHash(item->title())) + QL1S("-") + QString::number(item->id());

    setExpanded(model()->mapFromSource(sourceModel()->indexForItem(item)),
                settings->value(GROUP(Categories), setting_name, true).toBool());
  }
}

void FeedsView::invalidateReadFeedsFilter(bool set_new_value, bool show_unread_only) {
  if (set_new_value) {
    m_proxyModel->setShowUnreadOnly(show_unread_only);
  }

  QTimer::singleShot(0, m_proxyModel, SLOT(invalidateFilter()));
}

void FeedsView::expandCollapseCurrentItem() {
  if (selectionModel()->selectedRows().size() == 1) {
    QModelIndex index = selectionModel()->selectedRows().at(0);

    if (!index.child(0, 0).isValid() && index.parent().isValid()) {
      setCurrentIndex(index.parent());
      index = index.parent();
    }

    isExpanded(index) ? collapse(index) : expand(index);
  }
}

void FeedsView::updateAllItems() {
  emit feedsUpdateRequested(allFeeds());
}

void FeedsView::updateSelectedItems() {
  emit feedsUpdateRequested(selectedFeeds());
}

void FeedsView::updateAllItemsOnStartup() {
  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool()) {
    qDebug("Requesting update for all feeds on application startup.");
    QTimer::singleShot(STARTUP_UPDATE_DELAY, this, SLOT(updateAllItems()));
  }
}

void FeedsView::clearSelectedFeeds() {
  m_sourceModel->markItemCleared(selectedItem(), false);
  updateCountsOfSelectedFeeds(true);

  emit feedsNeedToBeReloaded(true);
}

void FeedsView::clearAllFeeds() {
  m_sourceModel->markItemCleared(m_sourceModel->rootItem(), false);
  updateCountsOfAllFeeds(true);

  emit feedsNeedToBeReloaded(true);
}

void FeedsView::receiveMessageCountsChange(FeedsSelection::SelectionMode mode,
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
  if (mode == FeedsSelection::MessagesFromRecycleBin) {
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

  invalidateReadFeedsFilter();
}

void FeedsView::editSelectedItem() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot edit item"),
                         tr("Selected item cannot be edited because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm(), true);
    // Thus, cannot delete and quit the method.
    return;
  }

  if (selectedItem()->canBeEdited()) {
    selectedItem()->editViaGui();
  }
  else {
    qApp->showGuiMessage(tr("Cannot edit item"),
                         tr("Selected item cannot be edited, this is not (yet?) supported."),
                         QSystemTrayIcon::Warning,
                         qApp->mainForm(),
                         true);
  }

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::deleteSelectedItem() {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot delete item"),
                         tr("Selected item cannot be deleted because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainForm(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  QModelIndex current_index = currentIndex();

  if (!current_index.isValid()) {
    // Changes are done, unlock the update master lock and exit.
    qApp->feedUpdateLock()->unlock();
    return;
  }

  RootItem *selected_item = selectedItem();

  if (selected_item != NULL) {
    if (selected_item->canBeDeleted()) {
      // Ask user first.
      if (MessageBox::show(qApp->mainForm(),
                           QMessageBox::Question,
                           tr("Deleting \"%1\"").arg(selected_item->title()),
                           tr("You are about to completely delete item \"%1\".").arg(selected_item->title()),
                           tr("Are you sure?"),
                           QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
        // User refused.
        qApp->feedUpdateLock()->unlock();
        return;
      }

      // We have deleteable item selected, remove it via GUI.
      if (!selected_item->deleteViaGui()) {
        qApp->showGuiMessage(tr("Cannot delete \"%1\"").arg(selected_item->title()),
                             tr("This item cannot be deleted because something critically failed. Submit bug report."),
                             QSystemTrayIcon::Critical,
                             qApp->mainForm(),
                             true);
      }
      else {
        // Item is gone, cleared from database. We can clear it from model now.
        // NOTE: Cleared from memory here too.
        // TODO: možná toto přesunout taky to metody deleteViaGui
        // a delete selected_item jen volat tady, editViaGui taky obstará všechno,
        // ale tam je to zas komplexnější.
        m_sourceModel->removeItem(m_proxyModel->mapToSource(current_index));
        m_sourceModel->notifyWithCounts();
      }
    }
    else {
      qApp->showGuiMessage(tr("Cannot delete \"%1\"").arg(selected_item->title()),
                           tr("This item cannot be deleted, because it does not support it\nor this functionality is not implemented yet."),
                           QSystemTrayIcon::Critical,
                           qApp->mainForm(),
                           true);
    }
  }

  // Changes are done, unlock the update master lock.
  qApp->feedUpdateLock()->unlock();
}

void FeedsView::markSelectedItemReadStatus(RootItem::ReadStatus read) {
  m_sourceModel->markItemRead(selectedItem(), read);
  updateCountsOfSelectedFeeds(false);

  emit feedsNeedToBeReloaded(read == 1);
}

void FeedsView::markSelectedItemsRead() {
  markSelectedItemReadStatus(RootItem::Read);
}

void FeedsView::markSelectedItemsUnread() {
  markSelectedItemReadStatus(RootItem::Unread);
}

void FeedsView::markAllItemsReadStatus(RootItem::ReadStatus read) {
  m_sourceModel->markItemRead(m_sourceModel->rootItem(), read);
  updateCountsOfAllFeeds(false);

  emit feedsNeedToBeReloaded(read == 1);
}

void FeedsView::markAllItemsRead() {
  markAllItemsReadStatus(RootItem::Read);
}

void FeedsView::clearAllReadMessages() {
  m_sourceModel->markItemCleared(m_sourceModel->rootItem(), true);
}

void FeedsView::openSelectedItemsInNewspaperMode() {
  QList<Message> messages = m_sourceModel->messagesForFeeds(selectedFeeds());

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);
    QTimer::singleShot(0, this, SLOT(markSelectedItemsRead()));
  }
}

void FeedsView::emptyRecycleBin() {
  if (MessageBox::show(qApp->mainForm(), QMessageBox::Question, tr("Permanently delete messages"),
                       tr("You are about to permanenty delete all messages from your recycle bin."),
                       tr("Do you really want to empty your recycle bin?"),
                       QString(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
    // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
    //m_sourceModel->recycleBin()->empty();
    updateCountsOfSelectedFeeds(true);

    emit feedsNeedToBeReloaded(true);
  }
}

void FeedsView::restoreRecycleBin() {
  // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
  //m_sourceModel->recycleBin()->restore();
  updateCountsOfAllFeeds(true);

  emit feedsNeedToBeReloaded(true);
}

void FeedsView::updateCountsOfSelectedFeeds(bool update_total_too) {
  foreach (Feed *feed, selectedFeeds()) {
    feed->updateCounts(update_total_too);
  }

  QModelIndexList selected_indexes = m_proxyModel->mapListToSource(selectionModel()->selectedRows());

  if (update_total_too) {
    // Number of items in recycle bin has changed.

    // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
    //m_sourceModel->recycleBin()->updateCounts(true);

    // We need to refresh data for recycle bin too.

    // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
    //selected_indexes.append(m_sourceModel->indexForItem(m_sourceModel->recycleBin()));
  }

  // Make sure that selected view reloads changed indexes.
  m_sourceModel->reloadChangedLayout(selected_indexes);
  m_sourceModel->notifyWithCounts();
}

void FeedsView::updateCountsOfRecycleBin(bool update_total_too) {

  // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
  //m_sourceModel->recycleBin()->updateCounts(update_total_too);
  //m_sourceModel->reloadChangedLayout(QModelIndexList() << m_sourceModel->indexForItem(m_sourceModel->recycleBin()));
  m_sourceModel->notifyWithCounts();
}

void FeedsView::updateCountsOfAllFeeds(bool update_total_too) {
  foreach (Feed *feed, allFeeds()) {
    feed->updateCounts(update_total_too);
  }

  if (update_total_too) {
    // Number of items in recycle bin has changed.

    // TODO: pridat metodu cisteni standardniho kose nebo vsech kosu.
    //m_sourceModel->recycleBin()->updateCounts(true);
  }

  // Make sure that all views reloads its data.
  m_sourceModel->reloadWholeLayout();
  m_sourceModel->notifyWithCounts();
}

void FeedsView::updateCountsOfParticularFeed(Feed *feed, bool update_total_too) {
  QModelIndex index = m_sourceModel->indexForItem(feed);

  if (index.isValid()) {
    feed->updateCounts(update_total_too);
    m_sourceModel->reloadChangedLayout(QModelIndexList() << index);
  }

  invalidateReadFeedsFilter();
  m_sourceModel->notifyWithCounts();
}

void FeedsView::selectNextItem() {
  QModelIndex index_next = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);
    setFocus();
  }
}

void FeedsView::selectPreviousItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    setFocus();
  }
}

void FeedsView::initializeContextMenuCategories(RootItem *clicked_item) {
  if (m_contextMenuCategories == NULL) {
    m_contextMenuCategories = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuCategories->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuActions();

  m_contextMenuCategories->addActions(QList<QAction*>() <<
                                      qApp->mainForm()->m_ui->m_actionUpdateSelectedItems <<
                                      qApp->mainForm()->m_ui->m_actionEditSelectedItem <<
                                      qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                      qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                      qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread <<
                                      qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  if (!specific_actions.isEmpty()) {
    m_contextMenuCategories->addSeparator();
    m_contextMenuCategories->addActions(specific_actions);
  }
}

void FeedsView::initializeContextMenuFeeds(RootItem *clicked_item) {
  if (m_contextMenuFeeds == NULL) {
    m_contextMenuFeeds = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuFeeds->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenuActions();

  m_contextMenuFeeds->addActions(QList<QAction*>() <<
                                 qApp->mainForm()->m_ui->m_actionUpdateSelectedItems <<
                                 qApp->mainForm()->m_ui->m_actionEditSelectedItem <<
                                 qApp->mainForm()->m_ui->m_actionViewSelectedItemsNewspaperMode <<
                                 qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsRead <<
                                 qApp->mainForm()->m_ui->m_actionMarkSelectedItemsAsUnread <<
                                 qApp->mainForm()->m_ui->m_actionDeleteSelectedItem);

  if (!specific_actions.isEmpty()) {
    m_contextMenuFeeds->addSeparator();
    m_contextMenuFeeds->addActions(specific_actions);
  }
}

void FeedsView::initializeContextMenuEmptySpace() {
  m_contextMenuEmptySpace = new QMenu(tr("Context menu for empty space"), this);
  m_contextMenuEmptySpace->addAction(qApp->mainForm()->m_ui->m_actionUpdateAllItems);
  m_contextMenuEmptySpace->addSeparator();
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

  sortByColumn(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortColumnFeeds)).toInt(),
               static_cast<Qt::SortOrder>(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortOrderFeeds)).toInt()));
}

void FeedsView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  RootItem *selected_item = selectedItem();

  m_proxyModel->setSelectedItem(selected_item);
  QTreeView::selectionChanged(selected, deselected);
  emit feedsSelected(FeedsSelection(selected_item));
  invalidateReadFeedsFilter();
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
    RootItem *clicked_item = sourceModel()->itemForIndex(mapped_index);

    if (clicked_item->kind() == RootItemKind::Category) {
      // Display context menu for categories.
      initializeContextMenuCategories(clicked_item);
      m_contextMenuCategories->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItemKind::Feed) {
      // Display context menu for feeds.
      initializeContextMenuFeeds(clicked_item);
      m_contextMenuFeeds->exec(event->globalPos());
    }
    else {
      // TODO: volaz specificke menu polozky? zobrazovat menu pro dalsi typy
      // polozek jako odpadkovy kos atp.
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

void FeedsView::saveSortState(int column, Qt::SortOrder order) {
  qApp->settings()->setValue(GROUP(GUI), GUI::DefaultSortColumnFeeds, column);
  qApp->settings()->setValue(GROUP(GUI), GUI::DefaultSortOrderFeeds, order);
}

void FeedsView::validateItemAfterDragDrop(const QModelIndex &source_index) {
  QModelIndex mapped = m_proxyModel->mapFromSource(source_index);

  if (mapped.isValid()) {
    expand(mapped);
    setCurrentIndex(mapped);
  }
}
