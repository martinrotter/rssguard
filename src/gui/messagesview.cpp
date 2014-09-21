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

#include "gui/messagesview.h"

#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "gui/formmain.h"
#include "gui/messagebox.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QMenu>


MessagesView::MessagesView(QWidget *parent)
  : QTreeView(parent),
    m_contextMenu(NULL),
    m_columnsAdjusted(false),
    m_batchUnreadSwitch(false) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Forward count changes to the view.
  createConnections();
  setModel(m_proxyModel);
  setupAppearance();
}

MessagesView::~MessagesView() {
  qDebug("Destroying MessagesView instance.");
}

void MessagesView::createConnections() {
  // Forward feed counts changes.
  connect(m_sourceModel, SIGNAL(feedCountsChanged(bool)), this, SIGNAL(feedCountsChanged(bool)));

  // Make sure that source message is opened
  // in new tab on double click.
  connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openSelectedSourceMessagesInternally()));

  // Adjust columns when layout gets changed.
  connect(header(), SIGNAL(geometriesChanged()), this, SLOT(adjustColumns()));
}

void MessagesView::keyboardSearch(const QString &search) {
  setSelectionMode(QAbstractItemView::SingleSelection);
  QTreeView::keyboardSearch(search);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MessagesView::reloadSelections(int mark_current_index_read) {
  QModelIndex current_index = selectionModel()->currentIndex();
  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  // Reload the model now.
  m_sourceModel->select();
  m_sourceModel->fetchAll();

  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  selected_indexes = m_proxyModel->mapListFromSource(mapped_indexes, true);
  current_index = m_proxyModel->mapFromSource(m_sourceModel->index(mapped_current_index.row(),
                                                                   mapped_current_index.column()));

  if (current_index.isValid()) {
    if (mark_current_index_read == 0) {
      // User selected to mark some messages as unread, if one
      // of them will be marked as current, then it will be read again.
      m_batchUnreadSwitch = true;
      setCurrentIndex(current_index);
      m_batchUnreadSwitch = false;
    }
    else {
      setCurrentIndex(current_index);
    }

    scrollTo(current_index);
    reselectIndexes(selected_indexes);
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    // TOTO: Check if this is OKAY. If not, then emit this signal
    // from FeedsView itself.
    emit currentMessagesRemoved();
  }
}

void MessagesView::setupAppearance() {
  header()->setDefaultSectionSize(MESSAGES_VIEW_DEFAULT_COL);
  header()->setStretchLastSection(false);
  setUniformRowHeights(true);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setExpandsOnDoubleClick(false);
  setRootIsDecorated(false);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setItemsExpandable(false);
  setSortingEnabled(true);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MessagesView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedMessages();
  }
}

void MessagesView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    qDebug("Context menu for MessagesView will not be shown because user clicked on invalid item.");
    return;
  }

  if (m_contextMenu == NULL) {
    // Context menu is not initialized, initialize.
    initializeContextMenu();
  }

  m_contextMenu->exec(event->globalPos());
}

void MessagesView::initializeContextMenu() {
  m_contextMenu = new QMenu(tr("Context menu for messages"), this);
  m_contextMenu->addActions(QList<QAction*>() <<
                            qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesExternally <<
                            qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesInternally <<
                            qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternally <<
                            qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsRead <<
                            qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsUnread <<
                            qApp->mainForm()->m_ui->m_actionSwitchImportanceOfSelectedMessages <<
                            qApp->mainForm()->m_ui->m_actionDeleteSelectedMessages <<
                            qApp->mainForm()->m_ui->m_actionRestoreSelectedMessagesFromRecycleBin);
}

void MessagesView::mousePressEvent(QMouseEvent *event) {
  QTreeView::mousePressEvent(event);

  switch (event->button()) {
    case Qt::LeftButton: {
      // Make sure that message importance is switched when user
      // clicks the "important" column.
      QModelIndex clicked_index = indexAt(event->pos());

      if (clicked_index.isValid()) {
        QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);

        if (mapped_index.column() == MSG_DB_IMPORTANT_INDEX) {
          m_sourceModel->switchMessageImportance(mapped_index.row());
        }
      }

      break;
    }

    case Qt::MiddleButton: {
      // Open selected messages in new tab on mouse middle button click.
      openSelectedSourceMessagesInternally();
      break;
    }

    default:
      break;
  }
}

