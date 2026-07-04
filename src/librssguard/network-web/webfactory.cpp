// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webfactory.h"

#include "3rd-party/gumbo/src/gumbo.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/settings.h"
#include "network-web/cookiejar.h"
#include "qt-publicsuffix/publicsuffix.h"
#include "services/abstract/serviceroot.h"

#include <utility>

#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QProcess>
#include <QTcpSocket>
#include <QUrl>

namespace {
  QString pacStringLiteral(const QString& value) {
    QString escaped;
    escaped.reserve(value.size() + 2);
    escaped.append(QL1C('"'));

    for (const QChar& chr : value) {
      switch (chr.unicode()) {
        case '\\':
          escaped.append(QSL("\\\\"));
          break;

        case '"':
          escaped.append(QSL("\\\""));
          break;

        case '\n':
          escaped.append(QSL("\\n"));
          break;

        case '\r':
          escaped.append(QSL("\\r"));
          break;

        case '\t':
          escaped.append(QSL("\\t"));
          break;

        default:
          escaped.append(chr);
          break;
      }
    }

    escaped.append(QL1C('"'));
    return escaped;
  }

  QString pacProxyString(const QNetworkProxy& proxy) {
    switch (proxy.type()) {
      case QNetworkProxy::ProxyType::Socks5Proxy:
        return QSL("SOCKS5 %1:%2").arg(proxy.hostName(), QString::number(proxy.port()));

      case QNetworkProxy::ProxyType::HttpProxy:
      case QNetworkProxy::ProxyType::HttpCachingProxy:
      case QNetworkProxy::ProxyType::FtpCachingProxy:
        return QSL("PROXY %1:%2").arg(proxy.hostName(), QString::number(proxy.port()));

      case QNetworkProxy::ProxyType::NoProxy:
      case QNetworkProxy::ProxyType::DefaultProxy:
      default:
        return QSL("DIRECT");
    }
  }

  bool isExplicitProxy(const QNetworkProxy& proxy) {
    return proxy.type() != QNetworkProxy::ProxyType::DefaultProxy && proxy.type() != QNetworkProxy::ProxyType::NoProxy;
  }

  int hostSpecificity(const QString& host) {
    return host.count(QL1C('.'));
  }

  QString normalizedPacHost(QString host) {
    host = host.trimmed();

    while (host.endsWith(QL1C('.'))) {
      host.chop(1);
    }

    QString normalized_host = QString::fromLatin1(QUrl::toAce(host)).toLower();

    if (normalized_host.isEmpty()) {
      normalized_host = host.toLower();
    }

    if (normalized_host.startsWith(QSL("www."))) {
      normalized_host.remove(0, 4);
    }

    return normalized_host;
  }

  QStringList pacRuleHostsForHost(const QString& host) {
    QStringList rule_hosts;
    const QString normalized_host = normalizedPacHost(host);
    const QString registrable_host = normalizedPacHost(PublicSuffix::registrableDomain(host));

    if (!normalized_host.isEmpty()) {
      rule_hosts << normalized_host;
    }

    if (!registrable_host.isEmpty() && registrable_host != normalized_host) {
      rule_hosts << registrable_host;
    }

    return rule_hosts;
  }

  void collectHyperlinksFromGumboNode(GumboNode* node, const QUrl& base_url, QStringList& hyperlinks) {
    if (node == nullptr) {
      return;
    }

    if (node->type == GUMBO_NODE_DOCUMENT) {
      GumboVector* children = &node->v.document.children;

      for (unsigned int i = 0; i < children->length; i++) {
        collectHyperlinksFromGumboNode(static_cast<GumboNode*>(children->data[i]), base_url, hyperlinks);
      }

      return;
    }

    if (node->type != GUMBO_NODE_ELEMENT) {
      return;
    }

    GumboElement& element = node->v.element;

    if (element.tag == GUMBO_TAG_A || element.tag == GUMBO_TAG_AREA) {
      GumboAttribute* href = gumbo_get_attribute(&element.attributes, "href");

      if (href != nullptr && href->value != nullptr) {
        const QString raw_href = QString::fromUtf8(href->value).trimmed();

        if (!raw_href.isEmpty()) {
          const QUrl raw_url(raw_href);
          const QUrl resolved_url = base_url.resolved(raw_url);
          const QString resolved_url_string = resolved_url.toString();

          if (resolved_url.isValid() && !resolved_url_string.isEmpty() && !hyperlinks.contains(resolved_url_string)) {
            hyperlinks << resolved_url_string;
          }
        }
      }
    }

    GumboVector* children = &element.children;

    for (unsigned int i = 0; i < children->length; i++) {
      collectHyperlinksFromGumboNode(static_cast<GumboNode*>(children->data[i]), base_url, hyperlinks);
    }
  }
} // namespace

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "gui/webviewers/qtwebengine/geminischemehandler.h"

#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>

#if QT_VERSION_MAJOR < 6
#include <QWebEngineDownloadItem>
#else
#include <QWebEngineDownloadRequest>
#endif
#endif

WebFactory::WebFactory(QObject* parent)
  : QObject(parent)
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    ,
    m_geminiHandler(new GeminiSchemeHandler(this))
