// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webpage.h"

#include "definitions/definitions.h"
#include "gui/webviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/webfactory.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEngineScript>

WebPage::WebPage(QObject* parent) : QWebEnginePage(parent) {
  setBackgroundColor(Qt::GlobalColor::transparent);

  connect(this, &QWebEnginePage::loadFinished, this, &WebPage::hideUnwantedElements);
}

WebViewer* WebPage::view() const {
  return qobject_cast<WebViewer*>(QWebEnginePage::view());
}

void WebPage::hideUnwantedElements() {
  if (!qApp->web()->adBlock()->isEnabled()) {
    return;
  }

  auto css = qApp->web()->adBlock()->elementHidingRulesForDomain(url());

  if (!css.isEmpty()) {
    auto js = qApp->web()->adBlock()->generateJsForElementHiding(css);

    runJavaScript(js);
    qDebugNN << LOGSEC_ADBLOCK << "Running domain-specific JS for element hiding rules.";
  }
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame) {
  const RootItem* root = view()->root();

  if (is_main_frame) {
    auto blocked = qApp->web()->adBlock()->block(AdblockRequestInfo(url));

    if (blocked.m_blocked) {
      // This website is entirely blocked.
      setHtml(qApp->skins()->adBlockedPage(url.toString(), blocked.m_blockedByFilter),
              QUrl::fromUserInput(INTERNAL_URL_ADBLOCKED));
      return false;
    }
  }

  if (url.toString().startsWith(INTERNAL_URL_PASSATTACHMENT) &&
      root != nullptr &&
      root->getParentServiceRoot()->downloadAttachmentOnMyOwn(url)) {
    return false;
  }

  /*if (url.host() == INTERNAL_URL_MESSAGE_HOST) {
     setHtml(view()->messageContents(), QUrl(INTERNAL_URL_MESSAGE));
     return true;
     }
     else {*/
  return QWebEnginePage::acceptNavigationRequest(url, type, is_main_frame);

  //}
}

void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message,
                                       int line_number, const QString& source_id) {
  Q_UNUSED(level)

  qWarningNN << LOGSEC_JS << message << QSL(" (source: %1:%2)").arg(source_id, QString::number(line_number));
}
