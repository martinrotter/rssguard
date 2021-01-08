// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webpage.h"

#include "definitions/definitions.h"
#include "gui/webviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/webfactory.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

WebPage::WebPage(QObject* parent) : QWebEnginePage(parent) {
  setBackgroundColor(Qt::transparent);
}

WebViewer* WebPage::view() const {
  return qobject_cast<WebViewer*>(QWebEnginePage::view());
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) {
  const RootItem* root = view()->root();

  if (isMainFrame) {
    auto* adblock_rule = qApp->web()->adBlock()->block(AdblockRequestInfo(url));

    if (adblock_rule != nullptr) {
      // This website is entirely blocked.
      QUrlQuery query;
      QUrl new_url(QSL("%1:///%2/").arg(APP_LOW_NAME, ADBLOCK_ADBLOCKED_PAGE));

      query.addQueryItem(QSL("rule"), adblock_rule->filter());
      query.addQueryItem(QSL("subscription"), adblock_rule->subscription()->title());
      new_url.setQuery(query);

      setUrl(new_url);
      return false;
    }
  }

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
