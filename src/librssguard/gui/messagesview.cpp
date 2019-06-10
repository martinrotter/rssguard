// This file is part of RSS Guard.

// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/styleditemdelegatewithoutfocus.h"
#include "gui/treeviewcolumnsmenu.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QFileIconProvider>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QScrollBar>
#include <QTimer>

MessagesView::MessagesView(QWidget* parent) : QTreeView(parent), m_contextMenu(nullptr), m_columnsAdjusted(false) {
  m_sourceModel = qApp->feedReader()->messagesModel();
  m_proxyModel = qApp->feedReader()->messagesProxyModel();

  // Forward count changes to the view.
  createConnections();
  setModel(m_proxyModel);
  setupAppearance();
  header()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(header(), &QHeaderView::customContextMenuRequested, this, [=](const QPoint& point) {
    TreeViewColumnsMenu mm(header());
    mm.exec(header()->mapToGlobal(point));
  });

  reloadFontSettings();
}

MessagesView::~MessagesView() {
  qDebug("Destroying MessagesView instance.");
}

void MessagesView::reloadFontSettings() {
  m_sourceModel->setupFonts();
}

void MessagesView::sort(int column, Qt::SortOrder order, bool repopulate_data, bool change_header, bool emit_changed_from_header) {
  if (change_header && !emit_changed_from_header) {
    header()->blockSignals(true);
  }

  m_sourceModel->addSortState(column, order);

  if (repopulate_data) {
    m_sourceModel->repopulate();
  }

  if (change_header) {
    header()->setSortIndicator(column, order);
    header()->blockSignals(false);
  }
}

void MessagesView::createConnections() {
  connect(this, &MessagesView::doubleClicked, this, &MessagesView::openSelectedSourceMessagesExternally);

  // Adjust columns when layout gets changed.
  connect(header(), &QHeaderView::geometriesChanged, this, &MessagesView::adjustColumns);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &MessagesView::onSortIndicatorChanged);
}

