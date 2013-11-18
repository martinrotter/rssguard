#include <QHeaderView>
#include <QKeyEvent>

#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);

  hideColumn(0);
  header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

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

void MessagesView::setupAppearance() {
  header()->setStretchLastSection(true);

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
  QModelIndex ind = m_proxyModel->mapToSource(current);


  qDebug("CurrentChanged %d %d source %d %d",
         current.row(), current.column(),
         ind.row(), ind.column());

  m_sourceModel->setData(m_sourceModel->index(ind.row(), 1), 1, Qt::EditRole);

  QTreeView::currentChanged(current, previous);
}