void MessagesView::currentChanged(const QModelIndex &current,
                                  const QModelIndex &previous) {
  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current);

  qDebug("Current row changed - row [%d,%d] source [%d, %d].",
         current.row(), current.column(),
         mapped_current_index.row(), mapped_current_index.column());

  if (mapped_current_index.isValid()) {
    if (!m_batchUnreadSwitch) {
      // Set this message as read only if current item
      // wasn't changed by "mark selected messages unread" action.
      m_sourceModel->setMessageRead(mapped_current_index.row(), 1);
    }

    emit currentMessagesChanged(QList<Message>() <<
                                m_sourceModel->messageAt(mapped_current_index.row()));
  }
  else {
    emit currentMessagesRemoved();
  }

  QTreeView::currentChanged(current, previous);
}

void MessagesView::selectionChanged(const QItemSelection &selected,
                                    const QItemSelection &deselected) {
  if (qApp->settings()->value(APP_CFG_MESSAGES, "keep_cursor_center", false).toBool()) {
    scrollTo(currentIndex(), QAbstractItemView::PositionAtCenter);
  }

  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::loadFeeds(const QList<int> &feed_ids) {
  m_sourceModel->loadMessages(feed_ids);

  // Make sure that initial sorting is that unread messages are visible
  // first.
  sortByColumn(MSG_DB_DCREATED_INDEX, Qt::DescendingOrder);

  // Messages are loaded, make sure that previously
  // active message is not shown in browser.
  emit currentMessagesRemoved();
}

void MessagesView::openSelectedSourceArticlesExternally() {
  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row()).m_url;

    if (!WebFactory::instance()->openUrlInExternalBrowser(link)) {
      MessageBox::show(this,
                       QMessageBox::Critical,
                       tr("Problem with starting external web browser"),
                       tr("External web browser could not be started."));
      return;
    }
  }

  // Finally, mark opened messages as read.
  markSelectedMessagesRead();
}

void MessagesView::openSelectedSourceMessagesInternally() {
  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row());

    if (message.m_url.isEmpty()) {
      MessageBox::show(this,
                       QMessageBox::Warning,
                       tr("Meesage without URL"),
                       tr("Message '%s' does not contain URL.").arg(message.m_title));
    }
    else {
      emit openLinkNewTab(message.m_url);
    }
  }

  // Finally, mark opened messages as read.
  markSelectedMessagesRead();
}

void MessagesView::openSelectedMessagesInternally() {
  QList<Message> messages;

  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    messages << m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row());
  }

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(messages);

    // Finally, mark opened messages as read.
    markSelectedMessagesRead();
  }
}

void MessagesView::markSelectedMessagesRead() {
  setSelectedMessagesReadStatus(1);
}

void MessagesView::markSelectedMessagesUnread() {
  setSelectedMessagesReadStatus(0);
}

void MessagesView::setSelectedMessagesReadStatus(int read) {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRead(mapped_indexes, read);
  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  selected_indexes = m_proxyModel->mapListFromSource(mapped_indexes, true);
  current_index = m_proxyModel->mapFromSource(m_sourceModel->index(mapped_current_index.row(),
                                                                   mapped_current_index.column()));

  if (read == 0) {
    // User selected to mark some messages as unread, if one
    // of them will be marked as current, then it will be read again.
    m_batchUnreadSwitch = true;
    setCurrentIndex(current_index);
    m_batchUnreadSwitch = false;
  }
  else {
    setCurrentIndex(current_index);
  }

  scrollTo(current_index);
  reselectIndexes(selected_indexes);
}

void MessagesView::deleteSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesDeleted(mapped_indexes, 1);
  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  int row_count = m_sourceModel->rowCount();
  if (row_count > 0) {
    QModelIndex last_item = current_index.row() < row_count ?
                              m_proxyModel->index(current_index.row(),
                                                  MSG_DB_TITLE_INDEX) :
                              m_proxyModel->index(row_count - 1,
                                                  MSG_DB_TITLE_INDEX);

    setCurrentIndex(last_item);
    scrollTo(last_item);
    reselectIndexes(QModelIndexList() << last_item);
  }
  else {
    emit currentMessagesRemoved();
  }
}

