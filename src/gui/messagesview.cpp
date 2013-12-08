#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMenu>

#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"
#include "gui/formmain.h"


MessagesView::MessagesView(QWidget *parent)
  : QTreeView(parent), m_contextMenu(NULL) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);

  // FIXME: Sometimes ASSERT occurs if model provides less columns
  // than we set resize mode for.
  qDebug("Loading MessagesView with %d columns.",
         header()->count());

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
  header()->setSectionResizeMode(MSG_DB_DUPDATED_INDEX, QHeaderView::Interactive);
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
  header()->setResizeMode(MSG_DB_DUPDATED_INDEX, QHeaderView::Interactive);
  header()->setResizeMode(MSG_DB_CONTENTS_INDEX, QHeaderView::Interactive);
#endif

  // Hide columns.
  hideColumn(MSG_DB_ID_INDEX);
  hideColumn(MSG_DB_DELETED_INDEX);
  hideColumn(MSG_DB_FEED_INDEX);
  hideColumn(MSG_DB_URL_INDEX);
  hideColumn(MSG_DB_CONTENTS_INDEX);

  // NOTE: It is recommended to call this after the model is set
  // due to sorting performance.
  setupAppearance();
}

MessagesView::~MessagesView() {
  qDebug("Destroying MessagesView instance.");
}

MessagesModel *MessagesView::sourceModel() {
  return m_sourceModel;
}

MessagesProxyModel *MessagesView::model() {
  return m_proxyModel;
}

void MessagesView::setSortingEnabled(bool enable) {
  QTreeView::setSortingEnabled(enable);
  header()->setSortIndicatorShown(false);
}

void MessagesView::setupAppearance() {
  header()->setStretchLastSection(false);
  setUniformRowHeights(true);
  setAcceptDrops(false);
  setDragEnabled(false);
  setDragDropMode(QAbstractItemView::NoDragDrop);
  setExpandsOnDoubleClick(false);
  setRootIsDecorated(false);
  setItemsExpandable(false);
  setSortingEnabled(true);
  setAllColumnsShowFocus(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MessagesView::selectionChanged(const QItemSelection &selected,
                                    const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);
}

void MessagesView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    qDebug("Context menu for MessagesView will not be shown because "
           "user clicked on invalid item.");
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
                            FormMain::getInstance()->m_ui->m_actionOpenSelectedSourceArticlesExternally <<
                            FormMain::getInstance()->m_ui->m_actionOpenSelectedSourceArticlesInternally <<
                            FormMain::getInstance()->m_ui->m_actionOpenSelectedMessagesInternally <<
                            FormMain::getInstance()->m_ui->m_actionMarkSelectedMessagesAsRead <<
                            FormMain::getInstance()->m_ui->m_actionMarkSelectedMessagesAsUnread <<
                            FormMain::getInstance()->m_ui->m_actionSwitchImportanceOfSelectedMessages <<
                            FormMain::getInstance()->m_ui->m_actionDeleteSelectedMessages);
}

void MessagesView::mousePressEvent(QMouseEvent *event) {
  QTreeView::mousePressEvent(event);

  if (event->button() != Qt::LeftButton) {
    // No need for extra actions on right/middle click.
    return;
  }

  QModelIndex clicked_index = indexAt(event->pos());

  if (!clicked_index.isValid()) {
    qDebug("Clicked on invalid index in MessagesView.");
    return;
  }

  QModelIndex mapped_index = m_proxyModel->mapToSource(clicked_index);

  if (mapped_index.column() == MSG_DB_IMPORTANT_INDEX) {
    m_sourceModel->switchMessageImportance(mapped_index.row());
  }
}

void MessagesView::currentChanged(const QModelIndex &current,
                                  const QModelIndex &previous) {
  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current);

  qDebug("Current row changed, row [%d,%d] source %d %d",
         current.row(), current.column(),
         mapped_current_index.row(), mapped_current_index.column());

  if (!signalsBlocked()) {
    if (mapped_current_index.isValid()) {
      m_sourceModel->setMessageRead(mapped_current_index.row(), 1);
      emit currentMessageChanged(m_sourceModel->messageAt(mapped_current_index.row()));
    }
    else {
      emit currentMessageRemoved();
    }
  }

  QTreeView::currentChanged(current, previous);
}

void MessagesView::openSelectedSourceArticlesExternally() {
  // TODO: otevře vybrane zpravy v externim prohlizeci
}

void MessagesView::openSelectedSourceMessagesInternally() {
  // TODO: otevre vybrane zpravy ze zdrojovych webz v internch tabech
}

void MessagesView::openSelectedMessagesInternally() {
  // TODO: otevre vybrane nactene zpravy v internich tabech
}

void MessagesView::markSelectedMessagesRead() {
  setSelectedMessagesReadStatus(1);
}

void MessagesView::markSelectedMessagesUnread() {
  setSelectedMessagesReadStatus(0);
}

void MessagesView::setSelectedMessagesReadStatus(int read) {
  QModelIndex current_index = selectionModel()->currentIndex();
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
    blockSignals(true);
    setCurrentIndex(current_index);
    blockSignals(false);
  }
  else {
    setCurrentIndex(current_index);
  }

  scrollTo(current_index);
  reselectIndexes(selected_indexes);
}

void MessagesView::deleteSelectedMessages() {
  QModelIndex current_index = selectionModel()->currentIndex();
  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setBatchMessagesDeleted(mapped_indexes, 1);

  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

  selected_indexes = m_proxyModel->mapListFromSource(mapped_indexes, true);
  current_index = m_proxyModel->mapFromSource(m_sourceModel->index(mapped_current_index.row(),
                                                                   mapped_current_index.column()));

  setCurrentIndex(current_index);
  scrollTo(current_index);
  reselectIndexes(selected_indexes);
}

void MessagesView::switchSelectedMessagesImportance() {
  QModelIndex current_index = selectionModel()->currentIndex();
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

  foreach (const QModelIndex &index, indexes) {
    selectionModel()->select(index,
                             QItemSelectionModel::Select |
                             QItemSelectionModel::Rows);
  }
}
