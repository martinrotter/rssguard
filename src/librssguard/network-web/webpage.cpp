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