#endif
    ,
    m_customUserAgent(QString()) {
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  initializeWebEngineProfile();
  initializeWebEngineAttributeActions();
#endif

  m_cookieJar = new CookieJar(this);
}

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
void WebFactory::initializeWebEngineAttributeActions() {
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("JS enabled"),
                                                            QWebEngineSettings::WebAttribute::JavascriptEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("JS can open popup windows"),
                                                            QWebEngineSettings::WebAttribute::JavascriptCanOpenWindows);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("JS can access clipboard"),
                                  QWebEngineSettings::WebAttribute::JavascriptCanAccessClipboard);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Hyperlinks can get focus"),
                                  QWebEngineSettings::WebAttribute::LinksIncludedInFocusChain);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Local storage enabled"),
                                                            QWebEngineSettings::WebAttribute::LocalStorageEnabled);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Local content can access remote URLs"),
                                  QWebEngineSettings::WebAttribute::LocalContentCanAccessRemoteUrls);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("XSS auditing enabled"),
                                                            QWebEngineSettings::WebAttribute::XSSAuditingEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Spatial navigation enabled"),
                                                            QWebEngineSettings::WebAttribute::SpatialNavigationEnabled);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Local content can access local files"),
                                  QWebEngineSettings::WebAttribute::LocalContentCanAccessFileUrls);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Hyperlink auditing enabled"),
                                                            QWebEngineSettings::WebAttribute::HyperlinkAuditingEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Animate scrolling"),
                                                            QWebEngineSettings::WebAttribute::ScrollAnimatorEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Error pages enabled"),
                                                            QWebEngineSettings::WebAttribute::ErrorPageEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Plugins enabled"),
                                                            QWebEngineSettings::WebAttribute::PluginsEnabled);

#if !defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Screen capture enabled"),
                                                            QWebEngineSettings::WebAttribute::ScreenCaptureEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("WebGL enabled"),
                                                            QWebEngineSettings::WebAttribute::WebGLEnabled);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Accelerate 2D canvas"),
                                  QWebEngineSettings::WebAttribute::Accelerated2dCanvasEnabled);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Print element backgrounds"),
                                                            QWebEngineSettings::WebAttribute::PrintElementBackgrounds);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Allow running insecure content"),
                                  QWebEngineSettings::WebAttribute::AllowRunningInsecureContent);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Allow geolocation on insecure origins"),
                                  QWebEngineSettings::WebAttribute::AllowGeolocationOnInsecureOrigins);
#endif

  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("JS can activate windows"),
                                  QWebEngineSettings::WebAttribute::AllowWindowActivationFromJavaScript);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Show scrollbars"),
                                                            QWebEngineSettings::WebAttribute::ShowScrollBars);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("Media playback with gestures"),
                                  QWebEngineSettings::WebAttribute::PlaybackRequiresUserGesture);
  m_webEngineAttributeActions
    << createEngineSettingsAction(this,
                                  tr("WebRTC uses only public interfaces"),
                                  QWebEngineSettings::WebAttribute::WebRTCPublicInterfacesOnly);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("JS can paste from clipboard"),
                                                            QWebEngineSettings::WebAttribute::JavascriptCanPaste);
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("DNS prefetch enabled"),
                                                            QWebEngineSettings::WebAttribute::DnsPrefetchEnabled);

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("PDF viewer enabled"),
                                                            QWebEngineSettings::WebAttribute::PdfViewerEnabled);
#endif

#if QT_VERSION >= 0x060700 // Qt >= 6.7.0
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Force dark mode"),
                                                            QWebEngineSettings::WebAttribute::ForceDarkMode);
#endif

#if QT_VERSION >= 0x060900 // Qt >= 6.9.0
  m_webEngineAttributeActions << createEngineSettingsAction(this,
                                                            tr("Printing - print headers/footers."),
                                                            QWebEngineSettings::WebAttribute::PrintHeaderAndFooter);
#endif
}

void WebFactory::updateWebEngineProfileSettings() {
  QString cache_cache = webCacheFolder() + QDir::separator() + QSL("cache");

  if (!QDir().mkpath(cache_cache)) {
    qCriticalNN << LOGSEC_NETWORK << "Failed to create web cache folder" << QUOTE_W_SPACE_DOT(cache_cache);
  }
  else {
    m_webEngineProfile->setCachePath(webCacheFolder());
  }

  QString cache_pers = webCacheFolder() + QDir::separator() + QSL("storage");

  if (!QDir().mkpath(cache_pers)) {
    qCriticalNN << LOGSEC_NETWORK << "Failed to create web storage folder" << QUOTE_W_SPACE_DOT(cache_cache);
  }
  else {
    m_webEngineProfile->setPersistentStoragePath(cache_pers);
  }

  m_webEngineProfile->setHttpAcceptLanguage(qApp->localization()->loadedLocale().name().replace(QL1C('_'), QL1C('-')));

  auto custom_ua = qApp->web()->customUserAgent();

  if (custom_ua.isEmpty()) {
    m_webEngineProfile->setHttpUserAgent(QString::fromLocal8Bit(HTTP_COMPLETE_USERAGENT));
  }
  else {
    m_webEngineProfile->setHttpUserAgent(custom_ua);
  }
}

QWebEngineProfile* WebFactory::webEngineProfile() const {
  return m_webEngineProfile;
}

void WebFactory::onClearHttpCacheCompleted() {
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       GuiMessage(tr("Web cache cleared"),
                                  tr("Web cache was cleared. List of visited links was cleared too.")),
                       GuiMessageDestination(true, true, true));
}

#if QT_VERSION_MAJOR < 6
void WebFactory::onDownloadRequested(QWebEngineDownloadItem* download) {
#else
void WebFactory::onDownloadRequested(QWebEngineDownloadRequest* download) {
#endif
  if (download->isSavePageDownload() ||
      download->mimeType().contains(QSL("pdf"), Qt::CaseSensitivity::CaseInsensitive)) {
    download->accept();
  }
  else {
    QString url = download->url().toString();

    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("File download"),
                                    tr("Download of file '%1' was offered.").arg(download->downloadFileName())),
                         GuiMessageDestination(true, true, false),
                         GuiAction(tr("Copy file URL"), m_webEngineProfile, [url]() {
                           QGuiApplication::clipboard()->setText(url);
                         }));

    download->cancel();
  }
}

void WebFactory::initializeWebEngineProfile() {
  m_webEngineProfile = new QWebEngineProfile(QSL(APP_LOW_NAME), this);
  m_webEngineProfile->installUrlSchemeHandler("gemini", m_geminiHandler);

#if QT_VERSION_MAJOR >= 6
  connect(m_webEngineProfile,
          &QWebEngineProfile::clearHttpCacheCompleted,
          this,
          &WebFactory::onClearHttpCacheCompleted);
#endif

  connect(m_webEngineProfile, &QWebEngineProfile::downloadRequested, this, &WebFactory::onDownloadRequested);
}

