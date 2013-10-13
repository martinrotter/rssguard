#include "gui/messagesview.h"


MessagesView::MessagesView(QWidget *parent) : QTreeView(parent) {
}

MessagesView::~MessagesView() {
  qDebug("Destroying MessagesView instance.");
}
