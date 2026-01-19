// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/threadpreviewer.h"

#include "src/definitions.h"
#include "src/redditnetworkfactory.h"
#include "src/redditserviceroot.h"
#include "src/redditsubscription.h"

#include <librssguard/exceptions/networkexception.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/gui/messagebox.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/network-web/oauth2service.h>

#include <QJsonObject>

ThreadPreviewer::ThreadPreviewer(RedditServiceRoot* account, QWidget* parent)
  : CustomMessagePreviewer(parent), m_account(account), m_webView(new ThreadWebBrowser(nullptr, this)),
    m_selectedItem(nullptr) {
  m_ui.setupUi(this);

  m_tmrLoadExtraMessageData.setInterval(200);
  m_tmrLoadExtraMessageData.setSingleShot(true);

  m_ui.m_mainLayout->addWidget(dynamic_cast<QWidget*>(m_webView.data()), 3, 0, 1, -1);
  m_ui.m_btnFetchComments->setIcon(qApp->icons()->fromTheme(QSL("download"), QSL("mail-attachment")));

  connect(m_ui.m_btnFetchComments, &QPushButton::clicked, this, &ThreadPreviewer::fetchComments);

  m_webView->setToolBarVisible(false);
}

ThreadPreviewer::~ThreadPreviewer() {
  qDebugNN << LOGSEC_GMAIL << "Thread previewer destroyed.";
}

WebBrowser* ThreadPreviewer::webBrowser() const {
  return m_webView.data();
}

void ThreadPreviewer::clear() {
  m_selectedItem = nullptr;
  m_tmrLoadExtraMessageData.stop();
  m_webView->clear(false);
}

void ThreadPreviewer::loadMessage(const Message& msg, RootItem* selected_item) {
  m_message = msg;
  m_selectedItem = selected_item;

  auto json_comments = QJsonDocument().fromJson(msg.m_customData.toUtf8());

  if (json_comments.isArray()) {
    auto cmnts = RedditComment::fromJson(json_comments.array());
    auto cmnts_html = m_account->network()->commentsTreeToHtml(cmnts, msg.m_title, msg.m_url);

    m_webView->setHtml(msg.m_contents + cmnts_html,
                       m_webView->viewer()->urlForMessage(msg, selected_item),
                       selected_item);
  }
  else {
    m_webView->setHtml(msg.m_contents, m_webView->viewer()->urlForMessage(msg, selected_item), selected_item);
  }
}

void ThreadPreviewer::fetchComments() {
  auto* general_feed = m_account->getItemFromSubTree([this](const RootItem* it) {
    return it->kind() == RootItem::Kind::Feed && it->customId() == m_message.m_feedCustomId;
  });

  if (general_feed == nullptr) {
    return;
  }

  auto* specific_feed = qobject_cast<RedditSubscription*>(general_feed);
  auto subreddit = m_account->network()->prefixedSubredditToBare(specific_feed->prefixedName());
  auto cmnts = m_account->network()->commentsTree(subreddit, m_message.m_customId, m_account->networkProxy());

  m_message.m_customData =
    QString::fromUtf8(QJsonDocument(RedditComment::toJson(cmnts)).toJson(QJsonDocument::JsonFormat::Compact));

  loadMessage(m_message, m_selectedItem);
}

ThreadWebBrowser::ThreadWebBrowser(WebViewer* viewer, QWidget* parent) : WebBrowser(viewer, parent) {}

ThreadWebBrowser::~ThreadWebBrowser() {}

void ThreadWebBrowser::onLinkMouseHighlighted(const QUrl& url) {}

void ThreadWebBrowser::onLinkMouseClicked(const QUrl& url) {}
