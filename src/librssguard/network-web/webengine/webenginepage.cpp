// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webengine/webenginepage.h"

#include "definitions/definitions.h"
#include "gui/webviewers/webengine/webengineviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/webfactory.h"
#include "services/abstract/rootitem.h"
#include "services/abstract/serviceroot.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QBuffer>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

WebEnginePage::WebEnginePage(QObject* parent) : QWebEnginePage(qApp->web()->engineProfile(), parent) {
  setBackgroundColor(Qt::GlobalColor::transparent);

  std::string userStylesPath = qApp->settings()->pathName().toStdString() + "/user-styles.css";

  if (std::filesystem::exists(userStylesPath)) {
    std::ifstream file(userStylesPath);
    //read entire file
    std::stringstream buff;
    buff << file.rdbuf();

    //give the css to js in base64 to prevent escaping the string by putting a ', ", or ` in the css
    QByteArray bytes;
    QBuffer qbuff(&bytes);
    qbuff.open(QBuffer::ReadWrite);
    //convert std::stringstream to QBuffer
    qbuff.write(buff.str().c_str());
    qbuff.close();

    //arbitrary name
    const char* name = "rssguard-user-styles";

    QWebEngineScript script;
    QString s = QString::fromLatin1("(function() {"
                                    "    css = document.createElement('style');"
                                    "    css.type = 'text/css';"
                                    "    css.id = '%1';"
                                    "    document.head.appendChild(css);"
                                    "    css.innerText = atob('%2');"
                                    "})()").arg(name).arg(bytes.toBase64().data());
    script.setName(name);
    script.setSourceCode(s);
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    //doesn't need to run every frame
    script.setRunsOnSubFrames(false);
    script.setWorldId(QWebEngineScript::ApplicationWorld);
    scripts().insert(script);
  }

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
