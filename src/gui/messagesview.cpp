#include <QHeaderView>
#include <QKeyEvent>

#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);

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

  // Hide columns.
  hideColumn(MSG_DB_ID_INDEX);
  hideColumn(MSG_DB_DELETED_INDEX);
  hideColumn(MSG_DB_FEED_INDEX);
  hideColumn(MSG_DB_URL_INDEX);
  hideColumn(MSG_DB_CONTENTS_INDEX);


  //hideColumn(0);

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

void MessagesView::currentChanged(const QModelIndex &current,
                                  const QModelIndex &previous) {
  QModelIndex mapped_current = m_proxyModel->mapToSource(current);


  qDebug("Current row changed, row [%d,%d] source %d %d",
         current.row(), current.column(),
         mapped_current.row(), mapped_current.column());

  m_sourceModel->setMessageRead(mapped_current.row(), 1);
  emit currentMessageChanged(m_sourceModel->messageAt(mapped_current.row()));

  QTreeView::currentChanged(current, previous);
}

void MessagesView::setSelectedMessagesReadStatus(int read) {
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
