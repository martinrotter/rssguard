// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/newspaperpreviewer.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagepreviewer.h"
#include "miscellaneous/application.h"

#include <QScrollBar>

NewspaperPreviewer::NewspaperPreviewer(RootItem* root, QList<Message> messages, QWidget* parent)
  : TabContent(parent), m_ui(new Ui::NewspaperPreviewer), m_root(root), m_messages(messages) {
  m_ui->setupUi(this);
  connect(m_ui->m_btnShowMoreMessages, &QPushButton::clicked, this, &NewspaperPreviewer::showMoreMessages);
  showMoreMessages();
}

NewspaperPreviewer::~NewspaperPreviewer() {}

void NewspaperPreviewer::showMoreMessages() {
  if (!m_root.isNull()) {
    int current_scroll = m_ui->scrollArea->verticalScrollBar()->value();

    for (int i = 0; i < 10 && !m_messages.isEmpty(); i++) {
      Message msg = m_messages.takeFirst();
      MessagePreviewer* prev = new MessagePreviewer(this);
      QMargins margins = prev->layout()->contentsMargins();

      connect(prev, &MessagePreviewer::requestMessageListReload, this, &NewspaperPreviewer::requestMessageListReload);
      margins.setRight(0);
      prev->layout()->setContentsMargins(margins);
      prev->setFixedHeight(300);
      prev->loadMessage(msg, m_root);
      m_ui->m_layout->insertWidget(m_ui->m_layout->count() - 2, prev);
    }

    m_ui->m_btnShowMoreMessages->setText(tr("Show more messages (%n remaining)", "", m_messages.size()));
    m_ui->m_btnShowMoreMessages->setEnabled(!m_messages.isEmpty());
    m_ui->scrollArea->verticalScrollBar()->setValue(current_scroll);
  }
  else {
    qApp->showGuiMessage(tr("Cannot show more messages"),
                         tr("Cannot show more messages because parent feed was removed."),
                         QSystemTrayIcon::Warning,
                         qApp->mainForm(), true);
  }
}
