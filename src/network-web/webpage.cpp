// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webpage.h"

#include "definitions/definitions.h"
#include "gui/webviewer.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <QString>
#include <QStringList>
#include <QUrl>

WebPage::WebPage(QObject* parent) : QWebEnginePage(parent) {
  setBackgroundColor(Qt::transparent);
}

WebViewer* WebPage::view() const {
  return qobject_cast<WebViewer*>(QWebEnginePage::view());
}

void WebPage::javaScriptAlert(const QUrl& securityOrigin, const QString& msg) {
  QStringList parts = msg.split(QL1C('-'));

  if (parts.size() == 2) {
    int message_id = parts.at(0).toInt();
    const QString& action = parts.at(1);

    if (action == QSL("read")) {
      emit messageStatusChangeRequested(message_id, MarkRead);
    }
    else if (action == QSL("unread")) {
      emit messageStatusChangeRequested(message_id, MarkUnread);
    }
    else if (action == QSL("starred")) {
      emit messageStatusChangeRequested(message_id, MarkStarred);
    }
    else if (action == QSL("unstarred")) {
      emit messageStatusChangeRequested(message_id, MarkUnstarred);
    }
    else {
      QWebEnginePage::javaScriptAlert(securityOrigin, msg);
    }
  }
  else {
    QWebEnginePage::javaScriptAlert(securityOrigin, msg);
  }
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) {
  const RootItem* root = view()->root();

  if (url.toString().startsWith(INTERNAL_URL_PASSATTACHMENT) &&
      root != nullptr &&
      root->getParentServiceRoot()->downloadAttachmentOnMyOwn(url)) {
    return false;
  }

  if (url.host() == INTERNAL_URL_MESSAGE_HOST) {
    setHtml(view()->messageContents(), QUrl(INTERNAL_URL_MESSAGE));
    return true;
  }
  else {
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
  }
}
