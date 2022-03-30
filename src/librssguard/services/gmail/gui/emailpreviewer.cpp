#include "services/gmail/gui/emailpreviewer.h"

EmailPreviewer::EmailPreviewer(QWidget* parent) : CustomMessagePreviewer(parent) {
  m_ui.setupUi(this);
}

EmailPreviewer::~EmailPreviewer() {
  qDebugNN << LOGSEC_GMAIL << "Email previewer destroyed.";
}

void EmailPreviewer::clear() {}

void EmailPreviewer::loadMessage(const Message& msg, RootItem* selected_item) {}