void MessagesView::keyboardSearch(const QString& search) {
  // WARNING: This is quite hacky way how to force selection of next item even
  // with extended selection enabled.
  setSelectionMode(QAbstractItemView::SingleSelection);
  QTreeView::keyboardSearch(search);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MessagesView::reloadSelections() {
  const QDateTime dt1 = QDateTime::currentDateTime();
  QModelIndex current_index = selectionModel()->currentIndex();
  const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  const Message selected_message = m_sourceModel->messageAt(mapped_current_index.row());
  const int col = header()->sortIndicatorSection();
  const Qt::SortOrder ord = header()->sortIndicatorOrder();

  // Reload the model now.
  sort(col, ord, true, false, false);

  // Now, we must find the same previously focused message.
  if (selected_message.m_id > 0) {
    if (m_proxyModel->rowCount() == 0) {
      current_index = QModelIndex();
    }
    else {
      for (int i = 0; i < m_proxyModel->rowCount(); i++) {
        QModelIndex msg_idx = m_proxyModel->index(i, MSG_DB_TITLE_INDEX);
        Message msg = m_sourceModel->messageAt(m_proxyModel->mapToSource(msg_idx).row());

        if (msg.m_id == selected_message.m_id) {
          current_index = msg_idx;
          break;
        }

        if (i == m_proxyModel->rowCount() - 1) {
          current_index = QModelIndex();
        }
      }
    }
  }

  if (current_index.isValid()) {
    scrollTo(current_index);
    setCurrentIndex(current_index);
    reselectIndexes(QModelIndexList() << current_index);
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved();
  }

  const QDateTime dt2 = QDateTime::currentDateTime();

  qDebug("Reloading of msg selections took %lld miliseconds.", dt1.msecsTo(dt2));
}

void MessagesView::setupAppearance() {
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setUniformRowHeights(true);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setExpandsOnDoubleClick(false);
  setRootIsDecorated(false);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setItemsExpandable(false);
  setSortingEnabled(true);
  setAllColumnsShowFocus(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setItemDelegate(new StyledItemDelegateWithoutFocus(this));
  header()->setDefaultSectionSize(MESSAGES_VIEW_DEFAULT_COL);
  header()->setMinimumSectionSize(MESSAGES_VIEW_MINIMUM_COL);
  header()->setCascadingSectionResizes(false);
  header()->setStretchLastSection(false);
}

void MessagesView::focusInEvent(QFocusEvent* event) {
  QTreeView::focusInEvent(event);

  if (currentIndex().isValid()) {
    selectionModel()->select(currentIndex(), QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
  }
}

void MessagesView::keyPressEvent(QKeyEvent* event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedMessages();
  }
}

void MessagesView::contextMenuEvent(QContextMenuEvent* event) {
  const QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    TreeViewColumnsMenu menu(header());

    menu.exec(event->globalPos());
  }
  else {
    // Context menu is not initialized, initialize.
    initializeContextMenu();
    m_contextMenu->exec(event->globalPos());
  }
}

void MessagesView::initializeContextMenu() {
  if (m_contextMenu == nullptr) {
    m_contextMenu = new QMenu(tr("Context menu for messages"), this);
  }

  m_contextMenu->clear();

  QFileIconProvider icon_provider;
  QMenu* menu = new QMenu(tr("Open with external tool"), m_contextMenu);

  menu->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  foreach (const ExternalTool& tool, ExternalTool::toolsFromSettings()) {
    QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu);

    act_tool->setIcon(icon_provider.icon(tool.executable()));
    act_tool->setToolTip(tool.executable());
    act_tool->setData(QVariant::fromValue(tool));
    menu->addAction(act_tool);

    connect(act_tool, &QAction::triggered, this, &MessagesView::openSelectedMessagesWithExternalTool);
  }

  if (menu->actions().isEmpty()) {
    QAction* act_not_tools = new QAction("No external tools activated");

    act_not_tools->setEnabled(false);
    menu->addAction(act_not_tools);
  }

  m_contextMenu->addMenu(menu);
  m_contextMenu->addActions(
    QList<QAction*>() << qApp->mainForm()->m_ui->m_actionSendMessageViaEmail << qApp->mainForm()->m_ui->m_actionOpenSelectedSourceArticlesExternally << qApp->mainForm()->m_ui->m_actionOpenSelectedMessagesInternally << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsRead << qApp->mainForm()->m_ui->m_actionMarkSelectedMessagesAsUnread << qApp->mainForm()->m_ui->m_actionSwitchImportanceOfSelectedMessages <<
      qApp->mainForm()->m_ui->m_actionDeleteSelectedMessages);

  if (m_sourceModel->loadedItem() != nullptr && m_sourceModel->loadedItem()->kind() == RootItemKind::Bin) {
    m_contextMenu->addAction(qApp->mainForm()->m_ui->m_actionRestoreSelectedMessages);
  }
}

void MessagesView::mousePressEvent(QMouseEvent* event) {
  QTreeView::mousePressEvent(event);

  switch (event->button()) {
    case Qt::LeftButton: {
      // Make sure that message importance is switched when user
      // clicks the "important" column.
      const QModelIndex clicked_index = indexAt(event->pos());

      if (clicked_index.isValid()) {
        const QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);

        if (mapped_index.column() == MSG_DB_IMPORTANT_INDEX) {
          if (m_sourceModel->switchMessageImportance(mapped_index.row())) {
            emit currentMessageChanged(m_sourceModel->messageAt(mapped_index.row()), m_sourceModel->loadedItem());
          }
        }
      }

      break;
    }

    case Qt::MiddleButton: {
      // Make sure that message importance is switched when user
      // clicks the "important" column.
      const QModelIndex clicked_index = indexAt(event->pos());

      if (clicked_index.isValid()) {
        const QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);
        const QString url = m_sourceModel->messageAt(mapped_index.row()).m_url;

        if (!url.isEmpty()) {
          qApp->mainForm()->tabWidget()->addLinkedBrowser(url);
        }
      }

      break;
    }

    default:
      break;
  }
}