bool WebFactory::isByDefaultDisabledWebEngineAttribute(QWebEngineSettings::WebAttribute web_attribute) {
  static QList<QWebEngineSettings::WebAttribute> attrs = {QWebEngineSettings::WebAttribute::JavascriptCanAccessClipboard
#if QT_VERSION >= 0x060700 // Qt >= 6.7.0
                                                          ,
                                                          QWebEngineSettings::WebAttribute::ForceDarkMode
#endif
  };

  return attrs.contains(web_attribute);
}

QAction* WebFactory::createEngineSettingsAction(QObject* parent,
                                                const QString& title,
                                                QWebEngineSettings::WebAttribute web_attribute) {
  auto* act = new QAction(title, parent);
  auto default_enabled = !isByDefaultDisabledWebEngineAttribute(web_attribute);
  auto enabled =
    qApp->settings()->value(WebEngineAttributes::ID, QString::number(int(web_attribute)), default_enabled).toBool();

  act->setData(int(web_attribute));
  act->setCheckable(true);
  act->setChecked(enabled);

  m_webEngineProfile->settings()->setAttribute(web_attribute, enabled);
  connect(act, &QAction::toggled, this, &WebFactory::onWebEngineAttributeChanged);
  return act;
}

void WebFactory::onWebEngineAttributeChanged(bool enabled) {
  const QAction* const act = qobject_cast<QAction*>(sender());

  QWebEngineSettings::WebAttribute attribute = QWebEngineSettings::WebAttribute(act->data().toInt());

  qApp->settings()->setValue(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), enabled);
  m_webEngineProfile->settings()->setAttribute(attribute, act->isChecked());
}

void WebFactory::generatePacAndStartServer(const QList<ServiceRoot*>& accounts) {
  // We obtain all proxies.
  QHash<QString, QNetworkProxy> proxies_per_host;

  if (accounts.size() == 1) {
    QNetworkProxy account_proxy = accounts.first()->networkProxy();

    if (isExplicitProxy(account_proxy)) {
      // We use this as fallback.
      proxies_per_host.insert(QString(), account_proxy);
    }
  }

  const bool has_proxy_fallback = proxies_per_host.contains(QString());

  for (ServiceRoot* acc : accounts) {
    QNetworkProxy account_proxy = acc->networkProxy();

    for (Feed* feed : acc->getSubTreeFeeds()) {
      QStringList feed_hosts = pacRuleHostsForHost(QUrl(feed->source()).host());

      // We add manually defined domains, which should be driven to the given proxy.
      //
      // This is particularly useful when some resources of the page like images or CSS
      // are located on 3rd-party CDN with totally different domain.
      feed_hosts.append(feed->proxyExtraDomains());

      if (feed_hosts.isEmpty()) {
        continue;
      }

      QNetworkProxy feed_proxy = acc->networkProxyForItem(feed);
      bool should_insert_rule = false;

      if (has_proxy_fallback) {
        // Single account with account-level proxy fallback:
        // feeds with a different explicit proxy or with proxying disabled need host rules.
        const bool disables_fallback = feed_proxy.type() == QNetworkProxy::ProxyType::NoProxy;
        const bool uses_different_proxy = isExplicitProxy(feed_proxy) && feed_proxy != account_proxy;

        should_insert_rule = disables_fallback || uses_different_proxy;
      }
      else {
        // No global PAC fallback: every explicit account/feed proxy needs its own host rule.
        should_insert_rule = isExplicitProxy(feed_proxy);
      }

      if (!should_insert_rule) {
        continue;
      }

      for (const QString& feed_host : std::as_const(feed_hosts)) {
        if (proxies_per_host.contains(feed_host)) {
          if (proxies_per_host.value(feed_host) != feed_proxy) {
            qWarningNN << LOGSEC_NETWORK << "Host" << QUOTE_W_SPACE(feed_host)
                       << "already has a different proxy rule in PAC file. Keeping the first one.";
          }

          continue;
        }

        proxies_per_host.insert(feed_host, feed_proxy);
      }
    }
  }

  // We generate PAC file which is saved into %data% folder.
  const QByteArray pac_data = generatePacFile(proxies_per_host);

  qDebugNN << LOGSEC_NETWORK << "Generated PAC file with" << QUOTE_W_SPACE(proxies_per_host.size())
           << "proxy rules and size" << QUOTE_W_SPACE_DOT(pac_data.size());

  IOFactory::writeFile(proxiesPacFilePath(), pac_data);

  // We start local HTTP server which server the PAC file.
  m_pacServer.setListenAddressPort(QSL("http://localhost:%1").arg(PAC_SERVER_PORT), true);
}

