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
#include "services/abstract/rootitem.h"
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
    m_contextMenuEmptySpace(NULL),
    m_contextMenuOtherItems(NULL) {
  setObjectName(QSL("FeedsView"));

  // Allocate models.
  m_proxyModel = new FeedsProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Connections.
  connect(m_sourceModel, SIGNAL(requireItemValidationAfterDragDrop(QModelIndex)), this, SLOT(validateItemAfterDragDrop(QModelIndex)));
  connect(m_sourceModel, SIGNAL(itemExpandRequested(QList<RootItem*>,bool)), this, SLOT(onItemExpandRequested(QList<RootItem*>,bool)));
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

void FeedsView::saveExpandedStates() {
  Settings *settings = qApp->settings();
  QList<RootItem*> expandable_items;

  expandable_items.append(sourceModel()->rootItem()->getSubTree(RootItemKind::Category | RootItemKind::ServiceRoot));

  // Iterate all categories and save their expand statuses.
  foreach (RootItem *item, expandable_items) {
    QString setting_name = QString::number(item->kind()) + QL1S("-") +  QString::number(qHash(item->title())) + QL1S("-") + QString::number(item->id());

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
    QString setting_name = QString::number(item->kind()) + QL1S("-") +  QString::number(qHash(item->title())) + QL1S("-") + QString::number(item->id());

    setExpanded(model()->mapFromSource(sourceModel()->indexForItem(item)),
                settings->value(GROUP(Categories), setting_name, item->childCount() > 0).toBool());
  }

  sortByColumn(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortColumnFeeds)).toInt(),
               static_cast<Qt::SortOrder>(qApp->settings()->value(GROUP(GUI), SETTING(GUI::DefaultSortOrderFeeds)).toInt()));
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
  m_sourceModel->updateAllFeeds();
}

void FeedsView::updateSelectedItems() {
  m_sourceModel->updateFeeds(selectedFeeds());
}

void FeedsView::clearSelectedFeeds() {
  m_sourceModel->markItemCleared(selectedItem(), false);
}

void FeedsView::clearAllFeeds() {
  m_sourceModel->markItemCleared(m_sourceModel->rootItem(), false);
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
}

void FeedsView::markSelectedItemRead() {
  markSelectedItemReadStatus(RootItem::Read);
}

void FeedsView::markSelectedItemUnread() {
  markSelectedItemReadStatus(RootItem::Unread);
}

void FeedsView::markAllItemsReadStatus(RootItem::ReadStatus read) {
  m_sourceModel->markItemRead(m_sourceModel->rootItem(), read);
}

void FeedsView::markAllItemsRead() {
  markAllItemsReadStatus(RootItem::Read);
}

void FeedsView::openSelectedItemsInNewspaperMode() {
  QList<Message> messages = m_sourceModel->messagesForItem(selectedItem());

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);
    QTimer::singleShot(150, this, SLOT(markSelectedItemRead()));
  }
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

void FeedsView::switchVisibility() {
  setVisible(!isVisible());
}

QMenu *FeedsView::initializeContextMenuCategories(RootItem *clicked_item) {
  if (m_contextMenuCategories == NULL) {
    m_contextMenuCategories = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuCategories->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenu();

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

  return m_contextMenuCategories;
}

QMenu *FeedsView::initializeContextMenuFeeds(RootItem *clicked_item) {
  if (m_contextMenuFeeds == NULL) {
    m_contextMenuFeeds = new QMenu(tr("Context menu for categories"), this);
  }
  else {
    m_contextMenuFeeds->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenu();

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

  return m_contextMenuFeeds;
}

QMenu *FeedsView::initializeContextMenuEmptySpace() {
  if (m_contextMenuEmptySpace == NULL) {
    m_contextMenuEmptySpace = new QMenu(tr("Context menu for empty space"), this);
    m_contextMenuEmptySpace->addAction(qApp->mainForm()->m_ui->m_actionUpdateAllItems);
    m_contextMenuEmptySpace->addSeparator();
  }

  return m_contextMenuEmptySpace;
}

QMenu *FeedsView::initializeContextMenuOtherItem(RootItem *clicked_item) {
  if (m_contextMenuOtherItems == NULL) {
    m_contextMenuOtherItems = new QMenu(tr("Context menu for other items"), this);
  }
  else {
    m_contextMenuOtherItems->clear();
  }

  QList<QAction*> specific_actions = clicked_item->contextMenu();

  if (!specific_actions.isEmpty()) {
    m_contextMenuOtherItems->addSeparator();
    m_contextMenuOtherItems->addActions(specific_actions);
  }
  else {
    m_contextMenuOtherItems->addAction(qApp->mainForm()->m_ui->m_actionNoActions);
  }

  return m_contextMenuOtherItems;
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
}

void FeedsView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  RootItem *selected_item = selectedItem();

  m_proxyModel->setSelectedItem(selected_item);
  QTreeView::selectionChanged(selected, deselected);
  emit itemSelected(selected_item);
  m_proxyModel->invalidateReadFeedsFilter();
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
      initializeContextMenuCategories(clicked_item)->exec(event->globalPos());
    }
    else if (clicked_item->kind() == RootItemKind::Feed) {
      // Display context menu for feeds.
      initializeContextMenuFeeds(clicked_item)->exec(event->globalPos());
    }
    else {
      initializeContextMenuOtherItem(clicked_item)->exec(event->globalPos());
    }
  }
  else {
    // Display menu for empty space.
    initializeContextMenuEmptySpace()->exec(event->globalPos());
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

void FeedsView::onItemExpandRequested(const QList<RootItem*> &items, bool exp) {
  foreach (RootItem *item, items) {
    QModelIndex source_index = m_sourceModel->indexForItem(item);
    QModelIndex proxy_index = m_proxyModel->mapFromSource(source_index);

    setExpanded(proxy_index, !exp);
    setExpanded(proxy_index, exp);
  }
}
