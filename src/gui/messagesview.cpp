#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>

#include "gui/messagesview.h"
#include "gui/formmain.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"
#include "core/settings.h"


MessagesView::MessagesView(QWidget *parent)
  : QTreeView(parent), m_contextMenu(NULL), m_batchUnreadSwitch(false) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  // Forward count changes to the view.
  connect(m_sourceModel, SIGNAL(feedCountsChanged()),
          this, SIGNAL(feedCountsChanged()));

  setModel(m_proxyModel);

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

void MessagesView::keyPressEvent(QKeyEvent *event) {
  QTreeView::keyPressEvent(event);

  if (event->key() == Qt::Key_Delete) {
    deleteSelectedMessages();
  }
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

  if (mapped_current_index.isValid()) {
    if (!m_batchUnreadSwitch) {
      // Set this message as read only if current item
      // wasn't changed by "mark selected messages unread" action.
      m_sourceModel->setMessageRead(mapped_current_index.row(), 1);
    }

    emit currentMessageChanged(m_sourceModel->messageAt(mapped_current_index.row()));
  }
  else {
    emit currentMessageRemoved();
  }

  QTreeView::currentChanged(current, previous);
}

void MessagesView::loadFeeds(const QList<int> feed_ids) {
  m_sourceModel->loadMessages(feed_ids);
}

void MessagesView::openSelectedSourceArticlesExternally() {

  QString browser = Settings::getInstance()->value(APP_CFG_MESSAGES,
                                                   "external_browser_executable").toString();
  QString arguments = Settings::getInstance()->value(APP_CFG_MESSAGES,
                                                     "external_browser_arguments",
                                                     "%1").toString();

  if (browser.isEmpty() || arguments.isEmpty()) {
    QMessageBox::critical(this,
                          tr("External browser not set"),
                          tr("External browser is not set, head to application settings and set it up to use this feature."),
                          QMessageBox::Ok);
    return;
  }

  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    QString link = m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row()).m_url;

    QProcess::execute(browser, QStringList() << arguments.arg(link));
  }
}

void MessagesView::openSelectedSourceMessagesInternally() {
  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    // TODO: What to do with messages w/o link?
    emit openLinkMessageNewTabRequested(m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row()).m_url);
  }
}

void MessagesView::openSelectedMessagesInternally() {
  foreach (const QModelIndex &index, selectionModel()->selectedRows()) {
    emit openMessageNewTabRequested(m_sourceModel->messageAt(m_proxyModel->mapToSource(index).row()));
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
    emit currentMessageRemoved();
  }
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

void MessagesView::setAllMessagesReadStatus(int read) {
  QModelIndex current_index = selectionModel()->currentIndex();
  QModelIndex mapped_current_index = m_proxyModel->mapToSource(current_index);
  QModelIndexList selected_indexes = selectionModel()->selectedRows();
  QModelIndexList mapped_indexes = m_proxyModel->mapListToSource(selected_indexes);

  m_sourceModel->setAllMessagesRead(read);
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

void MessagesView::setAllMessagesRead() {
  setAllMessagesReadStatus(1);
}

void MessagesView::setAllMessagesUnread() {
  setAllMessagesReadStatus(0);
}

void MessagesView::reselectIndexes(const QModelIndexList &indexes) {
  selectionModel()->clearSelection();

  foreach (const QModelIndex &index, indexes) {
    selectionModel()->select(index,
                             QItemSelectionModel::Select |
                             QItemSelectionModel::Rows);
  }
}
