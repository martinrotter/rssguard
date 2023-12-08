// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/newspaperpreviewer.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagepreviewer.h"
#include "miscellaneous/application.h"

#include <QScrollBar>

NewspaperPreviewer::NewspaperPreviewer(int msg_height, RootItem* root, QList<Message> messages, QWidget* parent)
  : TabContent(parent), m_msgHeight(msg_height), m_ui(new Ui::NewspaperPreviewer), m_root(root), m_messages(std::move(messages)) {
  m_ui->setupUi(this);
  m_ui->m_btnShowMoreMessages->setIcon(qApp->icons()->fromTheme(QSL("view-refresh")));
  connect(m_ui->m_btnShowMoreMessages, &QPushButton::clicked, this, &NewspaperPreviewer::showMoreMessages);
  showMoreMessages();
}

#if defined(NO_LITE)

WebBrowser* NewspaperPreviewer::webBrowser() const {
  return nullptr;
}

#endif

void NewspaperPreviewer::showMoreMessages() {
  if (!m_root.isNull()) {
    int current_scroll = m_ui->scrollArea->verticalScrollBar()->value();

    for (int i = 0; i < 5 && !m_messages.isEmpty(); i++) {
      Message msg = m_messages.takeFirst();
      auto* prev = new MessagePreviewer(true, this);
      QMargins margins = prev->layout()->contentsMargins();

      prev->setContentsMargins(margins);

      connect(prev, &MessagePreviewer::markMessageRead, this, &NewspaperPreviewer::markMessageRead);
      connect(prev, &MessagePreviewer::markMessageImportant, this, &NewspaperPreviewer::markMessageImportant);

      m_ui->m_layout->insertWidget(m_ui->m_layout->count() - 1, prev);
      prev->loadMessage(msg, m_root.data());
    }

    m_ui->m_btnShowMoreMessages->setText(tr("Show more articles (%n remaining)", "", m_messages.size()));
    m_ui->m_btnShowMoreMessages->setEnabled(!m_messages.isEmpty());
    m_ui->scrollArea->verticalScrollBar()->setValue(current_scroll);
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent, {
      tr("Cannot show more articles"),
      tr("Cannot show more articles because parent feed was removed."),
      QSystemTrayIcon::MessageIcon::Warning });
  }
}