QByteArray WebFactory::generatePacFile(const QHash<QString, QNetworkProxy>& proxies_per_host) {
  const QString fallback_proxy =
    pacProxyString(proxies_per_host.value(QString(), QNetworkProxy::ProxyType::DefaultProxy));
  const int fallback_rules = proxies_per_host.contains(QString()) ? 1 : 0;
  const int host_rules = proxies_per_host.size() - fallback_rules;
  QString pac;

  pac.reserve(1024 + (proxies_per_host.size() * 96));

  pac += QSL("// This PAC file was generated by RSS Guard.\n");
  pac += QSL("// Generated at: %1 UTC.\n").arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
  pac += QSL("// Host-specific proxy rules: %1.\n").arg(QString::number(host_rules));
  pac += QSL("// Hosts are matched without leading 'www.' and include subdomains.\n");
  pac += QSL("// Registrable-domain rules are derived from the Public Suffix List.\n");
  pac += QSL("// Fallback proxy: %1.\n").arg(fallback_proxy);
  pac += QSL("// Do not edit this file manually; it is regenerated from account/feed proxy settings.\n\n");
  pac += QSL("function hostMatches(host, ruleHost) {\n");
  pac += QSL("  return host == ruleHost || dnsDomainIs(host, \".\" + ruleHost);\n");
  pac += QSL("}\n\n");
  pac += QSL("function FindProxyForURL(url, host) {\n");
  pac += QSL("  host = host.toLowerCase();\n\n");

  QStringList hosts = proxies_per_host.keys();

  hosts.removeAll(QString());

  // Subdomain rules must go first, otherwise a base-domain rule would catch them via dnsDomainIs().
  std::sort(hosts.begin(), hosts.end(), [](const QString& lhs, const QString& rhs) {
    const int lhs_specificity = hostSpecificity(lhs);
    const int rhs_specificity = hostSpecificity(rhs);

    if (lhs_specificity != rhs_specificity) {
      return lhs_specificity > rhs_specificity;
    }

    if (lhs.size() != rhs.size()) {
      return lhs.size() > rhs.size();
    }

    return lhs < rhs;
  });

  for (const QString& host : std::as_const(hosts)) {
    pac += QSL("  if (hostMatches(host, %1)) {\n").arg(pacStringLiteral(host.toLower()));
    pac += QSL("    return %1;\n").arg(pacStringLiteral(pacProxyString(proxies_per_host.value(host))));
    pac += QSL("  }\n\n");
  }

  pac += QSL("  return %1;\n").arg(pacStringLiteral(fallback_proxy));
  pac += QSL("}\n");

  return pac.toUtf8();
}

QString WebFactory::proxiesPacFilePath() {
  static QString path = webCacheFolder() + QDir::separator() + QSL(PAC_SERVER_FILE);
  return path;
}

QString WebFactory::injectPacIntoChromiumFlags(const QString& cli_flags, const QString& user_flags) {
  if (cli_flags.contains(QSL("proxy-pac-url")) || user_flags.contains(QSL("proxy-pac-url"))) {
    return cli_flags + QSL(" ") + user_flags;
  }
  else {
    return QSL("--proxy-pac-url=http://127.0.0.1:%1/%2").arg(QString::number(PAC_SERVER_PORT), QSL(PAC_SERVER_FILE)) +
           QSL(" ") + cli_flags + QSL(" ") + user_flags;
  }
}

void PacServer::answerClient(QTcpSocket* socket, const HttpRequest& request) {
  QByteArray http_payload;
  QByteArray reply_message;
  int status_code = 200;
  QString status_text = QSL("OK");
  const QString request_path = request.m_url.path();

  qDebugNN << LOGSEC_NETWORK << "PAC server received request for" << QUOTE_W_SPACE(request_path) << "with method"
           << QUOTE_W_SPACE_DOT(int(request.m_method));

  try {
    if (request.m_method != HttpRequest::Method::Get && request.m_method != HttpRequest::Method::Head) {
      status_code = 405;
      status_text = QSL("Method Not Allowed");
      http_payload = QByteArrayLiteral("Method not allowed.");
      reply_message = QSL("HTTP/1.1 405 Method Not Allowed\r\n"
                          "Allow: GET, HEAD\r\n"
                          "Connection: close\r\n"
                          "Content-Type: text/plain; charset=utf-8\r\n"
                          "Content-Length: %1\r\n"
                          "\r\n")
                        .arg(QString::number(http_payload.size()))
                        .toUtf8();

      qWarningNN << LOGSEC_NETWORK << "PAC server rejected request with unsupported method"
                 << QUOTE_W_SPACE_DOT(int(request.m_method));
    }
    else if (request_path != QSL("/") && request_path != QSL("/%1").arg(QSL(PAC_SERVER_FILE))) {
      status_code = 404;
      status_text = QSL("Not Found");
      http_payload = QByteArrayLiteral("Not found.");
      reply_message = QSL("HTTP/1.1 404 Not Found\r\n"
                          "Connection: close\r\n"
                          "Content-Type: text/plain; charset=utf-8\r\n"
                          "Content-Length: %1\r\n"
                          "\r\n")
                        .arg(QString::number(http_payload.size()))
                        .toUtf8();

      qWarningNN << LOGSEC_NETWORK << "PAC server rejected request for unknown path" << QUOTE_W_SPACE_DOT(request_path);
    }
    else {
      http_payload = IOFactory::readFile(WebFactory::proxiesPacFilePath());
      reply_message = QSL("HTTP/1.1 200 OK\r\n"
                          "Connection: close\r\n"
                          "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                          "Content-Type: application/x-ns-proxy-autoconfig; charset=utf-8\r\n"
                          "Content-Length: %1\r\n"
                          "\r\n")
                        .arg(QString::number(http_payload.size()))
                        .toUtf8();

      qDebugNN << LOGSEC_NETWORK << "PAC server loaded PAC file from" << QUOTE_W_SPACE(WebFactory::proxiesPacFilePath())
               << "with size" << NONQUOTE_W_SPACE_DOT(http_payload.size());
    }
  }
  catch (const ApplicationException& ex) {
    status_code = 500;
    status_text = QSL("Internal Server Error");
    http_payload = ex.message().toUtf8();
    reply_message = QSL("HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain; charset=utf-8\r\n"
                        "Content-Length: %1\r\n"
                        "\r\n")
                      .arg(QString::number(http_payload.size()))
                      .toUtf8();

    qWarningNN << LOGSEC_NETWORK << "PAC server failed to read PAC file:" << QUOTE_W_SPACE_DOT(ex.message());
  }

  if (request.m_method != HttpRequest::Method::Head) {
    reply_message.append(http_payload);
  }

  socket->write(reply_message);
  socket->disconnectFromHost();

  qDebugNN << LOGSEC_NETWORK << "PAC server answered" << QUOTE_W_SPACE(socket->peerAddress().toString())
           << "with status" << NONQUOTE_W_SPACE(status_code) << QUOTE_W_SPACE(status_text) << "and payload size"
           << NONQUOTE_W_SPACE_DOT(http_payload.size());
}