void MessagesView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  const QModelIndex current_index = currentIndex();
  const QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);

  qDebug("Current row changed - row [%d,%d] source [%d, %d].",
         current_index.row(),
         current_index.column(),
         mapped_current_index.row(),
         mapped_current_index.column());

  if (mapped_current_index.isValid() && selected_rows.count() > 0) {
    Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row());

    // Set this message as read only if current item
    // wasn't changed by "mark selected messages unread" action.
    m_sourceModel->setMessageRead(mapped_current_index.row(), RootItem::Read);
    message.m_isRead = true;

    emit currentMessageChanged(message, m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool()) {
    scrollTo(currentIndex(), QAbstractItemView::PositionAtCenter);
  }

  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::loadItem(RootItem* item) {
  const int col = header()->sortIndicatorSection();
  const Qt::SortOrder ord = header()->sortIndicatorOrder();

  scrollToTop();
  sort(col, ord, false, true, false);
  m_sourceModel->loadMessages(item);

  // Messages are loaded, make sure that previously
  // active message is not shown in browser.
  emit currentMessageRemoved();
}

void MessagesView::openSelectedSourceMessagesExternally() {
  foreach (const QModelIndex& index, selectionModel()->selectedRows()) {
    QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row())
                   .m_url
                   .replace(QRegularExpression("[\\t\\n]"), QString());

    if (!qApp->web()->openUrlInExternalBrowser(link)) {
      qApp->showGuiMessage(tr("Problem with starting external web browser"),
                           tr("External web browser could not be started."),
                           QSystemTrayIcon::Critical);
      return;
    }
  }

  // Finally, mark opened messages as read.
  if (!selectionModel()->selectedRows().isEmpty()) {
    QTimer::singleShot(0, this, SLOT(markSelectedMessagesRead()));
  }
}

void MessagesView::openSelectedMessagesInternally() {
  QList<Message> messages;

  foreach (const QModelIndex& index, selectionModel()->selectedRows()) {
    messages << m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row());
  }

  if (!messages.isEmpty()) {
    emit openMessagesInNewspaperView(m_sourceModel->loadedItem(), messages);
  }
}

void MessagesView::sendSelectedMessageViaEmail() {
  if (selectionModel()->selectedRows().size() == 1) {
    const Message message = m_sourceModel->messageAt(m_proxyModel->mapToSource(selectionModel()->selectedRows().at(0)).row());

    if (!qApp->web()->sendMessageViaEmail(message)) {
      MessageBox::show(this, QMessageBox::Critical, tr("Problem with starting external e-mail client"),
                       tr("External e-mail client could not be started."));
    }
  }
}

void MessagesView::markSelectedMessagesRead() {
  setSelectedMessagesReadStatus(RootItem::Read);
}

void MessagesView::markSelectedMessagesUnread() {
  setSelectedMessagesReadStatus(RootItem::Unread);
}

void MessagesView::setSelectedMessagesReadStatus(RootItem::ReadStatus read) {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRead(mapped_indexes, read);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }
}

void MessagesView::deleteSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  const QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesDeleted(mapped_indexes);
  current_index = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (current_index.isValid()) {
    setCurrentIndex(current_index);

    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }
}

void MessagesView::restoreSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  const QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesRestored(mapped_indexes);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    emit currentMessageRemoved();
  }
}

void MessagesView::switchSelectedMessagesImportance() {
  QModelIndex current_index = selectionModel()->currentIndex();

  if (!current_index.isValid()) {
    return;
  }

  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  const QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->switchBatchMessageImportance(mapped_indexes);
  current_index = m_proxyModel->index(current_index.row(), current_index.column());

  if (current_index.isValid()) {
    emit currentMessageChanged(m_sourceModel->messageAt(m_proxyModel->mapToSource(current_index).row()), m_sourceModel->loadedItem());
  }
  else {
    // Messages were probably removed from the model, nothing can
    // be selected and no message can be displayed.
    emit currentMessageRemoved();
  }
}

