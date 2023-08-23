// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webengine/webenginepage.h"

#include "definitions/definitions.h"
#include "gui/webviewers/webengine/webengineviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/webfactory.h"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEngineScript>

WebEnginePage::WebEnginePage(QObject* parent) : QWebEnginePage(qApp->web()->engineProfile(), parent) {
  setBackgroundColor(Qt::GlobalColor::transparent);

  connect(this, &QWebEnginePage::loadFinished, this, &WebEnginePage::hideUnwantedElements);
}

WebEngineViewer* WebEnginePage::view() const {
#if QT_VERSION_MAJOR == 6
  return qobject_cast<WebEngineViewer*>(QWebEngineView::forPage(this));
#else
  return qobject_cast<WebEngineViewer*>(QWebEnginePage::view());
#endif
}

void WebEnginePage::hideUnwantedElements() {
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

bool WebEnginePage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame) {
  if (type == NavigationType::NavigationTypeLinkClicked) {
    bool open_externally_now =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::OpenLinksInExternalBrowserRightAway)).toBool();

    if (open_externally_now) {
      qApp->web()->openUrlInExternalBrowser(url.toString());
      return false;
    }
  }

  if (is_main_frame) {
    auto blocked = qApp->web()->adBlock()->block(AdblockRequestInfo(url));

    if (blocked.m_blocked) {
      // This website is entirely blocked.
      setHtml(qApp->skins()->adBlockedPage(url.toString(), blocked.m_blockedByFilter),
              QUrl::fromUserInput(QSL(INTERNAL_URL_ADBLOCKED)));
      return false;
    }
  }

  return QWebEnginePage::acceptNavigationRequest(url, type, is_main_frame);
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                             const QString& message,
                                             int line_number,
                                             const QString& source_id) {
  Q_UNUSED(level)

  qWarningNN << LOGSEC_JS << message << QSL(" (source: %1:%2)").arg(source_id, QString::number(line_number));
}
