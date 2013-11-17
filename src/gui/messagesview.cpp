#include <QHeaderView>

#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new MessagesProxyModel(this);
  m_sourceModel = m_proxyModel->sourceModel();

  setModel(m_proxyModel);
  setupAppearance();
}

MessagesView::~MessagesView() {
  qDebug("Destroying MessagesView instance.");
}

void MessagesView::setupAppearance() {
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
/*
  if (selected.indexes().size() > 0) {
      QModelIndex ind = m_proxyModel->mapToSource(selected.indexes().at(0));
      QModelIndex a = selected.indexes().at(0);

      qDebug("SelectionChanged %d %d source %d %d",
             selected.indexes().at(0).row(), selected.indexes().at(0).column(),
             ind.row(), ind.column());

      m_sourceModel->setMessageRead(1, ind.row());

      sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

      QModelIndex new_index = m_proxyModel->mapFromSource(ind);

      // TODO: Buď tady obnovovat celý předchozí výběr nějak
      // nebo použít starší kod a optimalizovat ho.
      selectionModel()->clearSelection();
      selectionModel()->blockSignals(true);
      setCurrentIndex(new_index);
      scrollTo(new_index);
      selectionModel()->select(new_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::Current );
      selectionModel()->blockSignals(false);

  }*/

  QTreeView::selectionChanged(selected, deselected);
}

void MessagesView::currentChanged(const QModelIndex &current,
                                  const QModelIndex &previous) {
  QModelIndex ind = m_proxyModel->mapToSource(current);


  qDebug("CurrentChanged %d %d source %d %d",
         current.row(), current.column(),
         ind.row(), ind.column());

  m_sourceModel->setMessageRead(ind.row(), 1);

  QTreeView::currentChanged(current, previous);
}