void MessagesView::restoreSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRestored(mapped_indexes);
  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  int row_count = m_sourceModel->rowCount();
  if (row_count > 0) {
    QModelIndex last_item = current_index.row() < row_count ?
                              m_proxyModel->index(current_index.row(),
                                                  MSG_DB_TITLE_INDEX) :
                              m_proxyModel->index(row_count - 1,
                                                  MSG_DB_TITLE_INDEX);

    setCurrentIndex(last_item);
    scrollTo(last_item);
    reselectIndexes(QModelIndexList() << last_item);
  }
  else {
    emit currentMessagesRemoved();
  }
}

void MessagesView::switchSelectedMessagesImportance() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->switchBatchMessageImportance(mapped_indexes);
  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  selected_indexes = m_proxyModel->mapListFromSource(mapped_indexes, true);
  current_index = m_proxyModel->mapFromSource(m_sourceModel->index(mapped_current_index.row(),
                                                                   mapped_current_index.column()));

  setCurrentIndex(current_index);
  scrollTo(current_index);
  reselectIndexes(selected_indexes);
}

void MessagesView::reselectIndexes(const QModelIndexList &indexes) {
  selectionModel()->clearSelection();

  QItemSelection selection;

  foreach (const QModelIndex &index, indexes) {
    // TODO: THIS IS very slow. Try to select 4000 messages
    // and hit "mark as read" button.
    selection.merge(QItemSelection(index, index), QItemSelectionModel::Select);
  }

  selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void MessagesView::selectNextItem() {
  QModelIndex index_next = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);
    selectionModel()->select(index_next, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

void MessagesView::selectPreviousItem() {
  QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    selectionModel()->select(index_previous, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

void MessagesView::searchMessages(const QString &pattern) {
  m_proxyModel->setFilterRegExp(pattern);

  if (selectionModel()->selectedRows().size() == 0) {
    emit currentMessagesRemoved();
  }
}

void MessagesView::filterMessages(MessagesModel::MessageFilter filter) {
  m_sourceModel->filterMessages(filter);
}

void MessagesView::adjustColumns() {
  if (header()->count() > 0 && !m_columnsAdjusted) {
    m_columnsAdjusted = true;

#if QT_VERSION >= 0x050000
    // Setup column resize strategies.
    header()->setSectionResizeMode(MSG_DB_ID_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_READ_INDEX, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_DELETED_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_IMPORTANT_INDEX, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_FEED_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_TITLE_INDEX, QHeaderView::Stretch);
    header()->setSectionResizeMode(MSG_DB_URL_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_AUTHOR_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_DCREATED_INDEX, QHeaderView::Interactive);
    header()->setSectionResizeMode(MSG_DB_CONTENTS_INDEX, QHeaderView::Interactive);
#else
    // Setup column resize strategies.
    header()->setResizeMode(MSG_DB_ID_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_READ_INDEX, QHeaderView::ResizeToContents);
    header()->setResizeMode(MSG_DB_DELETED_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_IMPORTANT_INDEX, QHeaderView::ResizeToContents);
    header()->setResizeMode(MSG_DB_FEED_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_TITLE_INDEX, QHeaderView::Stretch);
    header()->setResizeMode(MSG_DB_URL_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_AUTHOR_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_DCREATED_INDEX, QHeaderView::Interactive);
    header()->setResizeMode(MSG_DB_CONTENTS_INDEX, QHeaderView::Interactive);
#endif

    // Hide columns.
    hideColumn(MSG_DB_ID_INDEX);
    hideColumn(MSG_DB_DELETED_INDEX);
    hideColumn(MSG_DB_FEED_INDEX);
    hideColumn(MSG_DB_URL_INDEX);
    hideColumn(MSG_DB_CONTENTS_INDEX);

    qDebug("Adjusting column resize modes for MessagesView.");
  }
}