QList<QAction*> WebFactory::webEngineAttributeActions() const {
  return m_webEngineAttributeActions;
}
#endif

WebFactory::~WebFactory() {
  if (m_cookieJar != nullptr && m_cookieJar->parent() == nullptr) {
    m_cookieJar->deleteLater();
  }
}

bool WebFactory::sendMessageViaEmail(const Message& message) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool()) {
    const QString browser =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString();
    const QString arguments =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString();
    const QStringList tokenized_arguments =
      TextFactory::tokenizeProcessArguments(arguments.arg(message.m_title, stripTags(message.m_contents)));

    return IOFactory::startProcessDetached(browser, tokenized_arguments);
  }
  else {
    // Send it via mailto protocol.
    // NOTE: http://en.wikipedia.org/wiki/Mailto
    return QDesktopServices::openUrl(QSL("mailto:?subject=%1&body=%2")
                                       .arg(QString(QUrl::toPercentEncoding(message.m_title)),
                                            QString(QUrl::toPercentEncoding(stripTags(message.m_contents)))));
  }
}

bool WebFactory::openUrlInExternalBrowser(const QUrl& url) const {
  return openUrlInExternalBrowser(QList<QUrl>{url}, false, false);
}

bool WebFactory::openUrlInExternalBrowser(const QUrl& url, bool use_external_tools) const {
  return openUrlInExternalBrowser(QList<QUrl>{url}, use_external_tools, false);
}

bool WebFactory::openUrlInExternalBrowser(const QList<QUrl>& urls,
                                          bool use_external_tools,
                                          bool can_bring_forward_after) const {
  if (urls.isEmpty()) {
    return true;
  }

  QStringList failed_urls;

  const QList<ExternalTool> tools = use_external_tools ? ExternalTool::toolsFromSettings() : QList<ExternalTool>();
  const bool custom_browser_enabled =
    qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool();
  const QString custom_browser =
    custom_browser_enabled
      ? qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString()
      : QString();
  const QString custom_browser_arguments =
    custom_browser_enabled
      ? qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString()
      : QString();
  QHash<QString, std::optional<ExternalTool>> external_tools_for_hosts;

  for (const QUrl& url : urls) {
    QString my_url = url.toString(QUrl::ComponentFormattingOption::FullyEncoded);

    qDebugNN << LOGSEC_NETWORK << "We are trying to open URL" << QUOTE_W_SPACE_DOT(my_url);

    if (use_external_tools && !url.host().isEmpty()) {
      const QString host = url.host().toLower();
      auto tool_for_domain = external_tools_for_hosts.value(host);

      if (!external_tools_for_hosts.contains(host)) {
        tool_for_domain = ExternalTool::toolForDomain(tools, host);
        external_tools_for_hosts.insert(host, tool_for_domain);
      }

      if (tool_for_domain.has_value()) {
        auto found_tool_for_domain = tool_for_domain.value();
        qDebugNN << LOGSEC_NETWORK << "Opening URL via external tool"
                 << QUOTE_W_SPACE_DOT(found_tool_for_domain.name());

        if (!found_tool_for_domain.run(my_url)) {
          qWarningNN << LOGSEC_NETWORK << "External tool failed" << QUOTE_W_SPACE_DOT(found_tool_for_domain.name());
        }
        else {
          continue;
        }
      }
      else {
        qWarningNN << LOGSEC_NETWORK
                   << "External tool valid for the given URL was not found. Falling back to external web browser.";
      }
    }

    bool opened = false;

    if (custom_browser_enabled) {
      const QString nice_args = custom_browser_arguments.arg(my_url);

      qDebugNN << LOGSEC_NETWORK << "Arguments for external browser:" << QUOTE_W_SPACE_DOT(nice_args);

      bool started = IOFactory::startProcessDetached(custom_browser, TextFactory::tokenizeProcessArguments(nice_args));

      if (!started) {
        qDebugNN << LOGSEC_NETWORK << "External web browser call failed.";
      }
      else {
        opened = true;
      }
    }
    else {
      opened = QDesktopServices::openUrl(my_url);
    }

    if (!opened) {
      failed_urls.append(my_url);
    }
  }

  if (!failed_urls.isEmpty()) {
    failed_urls.removeDuplicates();

    // We display GUI information that browser was not probably opened.
    MsgBox::show({},
                 QMessageBox::Icon::Critical,
                 tr("Navigate to website(s) manually"),
                 tr("%1 was unable to launch your web browser with the given URL, you need to open the "
                    "below website URLs in your web browser manually.")
                   .arg(QSL(APP_NAME)),
                 {},
                 failed_urls.join(QSL("\r\n")),
                 QMessageBox::StandardButton::Ok);
  }
  else if (can_bring_forward_after) {
    if (qApp->settings()
          ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
          .toBool()) {
      QTimer::singleShot(1000, this, []() {
        qApp->mainForm()->display();
      });
    }
  }

  return failed_urls.isEmpty();
}

QString WebFactory::stripTags(QString text) {
  static QRegularExpression reg_tags(QSL("<[^>]*>"));

  return text.remove(reg_tags);
}

QString WebFactory::urlToTld(const QUrl& url) {
  const QString host = url.host();

  if (host.isEmpty()) {
    return host;
  }

  const QString registrable_domain = PublicSuffix::registrableDomain(host);

  return registrable_domain.isEmpty() ? host : registrable_domain;
}

QString WebFactory::webCacheFolder() {
  QString cache_folder = qApp->userDataFolder() + QDir::separator() + QSL("web");

  return cache_folder;
}

QStringList WebFactory::extractAllHyperlinks(const QUrl& base_url, const QByteArray& html_data) {
  QStringList hyperlinks;

  if (html_data.isEmpty()) {
    return hyperlinks;
  }

  GumboOutput* output =
    gumbo_parse_with_options(&kGumboDefaultOptions, html_data.constData(), static_cast<size_t>(html_data.size()));

  if (output == nullptr) {
    return hyperlinks;
  }

  collectHyperlinksFromGumboNode(output->root, base_url, hyperlinks);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  qDebugNN << hyperlinks;

  return hyperlinks;
}

