#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollBar>

#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
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
    m_sourceModel->setMessageRead(mapped_current_index.row(), 1);
    emit currentMessageChanged(m_sourceModel->messageAt(mapped_current_index.row()));
  }
  else {
    emit currentMessageRemoved();
  }

  QTreeView::currentChanged(current, previous);
}

void MessagesView::switchSelectedMessagesImportance() {
  QModelIndex current_idx = selectionModel()->currentIndex();
  QModelIndex mapped_current_idx = m_proxyModel->mapToSource(current_idx);
  QModelIndexList selected_idxs = selectionModel()->selectedRows();
  QModelIndexList mapped_idxs = m_proxyModel->mapListToSource(selected_idxs);

  m_sourceModel->switchBatchMessageImportance(mapped_idxs);

  //selected_idxs.clear();
  selected_idxs = m_proxyModel->mapListFromSource(mapped_idxs);

  sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
/*
  foreach (const QModelIndex &index, mapped_idxs) {
    selected_idxs << m_proxyModel->mapFromSource(m_sourceModel->index(index.row(),
                                                                         index.column()));
  }
*/
  current_idx = m_proxyModel->mapFromSource(m_sourceModel->index(mapped_current_idx.row(),
                                                          mapped_current_idx.column()));

  setCurrentIndex(current_idx);
  scrollTo(current_idx);
  reselectIndexes(selected_idxs);
}

void MessagesView::reselectIndexes(const QModelIndexList &indexes) {
  selectionModel()->clearSelection();

  foreach (const QModelIndex &idx, indexes) {
    selectionModel()->select(idx,
                             QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

void MessagesView::setAllMessagesRead() {
  selectAll();
  QModelIndexList selected_indexes = selectedIndexes();
  QModelIndexList mapp;

  foreach (const QModelIndex &index, selected_indexes) {
    mapp << m_proxyModel->mapToSource(index);
  }

  m_sourceModel->switchBatchMessageImportance(mapp);
}
