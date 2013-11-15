#include "gui/messagesview.h"
#include "core/messagesproxymodel.h"
#include "core/messagesmodel.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
  m_proxyModel = new MessagesProxyModel(this);

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
}