QString WebFactory::unescapeHtml(const QString& html) {
  if (html.isEmpty()) {
    return html;
  }

  static QMap<QString, char16_t> entities = generateUnescapes();

  QString output;
  output.reserve(html.size());

  // Traverse input HTML string and replace named/number entities.
  for (int pos = 0; pos < html.size();) {
    const QChar first = html.at(pos);

    if (first == QChar('&')) {
      // We need to find ending ';'.
      int pos_end = -1;

      // We're finding end of entity, but also we limit searching window to 10 characters.
      for (int pos_find = pos; pos_find <= pos + 10 && pos_find < html.size(); pos_find++) {
        if (html.at(pos_find) == QChar(';')) {
          // We found end of the entity.
          pos_end = pos_find;
          break;
        }
      }

      if (pos_end + 1 > pos) {
        // OK, we have entity.
        if (html.at(pos + 1) == QChar('#')) {
          // We have numbered entity.
          uint number;
          QString number_str;

          if (html.at(pos + 2) == QChar('x')) {
            // base-16 number.
            number_str = html.mid(pos + 3, pos_end - pos - 3);
            number = number_str.toUInt(nullptr, 16);
          }
          else {
            // base-10 number.
            number_str = html.mid(pos + 2, pos_end - pos - 2);
            number = number_str.toUInt();
          }

          if (number > 0U) {
            output.append(QString::fromUcs4((const char32_t*)&number, 1));
          }
          else {
            // Failed to convert to number, leave intact.
            output.append(html.mid(pos, pos_end - pos + 1));
          }

          pos = pos_end + 1;
          continue;
        }
        else {
          // We have named entity.
          auto entity_name = html.mid(pos + 1, pos_end - pos - 1);

          if (entities.contains(entity_name)) {
            // Entity found, proceed.
            output.append(entities.value(entity_name));
          }
          else {
            // Entity NOT found, leave intact.
            output.append('&');
            output.append(entity_name);
            output.append(';');
          }

          pos = pos_end + 1;
          continue;
        }
      }
    }

    // No entity, normally append and continue.
    output.append(first);
    pos++;
  }

  /*
     qDebugNN << LOGSEC_CORE
           << "Unescaped string" << QUOTE_W_SPACE(html)
           << "to" << QUOTE_W_SPACE_DOT(output);
   */

  return output;
}

QString WebFactory::processFeedUriScheme(const QString& url) {
  if (url.startsWith(QSL(URI_SCHEME_FEED))) {
    return QSL(URI_SCHEME_HTTPS) + url.mid(QSL(URI_SCHEME_FEED).size());
  }
  else if (url.startsWith(QSL(URI_SCHEME_FEED_SHORT))) {
    return url.mid(QSL(URI_SCHEME_FEED_SHORT).size());
  }
  else {
    return url;
  }
}

void WebFactory::updateProxy() {
  const QNetworkProxy::ProxyType selected_proxy_type =
    static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy), SETTING(Proxy::Type)).toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    qDebugNN << LOGSEC_NETWORK << "Disabling application-wide proxy completely.";

    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::ProxyType::NoProxy);
  }
  else if (selected_proxy_type == QNetworkProxy::ProxyType::DefaultProxy) {
    qDebugNN << LOGSEC_NETWORK << "Using application-wide proxy to be system's default proxy.";
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  }
  else {
    const Settings* settings = qApp->settings();
    QNetworkProxy new_proxy;

    // Custom proxy is selected, set it up.
    new_proxy.setType(selected_proxy_type);
    new_proxy.setHostName(settings->value(GROUP(Proxy), SETTING(Proxy::Host)).toString());
    new_proxy.setPort(quint16(settings->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt()));
    new_proxy.setUser(settings->value(GROUP(Proxy), SETTING(Proxy::Username)).toString());
    new_proxy.setPassword(settings->password(GROUP(Proxy), SETTING(Proxy::Password)).toString());

    qWarningNN << LOGSEC_NETWORK
               << "Activating application-wide custom proxy, address:" << QUOTE_W_SPACE_COMMA(new_proxy.hostName())
               << " type:" << QUOTE_W_SPACE_DOT(new_proxy.type());

    QNetworkProxy::setApplicationProxy(new_proxy);
  }
}