void MessagesView::reselectIndexes(const QModelIndexList& indexes) {
  if (indexes.size() < RESELECT_MESSAGE_THRESSHOLD) {
    QItemSelection selection;

    foreach (const QModelIndex& index, indexes) {
      selection.merge(QItemSelection(index, index), QItemSelectionModel::Select);
    }

    selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
}

void MessagesView::selectNextItem() {
  const QModelIndex index_next = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

  if (index_next.isValid()) {
    setCurrentIndex(index_next);
    selectionModel()->select(index_next, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    setFocus();
  }
}

void MessagesView::selectPreviousItem() {
  const QModelIndex index_previous = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);

  if (index_previous.isValid()) {
    setCurrentIndex(index_previous);
    selectionModel()->select(index_previous, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    setFocus();
  }
}

void MessagesView::selectNextUnreadItem() {
  const QModelIndexList selected_rows = selectionModel()->selectedRows();
  int active_row;

  if (!selected_rows.isEmpty()) {
    // Okay, something is selected, start from it.
    active_row = selected_rows.at(0).row();
  }
  else {
    active_row = 0;
  }

  const QModelIndex next_unread = m_proxyModel->getNextPreviousUnreadItemIndex(active_row);

  if (next_unread.isValid()) {
    // We found unread message, mark it.
    setCurrentIndex(next_unread);
    selectionModel()->select(next_unread, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    setFocus();
  }
}

void MessagesView::searchMessages(const QString& pattern) {
  m_proxyModel->setFilterRegExp(pattern);

  if (selectionModel()->selectedRows().size() == 0) {
    emit currentMessageRemoved();
  }
  else {
    // Scroll to selected message, it could become scrolled out due to filter change.
    scrollTo(selectionModel()->selectedRows().at(0));
  }
}

void MessagesView::filterMessages(MessagesModel::MessageHighlighter filter) {
  m_sourceModel->highlightMessages(filter);
}

void MessagesView::openSelectedMessagesWithExternalTool() {
  QAction* sndr = qobject_cast<QAction*>(sender());

  if (sndr != nullptr) {
    auto tool = sndr->data().value<ExternalTool>();

    foreach (const QModelIndex& index, selectionModel()->selectedRows()) {
      const QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row())
                           .m_url
                           .replace(QRegularExpression("[\\t\\n]"), QString());

      if (!link.isEmpty()) {
        if (!QProcess::startDetached(tool.executable(), QStringList() << tool.parameters() << link)) {
          qApp->showGuiMessage(tr("Cannot run external tool"),
                               tr("External tool '%1' could not be started.").arg(tool.executable()),
                               QSystemTrayIcon::Critical);
        }
      }
    }
  }
}

void MessagesView::adjustColumns() {
  if (header()->count() > 0 && !m_columnsAdjusted) {
    m_columnsAdjusted = true;

    // Setup column resize strategies.
    for (int i = 0; i < header()->count(); i++) {
      header()->setSectionResizeMode(i, QHeaderView::Interactive);
    }

    header()->setSectionResizeMode(MSG_DB_TITLE_INDEX, QHeaderView::Stretch);
    header()->setSectionResizeMode(MSG_DB_READ_INDEX, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_IMPORTANT_INDEX, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(MSG_DB_HAS_ENCLOSURES, QHeaderView::ResizeToContents);

    // Hide columns.
    hideColumn(MSG_DB_ID_INDEX);
    hideColumn(MSG_DB_DELETED_INDEX);
    hideColumn(MSG_DB_URL_INDEX);
    hideColumn(MSG_DB_CONTENTS_INDEX);
    hideColumn(MSG_DB_PDELETED_INDEX);
    hideColumn(MSG_DB_ENCLOSURES_INDEX);
    hideColumn(MSG_DB_ACCOUNT_ID_INDEX);
    hideColumn(MSG_DB_CUSTOM_ID_INDEX);
    hideColumn(MSG_DB_CUSTOM_HASH_INDEX);
    hideColumn(MSG_DB_FEED_CUSTOM_ID_INDEX);
  }
}

void MessagesView::onSortIndicatorChanged(int column, Qt::SortOrder order) {
  // Repopulate the shit.
  sort(column, order, true, false, false);

  emit currentMessageRemoved();
}
