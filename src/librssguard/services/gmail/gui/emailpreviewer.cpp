// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/emailpreviewer.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

EmailPreviewer::EmailPreviewer(QWidget* parent) : CustomMessagePreviewer(parent), m_webView(new WebBrowser(nullptr, this)) {
  m_ui.setupUi(this);

  m_ui.m_mainLayout->addWidget(dynamic_cast<QWidget*>(m_webView.data()), 3, 0, 1, -1);
  m_ui.m_btnAttachments->setIcon(qApp->icons()->fromTheme(QSL("mail-attachment")));
  m_ui.m_btnForward->setIcon(qApp->icons()->fromTheme(QSL("mail-forward")));
  m_ui.m_btnReply->setIcon(qApp->icons()->fromTheme(QSL("mail-reply-sender")));

  m_webView->setNavigationBarVisible(false);
}

EmailPreviewer::~EmailPreviewer() {
  qDebugNN << LOGSEC_GMAIL << "Email previewer destroyed.";
}

void EmailPreviewer::clear() {
  m_webView->clear(false);
}

void EmailPreviewer::loadMessage(const Message& msg, RootItem* selected_item) {
  m_webView->setHtml(msg.m_contents);
  m_ui.m_tbFrom->setText(msg.m_author);
  m_ui.m_tbSubject->setText(msg.m_title);

  // TODO: todo
  m_ui.m_tbTo->setText(QSL("-"));
}