QMap<QString, char16_t> WebFactory::generateUnescapes() {
  QMap<QString, char16_t> res;
  res[QSL("AElig")] = 0x00c6;
  res[QSL("AMP")] = 38;
  res[QSL("Aacute")] = 0x00c1;
  res[QSL("Acirc")] = 0x00c2;
  res[QSL("Agrave")] = 0x00c0;
  res[QSL("Alpha")] = 0x0391;
  res[QSL("Aring")] = 0x00c5;
  res[QSL("Atilde")] = 0x00c3;
  res[QSL("Auml")] = 0x00c4;
  res[QSL("Beta")] = 0x0392;
  res[QSL("Ccedil")] = 0x00c7;
  res[QSL("Chi")] = 0x03a7;
  res[QSL("Dagger")] = 0x2021;
  res[QSL("Delta")] = 0x0394;
  res[QSL("ETH")] = 0x00d0;
  res[QSL("Eacute")] = 0x00c9;
  res[QSL("Ecirc")] = 0x00ca;
  res[QSL("Egrave")] = 0x00c8;
  res[QSL("Epsilon")] = 0x0395;
  res[QSL("Eta")] = 0x0397;
  res[QSL("Euml")] = 0x00cb;
  res[QSL("GT")] = 62;
  res[QSL("Gamma")] = 0x0393;
  res[QSL("Iacute")] = 0x00cd;
  res[QSL("Icirc")] = 0x00ce;
  res[QSL("Igrave")] = 0x00cc;
  res[QSL("Iota")] = 0x0399;
  res[QSL("Iuml")] = 0x00cf;
  res[QSL("Kappa")] = 0x039a;
  res[QSL("LT")] = 60;
  res[QSL("Lambda")] = 0x039b;
  res[QSL("Mu")] = 0x039c;
  res[QSL("Ntilde")] = 0x00d1;
  res[QSL("Nu")] = 0x039d;
  res[QSL("OElig")] = 0x0152;
  res[QSL("Oacute")] = 0x00d3;
  res[QSL("Ocirc")] = 0x00d4;
  res[QSL("Ograve")] = 0x00d2;
  res[QSL("Omega")] = 0x03a9;
  res[QSL("Omicron")] = 0x039f;
  res[QSL("Oslash")] = 0x00d8;
  res[QSL("Otilde")] = 0x00d5;
  res[QSL("Ouml")] = 0x00d6;
  res[QSL("Phi")] = 0x03a6;
  res[QSL("Pi")] = 0x03a0;
  res[QSL("Prime")] = 0x2033;
  res[QSL("Psi")] = 0x03a8;
  res[QSL("QUOT")] = 34;
  res[QSL("Rho")] = 0x03a1;
  res[QSL("Scaron")] = 0x0160;
  res[QSL("Sigma")] = 0x03a3;
  res[QSL("THORN")] = 0x00de;
  res[QSL("Tau")] = 0x03a4;
  res[QSL("Theta")] = 0x0398;
  res[QSL("Uacute")] = 0x00da;
  res[QSL("Ucirc")] = 0x00db;
  res[QSL("Ugrave")] = 0x00d9;
  res[QSL("Upsilon")] = 0x03a5;
  res[QSL("Uuml")] = 0x00dc;
  res[QSL("Xi")] = 0x039e;
  res[QSL("Yacute")] = 0x00dd;
  res[QSL("Yuml")] = 0x0178;
  res[QSL("Zeta")] = 0x0396;
  res[QSL("aacute")] = 0x00e1;
  res[QSL("acirc")] = 0x00e2;
  res[QSL("acute")] = 0x00b4;
  res[QSL("aelig")] = 0x00e6;
  res[QSL("agrave")] = 0x00e0;
  res[QSL("alefsym")] = 0x2135;
  res[QSL("alpha")] = 0x03b1;
  res[QSL("amp")] = 38;
  res[QSL("and")] = 0x22a5;
  res[QSL("ang")] = 0x2220;
  res[QSL("apos")] = 0x0027;
  res[QSL("aring")] = 0x00e5;
  res[QSL("asymp")] = 0x2248;
  res[QSL("atilde")] = 0x00e3;
  res[QSL("auml")] = 0x00e4;
  res[QSL("bdquo")] = 0x201e;
  res[QSL("beta")] = 0x03b2;
  res[QSL("brvbar")] = 0x00a6;
  res[QSL("bull")] = 0x2022;
  res[QSL("cap")] = 0x2229;
  res[QSL("ccedil")] = 0x00e7;
  res[QSL("cedil")] = 0x00b8;
  res[QSL("cent")] = 0x00a2;
  res[QSL("chi")] = 0x03c7;
  res[QSL("circ")] = 0x02c6;
  res[QSL("clubs")] = 0x2663;
  res[QSL("cong")] = 0x2245;
  res[QSL("copy")] = 0x00a9;
  res[QSL("crarr")] = 0x21b5;
  res[QSL("cup")] = 0x222a;
  res[QSL("curren")] = 0x00a4;
  res[QSL("dArr")] = 0x21d3;
  res[QSL("dagger")] = 0x2020;
  res[QSL("darr")] = 0x2193;
  res[QSL("deg")] = 0x00b0;
  res[QSL("delta")] = 0x03b4;
  res[QSL("diams")] = 0x2666;
  res[QSL("divide")] = 0x00f7;
  res[QSL("eacute")] = 0x00e9;
  res[QSL("ecirc")] = 0x00ea;
  res[QSL("egrave")] = 0x00e8;
  res[QSL("empty")] = 0x2205;
  res[QSL("emsp")] = 0x2003;
  res[QSL("ensp")] = 0x2002;
  res[QSL("epsilon")] = 0x03b5;
  res[QSL("equiv")] = 0x2261;
  res[QSL("eta")] = 0x03b7;
  res[QSL("eth")] = 0x00f0;
  res[QSL("euml")] = 0x00eb;
  res[QSL("euro")] = 0x20ac;
  res[QSL("exist")] = 0x2203;
  res[QSL("fnof")] = 0x0192;
  res[QSL("forall")] = 0x2200;
  res[QSL("frac12")] = 0x00bd;
  res[QSL("frac14")] = 0x00bc;
  res[QSL("frac34")] = 0x00be;
  res[QSL("frasl")] = 0x2044;
  res[QSL("gamma")] = 0x03b3;
  res[QSL("ge")] = 0x2265;
  res[QSL("gt")] = 62;
  res[QSL("hArr")] = 0x21d4;
  res[QSL("harr")] = 0x2194;
  res[QSL("hearts")] = 0x2665;
  res[QSL("hellip")] = 0x2026;
  res[QSL("iacute")] = 0x00ed;
  res[QSL("icirc")] = 0x00ee;
  res[QSL("iexcl")] = 0x00a1;
  res[QSL("igrave")] = 0x00ec;
  res[QSL("image")] = 0x2111;
  res[QSL("infin")] = 0x221e;
  res[QSL("int")] = 0x222b;
  res[QSL("iota")] = 0x03b9;
  res[QSL("iquest")] = 0x00bf;
  res[QSL("isin")] = 0x2208;
  res[QSL("iuml")] = 0x00ef;
  res[QSL("kappa")] = 0x03ba;
  res[QSL("lArr")] = 0x21d0;
  res[QSL("lambda")] = 0x03bb;
  res[QSL("lang")] = 0x2329;
  res[QSL("laquo")] = 0x00ab;
  res[QSL("larr")] = 0x2190;
  res[QSL("lceil")] = 0x2308;
  res[QSL("ldquo")] = 0x201c;
  res[QSL("le")] = 0x2264;
  res[QSL("lfloor")] = 0x230a;
  res[QSL("lowast")] = 0x2217;
  res[QSL("loz")] = 0x25ca;
  res[QSL("lrm")] = 0x200e;
  res[QSL("lsaquo")] = 0x2039;
  res[QSL("lsquo")] = 0x2018;
  res[QSL("lt")] = 60;
  res[QSL("macr")] = 0x00af;
  res[QSL("mdash")] = 0x2014;
  res[QSL("micro")] = 0x00b5;
  res[QSL("middot")] = 0x00b7;
  res[QSL("minus")] = 0x2212;
  res[QSL("mu")] = 0x03bc;
  res[QSL("nabla")] = 0x2207;
  res[QSL("nbsp")] = 0x00a0;
  res[QSL("ndash")] = 0x2013;
  res[QSL("ne")] = 0x2260;
  res[QSL("ni")] = 0x220b;
  res[QSL("not")] = 0x00ac;
  res[QSL("notin")] = 0x2209;
  res[QSL("nsub")] = 0x2284;
  res[QSL("ntilde")] = 0x00f1;
  res[QSL("nu")] = 0x03bd;
  res[QSL("oacute")] = 0x00f3;
  res[QSL("ocirc")] = 0x00f4;
  res[QSL("oelig")] = 0x0153;
  res[QSL("ograve")] = 0x00f2;
  res[QSL("oline")] = 0x203e;
  res[QSL("omega")] = 0x03c9;
  res[QSL("omicron")] = 0x03bf;
  res[QSL("oplus")] = 0x2295;
  res[QSL("or")] = 0x22a6;
  res[QSL("ordf")] = 0x00aa;
  res[QSL("ordm")] = 0x00ba;
  res[QSL("oslash")] = 0x00f8;
  res[QSL("otilde")] = 0x00f5;
  res[QSL("otimes")] = 0x2297;
  res[QSL("ouml")] = 0x00f6;
  res[QSL("para")] = 0x00b6;
  res[QSL("part")] = 0x2202;
  res[QSL("percnt")] = 0x0025;
  res[QSL("permil")] = 0x2030;
  res[QSL("perp")] = 0x22a5;
  res[QSL("phi")] = 0x03c6;
  res[QSL("pi")] = 0x03c0;
  res[QSL("piv")] = 0x03d6;
  res[QSL("plusmn")] = 0x00b1;
  res[QSL("pound")] = 0x00a3;
  res[QSL("prime")] = 0x2032;
  res[QSL("prod")] = 0x220f;
  res[QSL("prop")] = 0x221d;
  res[QSL("psi")] = 0x03c8;
  res[QSL("quot")] = 34;
  res[QSL("rArr")] = 0x21d2;
  res[QSL("radic")] = 0x221a;
  res[QSL("rang")] = 0x232a;
  res[QSL("raquo")] = 0x00bb;
  res[QSL("rarr")] = 0x2192;
  res[QSL("rceil")] = 0x2309;
  res[QSL("rdquo")] = 0x201d;
  res[QSL("real")] = 0x211c;
  res[QSL("reg")] = 0x00ae;
  res[QSL("rfloor")] = 0x230b;
  res[QSL("rho")] = 0x03c1;
  res[QSL("rlm")] = 0x200f;
  res[QSL("rsaquo")] = 0x203a;
  res[QSL("rsquo")] = 0x2019;
  res[QSL("sbquo")] = 0x201a;
  res[QSL("scaron")] = 0x0161;
  res[QSL("sdot")] = 0x22c5;
  res[QSL("sect")] = 0x00a7;
  res[QSL("shy")] = 0x00ad;
  res[QSL("sigma")] = 0x03c3;
  res[QSL("sigmaf")] = 0x03c2;
  res[QSL("sim")] = 0x223c;
  res[QSL("spades")] = 0x2660;
  res[QSL("sub")] = 0x2282;
  res[QSL("sube")] = 0x2286;
  res[QSL("sum")] = 0x2211;
  res[QSL("sup")] = 0x2283;
  res[QSL("sup1")] = 0x00b9;
  res[QSL("sup2")] = 0x00b2;
  res[QSL("sup3")] = 0x00b3;
  res[QSL("supe")] = 0x2287;
  res[QSL("szlig")] = 0x00df;
  res[QSL("tau")] = 0x03c4;
  res[QSL("there4")] = 0x2234;
  res[QSL("theta")] = 0x03b8;
  res[QSL("thetasym")] = 0x03d1;
  res[QSL("thinsp")] = 0x2009;
  res[QSL("thorn")] = 0x00fe;
  res[QSL("tilde")] = 0x02dc;
  res[QSL("times")] = 0x00d7;
  res[QSL("trade")] = 0x2122;
  res[QSL("uArr")] = 0x21d1;
  res[QSL("uacute")] = 0x00fa;
  res[QSL("uarr")] = 0x2191;
  res[QSL("ucirc")] = 0x00fb;
  res[QSL("ugrave")] = 0x00f9;
  res[QSL("uml")] = 0x00a8;
  res[QSL("upsih")] = 0x03d2;
  res[QSL("upsilon")] = 0x03c5;
  res[QSL("uuml")] = 0x00fc;
  res[QSL("weierp")] = 0x2118;
  res[QSL("xi")] = 0x03be;
  res[QSL("yacute")] = 0x00fd;
  res[QSL("yen")] = 0x00a5;
  res[QSL("yuml")] = 0x00ff;
  res[QSL("zeta")] = 0x03b6;
  res[QSL("zwj")] = 0x200d;
  res[QSL("zwnj")] = 0x200c;

  return res;
}

CookieJar* WebFactory::cookieJar() const {
  return m_cookieJar;
}

QString WebFactory::customUserAgent() const {
  return m_customUserAgent;
}

void WebFactory::setCustomUserAgent(const QString& user_agent) {
  m_customUserAgent = user_agent;
}
