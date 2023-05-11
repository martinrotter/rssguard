// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webfactory.h"

#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/cookiejar.h"
#include "network-web/readability.h"

#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

#if defined(USE_WEBENGINE)
#include "network-web/webengine/networkurlinterceptor.h"

#if QT_VERSION_MAJOR == 6
#include <QWebEngineDownloadRequest>
#else
#include <QWebEngineDownloadItem>
#endif
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineUrlScheme>
#endif

WebFactory::WebFactory(QObject* parent) : QObject(parent), m_customUserAgent(QString()) {
  m_adBlock = new AdBlockManager(this);

#if defined(USE_WEBENGINE)
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::DisableCache)).toBool()) {
    qWarningNN << LOGSEC_NETWORK << "Using off-the-record WebEngine profile.";

    m_engineProfile = new QWebEngineProfile(this);
  }
  else {
    m_engineProfile = new QWebEngineProfile(QSL(APP_LOW_NAME), this);
  }

  m_engineSettings = nullptr;
  m_urlInterceptor = new NetworkUrlInterceptor(this);
#endif

  m_cookieJar = new CookieJar(this);
  m_readability = new Readability(this);

#if defined(USE_WEBENGINE)
#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  m_engineProfile->setUrlRequestInterceptor(m_urlInterceptor);
#else
  m_engineProfile->setRequestInterceptor(m_urlInterceptor);
#endif
#endif
}

WebFactory::~WebFactory() {
#if defined(USE_WEBENGINE)
  if (m_engineSettings != nullptr && m_engineSettings->menu() != nullptr) {
    m_engineSettings->menu()->deleteLater();
  }
#endif

  /*if (m_cookieJar != nullptr) {
    m_cookieJar->deleteLater();
  }*/
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

bool WebFactory::openUrlInExternalBrowser(const QString& url) const {
  qDebugNN << LOGSEC_NETWORK << "We are trying to open URL" << QUOTE_W_SPACE_DOT(url);

  bool result = false;

  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
    const QString browser =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
    const QString arguments =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();
    const auto nice_args = arguments.arg(url);

    qDebugNN << LOGSEC_NETWORK << "Arguments for external browser:" << QUOTE_W_SPACE_DOT(nice_args);

    result = IOFactory::startProcessDetached(browser, TextFactory::tokenizeProcessArguments(nice_args));

    if (!result) {
      qDebugNN << LOGSEC_NETWORK << "External web browser call failed.";
    }
  }
  else {
    result = QDesktopServices::openUrl(url);
  }

  if (!result) {
    // We display GUI information that browser was not probably opened.
    MsgBox::show(qApp->mainFormWidget(),
                 QMessageBox::Icon::Critical,
                 tr("Navigate to website manually"),
                 tr("%1 was unable to launch your web browser with the given URL, you need to open the "
                    "below website URL in your web browser manually.")
                   .arg(QSL(APP_NAME)),
                 {},
                 url,
                 QMessageBox::StandardButton::Ok);
  }

  return result;
}

QString WebFactory::stripTags(QString text) {
  return text.remove(QRegularExpression(QSL("<[^>]*>")));
}

QString WebFactory::unescapeHtml(const QString& html) {
  if (html.isEmpty()) {
    return html;
  }

  if (m_htmlNamedEntities.isEmpty()) {
    generateUnescapes();
  }

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
            output.append(QChar(number));
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

          if (m_htmlNamedEntities.contains(entity_name)) {
            // Entity found, proceed.
            output.append(m_htmlNamedEntities.value(entity_name));
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

AdBlockManager* WebFactory::adBlock() const {
  return m_adBlock;
}

#if defined(USE_WEBENGINE)
NetworkUrlInterceptor* WebFactory::urlIinterceptor() const {
  return m_urlInterceptor;
}

QWebEngineProfile* WebFactory::engineProfile() const {
  return m_engineProfile;
}

QAction* WebFactory::engineSettingsAction() {
  if (m_engineSettings == nullptr) {
    m_engineSettings =
      new QAction(qApp->icons()->fromTheme(QSL("applications-internet")), tr("Web engine settings"), this);
    m_engineSettings->setMenu(new QMenu());
    createMenu(m_engineSettings->menu());
    connect(m_engineSettings->menu(), &QMenu::aboutToShow, this, [this]() {
      createMenu();
    });
  }

  return m_engineSettings;
}

void WebFactory::createMenu(QMenu* menu) {
  if (menu == nullptr) {
    menu = qobject_cast<QMenu*>(sender());

    if (menu == nullptr) {
      return;
    }
  }

  menu->clear();
  QList<QAction*> actions;

  actions << createEngineSettingsAction(tr("Auto-load images"), QWebEngineSettings::WebAttribute::AutoLoadImages);
  actions << createEngineSettingsAction(tr("JS enabled"), QWebEngineSettings::WebAttribute::JavascriptEnabled);
  actions << createEngineSettingsAction(tr("JS can open popup windows"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanOpenWindows);
  actions << createEngineSettingsAction(tr("JS can access clipboard"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanAccessClipboard);
  actions << createEngineSettingsAction(tr("Hyperlinks can get focus"),
                                        QWebEngineSettings::WebAttribute::LinksIncludedInFocusChain);
  actions << createEngineSettingsAction(tr("Local storage enabled"),
                                        QWebEngineSettings::WebAttribute::LocalStorageEnabled);
  actions << createEngineSettingsAction(tr("Local content can access remote URLs"),
                                        QWebEngineSettings::WebAttribute::LocalContentCanAccessRemoteUrls);
  actions << createEngineSettingsAction(tr("XSS auditing enabled"),
                                        QWebEngineSettings::WebAttribute::XSSAuditingEnabled);
  actions << createEngineSettingsAction(tr("Spatial navigation enabled"),
                                        QWebEngineSettings::WebAttribute::SpatialNavigationEnabled);
  actions << createEngineSettingsAction(tr("Local content can access local files"),
                                        QWebEngineSettings::WebAttribute::LocalContentCanAccessFileUrls);
  actions << createEngineSettingsAction(tr("Hyperlink auditing enabled"),
                                        QWebEngineSettings::WebAttribute::HyperlinkAuditingEnabled);
  actions << createEngineSettingsAction(tr("Animate scrolling"),
                                        QWebEngineSettings::WebAttribute::ScrollAnimatorEnabled);
  actions << createEngineSettingsAction(tr("Error pages enabled"), QWebEngineSettings::WebAttribute::ErrorPageEnabled);
  actions << createEngineSettingsAction(tr("Plugins enabled"), QWebEngineSettings::WebAttribute::PluginsEnabled);
  actions << createEngineSettingsAction(tr("Fullscreen enabled"),
                                        QWebEngineSettings::WebAttribute::FullScreenSupportEnabled);

#if !defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  actions << createEngineSettingsAction(tr("Screen capture enabled"),
                                        QWebEngineSettings::WebAttribute::ScreenCaptureEnabled);
  actions << createEngineSettingsAction(tr("WebGL enabled"), QWebEngineSettings::WebAttribute::WebGLEnabled);
  actions << createEngineSettingsAction(tr("Accelerate 2D canvas"),
                                        QWebEngineSettings::WebAttribute::Accelerated2dCanvasEnabled);
  actions << createEngineSettingsAction(tr("Print element backgrounds"),
                                        QWebEngineSettings::WebAttribute::PrintElementBackgrounds);
  actions << createEngineSettingsAction(tr("Allow running insecure content"),
                                        QWebEngineSettings::WebAttribute::AllowRunningInsecureContent);
  actions << createEngineSettingsAction(tr("Allow geolocation on insecure origins"),
                                        QWebEngineSettings::WebAttribute::AllowGeolocationOnInsecureOrigins);
#endif

  actions << createEngineSettingsAction(tr("JS can activate windows"),
                                        QWebEngineSettings::WebAttribute::AllowWindowActivationFromJavaScript);
  actions << createEngineSettingsAction(tr("Show scrollbars"), QWebEngineSettings::WebAttribute::ShowScrollBars);
  actions << createEngineSettingsAction(tr("Media playback with gestures"),
                                        QWebEngineSettings::WebAttribute::PlaybackRequiresUserGesture);
  actions << createEngineSettingsAction(tr("WebRTC uses only public interfaces"),
                                        QWebEngineSettings::WebAttribute::WebRTCPublicInterfacesOnly);
  actions << createEngineSettingsAction(tr("JS can paste from clipboard"),
                                        QWebEngineSettings::WebAttribute::JavascriptCanPaste);
  actions << createEngineSettingsAction(tr("DNS prefetch enabled"),
                                        QWebEngineSettings::WebAttribute::DnsPrefetchEnabled);

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  actions << createEngineSettingsAction(tr("PDF viewer enabled"), QWebEngineSettings::WebAttribute::PdfViewerEnabled);
#endif

  menu->addActions(actions);
}

void WebFactory::webEngineSettingChanged(bool enabled) {
  const QAction* const act = qobject_cast<QAction*>(sender());

  QWebEngineSettings::WebAttribute attribute = static_cast<QWebEngineSettings::WebAttribute>(act->data().toInt());

  qApp->settings()->setValue(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), enabled);
  m_engineProfile->settings()->setAttribute(attribute, act->isChecked());
}

QAction* WebFactory::createEngineSettingsAction(const QString& title, QWebEngineSettings::WebAttribute attribute) {
  auto* act = new QAction(title, m_engineSettings->menu());

  act->setData(attribute);
  act->setCheckable(true);
  act->setChecked(qApp->settings()
                    ->value(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), true)
                    .toBool());

  auto enabl = act->isChecked();

  m_engineProfile->settings()->setAttribute(attribute, enabl);
  connect(act, &QAction::toggled, this, &WebFactory::webEngineSettingChanged);
  return act;
}

#endif

CookieJar* WebFactory::cookieJar() const {
  return m_cookieJar;
}

Readability* WebFactory::readability() const {
  return m_readability;
}

void WebFactory::generateUnescapes() {
  m_htmlNamedEntities[QSL("AElig")] = 0x00c6;
  m_htmlNamedEntities[QSL("AMP")] = 38;
  m_htmlNamedEntities[QSL("Aacute")] = 0x00c1;
  m_htmlNamedEntities[QSL("Acirc")] = 0x00c2;
  m_htmlNamedEntities[QSL("Agrave")] = 0x00c0;
  m_htmlNamedEntities[QSL("Alpha")] = 0x0391;
  m_htmlNamedEntities[QSL("Aring")] = 0x00c5;
  m_htmlNamedEntities[QSL("Atilde")] = 0x00c3;
  m_htmlNamedEntities[QSL("Auml")] = 0x00c4;
  m_htmlNamedEntities[QSL("Beta")] = 0x0392;
  m_htmlNamedEntities[QSL("Ccedil")] = 0x00c7;
  m_htmlNamedEntities[QSL("Chi")] = 0x03a7;
  m_htmlNamedEntities[QSL("Dagger")] = 0x2021;
  m_htmlNamedEntities[QSL("Delta")] = 0x0394;
  m_htmlNamedEntities[QSL("ETH")] = 0x00d0;
  m_htmlNamedEntities[QSL("Eacute")] = 0x00c9;
  m_htmlNamedEntities[QSL("Ecirc")] = 0x00ca;
  m_htmlNamedEntities[QSL("Egrave")] = 0x00c8;
  m_htmlNamedEntities[QSL("Epsilon")] = 0x0395;
  m_htmlNamedEntities[QSL("Eta")] = 0x0397;
  m_htmlNamedEntities[QSL("Euml")] = 0x00cb;
  m_htmlNamedEntities[QSL("GT")] = 62;
  m_htmlNamedEntities[QSL("Gamma")] = 0x0393;
  m_htmlNamedEntities[QSL("Iacute")] = 0x00cd;
  m_htmlNamedEntities[QSL("Icirc")] = 0x00ce;
  m_htmlNamedEntities[QSL("Igrave")] = 0x00cc;
  m_htmlNamedEntities[QSL("Iota")] = 0x0399;
  m_htmlNamedEntities[QSL("Iuml")] = 0x00cf;
  m_htmlNamedEntities[QSL("Kappa")] = 0x039a;
  m_htmlNamedEntities[QSL("LT")] = 60;
  m_htmlNamedEntities[QSL("Lambda")] = 0x039b;
  m_htmlNamedEntities[QSL("Mu")] = 0x039c;
  m_htmlNamedEntities[QSL("Ntilde")] = 0x00d1;
  m_htmlNamedEntities[QSL("Nu")] = 0x039d;
  m_htmlNamedEntities[QSL("OElig")] = 0x0152;
  m_htmlNamedEntities[QSL("Oacute")] = 0x00d3;
  m_htmlNamedEntities[QSL("Ocirc")] = 0x00d4;
  m_htmlNamedEntities[QSL("Ograve")] = 0x00d2;
  m_htmlNamedEntities[QSL("Omega")] = 0x03a9;
  m_htmlNamedEntities[QSL("Omicron")] = 0x039f;
  m_htmlNamedEntities[QSL("Oslash")] = 0x00d8;
  m_htmlNamedEntities[QSL("Otilde")] = 0x00d5;
  m_htmlNamedEntities[QSL("Ouml")] = 0x00d6;
  m_htmlNamedEntities[QSL("Phi")] = 0x03a6;
  m_htmlNamedEntities[QSL("Pi")] = 0x03a0;
  m_htmlNamedEntities[QSL("Prime")] = 0x2033;
  m_htmlNamedEntities[QSL("Psi")] = 0x03a8;
  m_htmlNamedEntities[QSL("QUOT")] = 34;
  m_htmlNamedEntities[QSL("Rho")] = 0x03a1;
  m_htmlNamedEntities[QSL("Scaron")] = 0x0160;
  m_htmlNamedEntities[QSL("Sigma")] = 0x03a3;
  m_htmlNamedEntities[QSL("THORN")] = 0x00de;
  m_htmlNamedEntities[QSL("Tau")] = 0x03a4;
  m_htmlNamedEntities[QSL("Theta")] = 0x0398;
  m_htmlNamedEntities[QSL("Uacute")] = 0x00da;
  m_htmlNamedEntities[QSL("Ucirc")] = 0x00db;
  m_htmlNamedEntities[QSL("Ugrave")] = 0x00d9;
  m_htmlNamedEntities[QSL("Upsilon")] = 0x03a5;
  m_htmlNamedEntities[QSL("Uuml")] = 0x00dc;
  m_htmlNamedEntities[QSL("Xi")] = 0x039e;
  m_htmlNamedEntities[QSL("Yacute")] = 0x00dd;
  m_htmlNamedEntities[QSL("Yuml")] = 0x0178;
  m_htmlNamedEntities[QSL("Zeta")] = 0x0396;
  m_htmlNamedEntities[QSL("aacute")] = 0x00e1;
  m_htmlNamedEntities[QSL("acirc")] = 0x00e2;
  m_htmlNamedEntities[QSL("acute")] = 0x00b4;
  m_htmlNamedEntities[QSL("aelig")] = 0x00e6;
  m_htmlNamedEntities[QSL("agrave")] = 0x00e0;
  m_htmlNamedEntities[QSL("alefsym")] = 0x2135;
  m_htmlNamedEntities[QSL("alpha")] = 0x03b1;
  m_htmlNamedEntities[QSL("amp")] = 38;
  m_htmlNamedEntities[QSL("and")] = 0x22a5;
  m_htmlNamedEntities[QSL("ang")] = 0x2220;
  m_htmlNamedEntities[QSL("apos")] = 0x0027;
  m_htmlNamedEntities[QSL("aring")] = 0x00e5;
  m_htmlNamedEntities[QSL("asymp")] = 0x2248;
  m_htmlNamedEntities[QSL("atilde")] = 0x00e3;
  m_htmlNamedEntities[QSL("auml")] = 0x00e4;
  m_htmlNamedEntities[QSL("bdquo")] = 0x201e;
  m_htmlNamedEntities[QSL("beta")] = 0x03b2;
  m_htmlNamedEntities[QSL("brvbar")] = 0x00a6;
  m_htmlNamedEntities[QSL("bull")] = 0x2022;
  m_htmlNamedEntities[QSL("cap")] = 0x2229;
  m_htmlNamedEntities[QSL("ccedil")] = 0x00e7;
  m_htmlNamedEntities[QSL("cedil")] = 0x00b8;
  m_htmlNamedEntities[QSL("cent")] = 0x00a2;
  m_htmlNamedEntities[QSL("chi")] = 0x03c7;
  m_htmlNamedEntities[QSL("circ")] = 0x02c6;
  m_htmlNamedEntities[QSL("clubs")] = 0x2663;
  m_htmlNamedEntities[QSL("cong")] = 0x2245;
  m_htmlNamedEntities[QSL("copy")] = 0x00a9;
  m_htmlNamedEntities[QSL("crarr")] = 0x21b5;
  m_htmlNamedEntities[QSL("cup")] = 0x222a;
  m_htmlNamedEntities[QSL("curren")] = 0x00a4;
  m_htmlNamedEntities[QSL("dArr")] = 0x21d3;
  m_htmlNamedEntities[QSL("dagger")] = 0x2020;
  m_htmlNamedEntities[QSL("darr")] = 0x2193;
  m_htmlNamedEntities[QSL("deg")] = 0x00b0;
  m_htmlNamedEntities[QSL("delta")] = 0x03b4;
  m_htmlNamedEntities[QSL("diams")] = 0x2666;
  m_htmlNamedEntities[QSL("divide")] = 0x00f7;
  m_htmlNamedEntities[QSL("eacute")] = 0x00e9;
  m_htmlNamedEntities[QSL("ecirc")] = 0x00ea;
  m_htmlNamedEntities[QSL("egrave")] = 0x00e8;
  m_htmlNamedEntities[QSL("empty")] = 0x2205;
  m_htmlNamedEntities[QSL("emsp")] = 0x2003;
  m_htmlNamedEntities[QSL("ensp")] = 0x2002;
  m_htmlNamedEntities[QSL("epsilon")] = 0x03b5;
  m_htmlNamedEntities[QSL("equiv")] = 0x2261;
  m_htmlNamedEntities[QSL("eta")] = 0x03b7;
  m_htmlNamedEntities[QSL("eth")] = 0x00f0;
  m_htmlNamedEntities[QSL("euml")] = 0x00eb;
  m_htmlNamedEntities[QSL("euro")] = 0x20ac;
  m_htmlNamedEntities[QSL("exist")] = 0x2203;
  m_htmlNamedEntities[QSL("fnof")] = 0x0192;
  m_htmlNamedEntities[QSL("forall")] = 0x2200;
  m_htmlNamedEntities[QSL("frac12")] = 0x00bd;
  m_htmlNamedEntities[QSL("frac14")] = 0x00bc;
  m_htmlNamedEntities[QSL("frac34")] = 0x00be;
  m_htmlNamedEntities[QSL("frasl")] = 0x2044;
  m_htmlNamedEntities[QSL("gamma")] = 0x03b3;
  m_htmlNamedEntities[QSL("ge")] = 0x2265;
  m_htmlNamedEntities[QSL("gt")] = 62;
  m_htmlNamedEntities[QSL("hArr")] = 0x21d4;
  m_htmlNamedEntities[QSL("harr")] = 0x2194;
  m_htmlNamedEntities[QSL("hearts")] = 0x2665;
  m_htmlNamedEntities[QSL("hellip")] = 0x2026;
  m_htmlNamedEntities[QSL("iacute")] = 0x00ed;
  m_htmlNamedEntities[QSL("icirc")] = 0x00ee;
  m_htmlNamedEntities[QSL("iexcl")] = 0x00a1;
  m_htmlNamedEntities[QSL("igrave")] = 0x00ec;
  m_htmlNamedEntities[QSL("image")] = 0x2111;
  m_htmlNamedEntities[QSL("infin")] = 0x221e;
  m_htmlNamedEntities[QSL("int")] = 0x222b;
  m_htmlNamedEntities[QSL("iota")] = 0x03b9;
  m_htmlNamedEntities[QSL("iquest")] = 0x00bf;
  m_htmlNamedEntities[QSL("isin")] = 0x2208;
  m_htmlNamedEntities[QSL("iuml")] = 0x00ef;
  m_htmlNamedEntities[QSL("kappa")] = 0x03ba;
  m_htmlNamedEntities[QSL("lArr")] = 0x21d0;
  m_htmlNamedEntities[QSL("lambda")] = 0x03bb;
  m_htmlNamedEntities[QSL("lang")] = 0x2329;
  m_htmlNamedEntities[QSL("laquo")] = 0x00ab;
  m_htmlNamedEntities[QSL("larr")] = 0x2190;
  m_htmlNamedEntities[QSL("lceil")] = 0x2308;
  m_htmlNamedEntities[QSL("ldquo")] = 0x201c;
  m_htmlNamedEntities[QSL("le")] = 0x2264;
  m_htmlNamedEntities[QSL("lfloor")] = 0x230a;
  m_htmlNamedEntities[QSL("lowast")] = 0x2217;
  m_htmlNamedEntities[QSL("loz")] = 0x25ca;
  m_htmlNamedEntities[QSL("lrm")] = 0x200e;
  m_htmlNamedEntities[QSL("lsaquo")] = 0x2039;
  m_htmlNamedEntities[QSL("lsquo")] = 0x2018;
  m_htmlNamedEntities[QSL("lt")] = 60;
  m_htmlNamedEntities[QSL("macr")] = 0x00af;
  m_htmlNamedEntities[QSL("mdash")] = 0x2014;
  m_htmlNamedEntities[QSL("micro")] = 0x00b5;
  m_htmlNamedEntities[QSL("middot")] = 0x00b7;
  m_htmlNamedEntities[QSL("minus")] = 0x2212;
  m_htmlNamedEntities[QSL("mu")] = 0x03bc;
  m_htmlNamedEntities[QSL("nabla")] = 0x2207;
  m_htmlNamedEntities[QSL("nbsp")] = 0x00a0;
  m_htmlNamedEntities[QSL("ndash")] = 0x2013;
  m_htmlNamedEntities[QSL("ne")] = 0x2260;
  m_htmlNamedEntities[QSL("ni")] = 0x220b;
  m_htmlNamedEntities[QSL("not")] = 0x00ac;
  m_htmlNamedEntities[QSL("notin")] = 0x2209;
  m_htmlNamedEntities[QSL("nsub")] = 0x2284;
  m_htmlNamedEntities[QSL("ntilde")] = 0x00f1;
  m_htmlNamedEntities[QSL("nu")] = 0x03bd;
  m_htmlNamedEntities[QSL("oacute")] = 0x00f3;
  m_htmlNamedEntities[QSL("ocirc")] = 0x00f4;
  m_htmlNamedEntities[QSL("oelig")] = 0x0153;
  m_htmlNamedEntities[QSL("ograve")] = 0x00f2;
  m_htmlNamedEntities[QSL("oline")] = 0x203e;
  m_htmlNamedEntities[QSL("omega")] = 0x03c9;
  m_htmlNamedEntities[QSL("omicron")] = 0x03bf;
  m_htmlNamedEntities[QSL("oplus")] = 0x2295;
  m_htmlNamedEntities[QSL("or")] = 0x22a6;
  m_htmlNamedEntities[QSL("ordf")] = 0x00aa;
  m_htmlNamedEntities[QSL("ordm")] = 0x00ba;
  m_htmlNamedEntities[QSL("oslash")] = 0x00f8;
  m_htmlNamedEntities[QSL("otilde")] = 0x00f5;
  m_htmlNamedEntities[QSL("otimes")] = 0x2297;
  m_htmlNamedEntities[QSL("ouml")] = 0x00f6;
  m_htmlNamedEntities[QSL("para")] = 0x00b6;
  m_htmlNamedEntities[QSL("part")] = 0x2202;
  m_htmlNamedEntities[QSL("percnt")] = 0x0025;
  m_htmlNamedEntities[QSL("permil")] = 0x2030;
  m_htmlNamedEntities[QSL("perp")] = 0x22a5;
  m_htmlNamedEntities[QSL("phi")] = 0x03c6;
  m_htmlNamedEntities[QSL("pi")] = 0x03c0;
  m_htmlNamedEntities[QSL("piv")] = 0x03d6;
  m_htmlNamedEntities[QSL("plusmn")] = 0x00b1;
  m_htmlNamedEntities[QSL("pound")] = 0x00a3;
  m_htmlNamedEntities[QSL("prime")] = 0x2032;
  m_htmlNamedEntities[QSL("prod")] = 0x220f;
  m_htmlNamedEntities[QSL("prop")] = 0x221d;
  m_htmlNamedEntities[QSL("psi")] = 0x03c8;
  m_htmlNamedEntities[QSL("quot")] = 34;
  m_htmlNamedEntities[QSL("rArr")] = 0x21d2;
  m_htmlNamedEntities[QSL("radic")] = 0x221a;
  m_htmlNamedEntities[QSL("rang")] = 0x232a;
  m_htmlNamedEntities[QSL("raquo")] = 0x00bb;
  m_htmlNamedEntities[QSL("rarr")] = 0x2192;
  m_htmlNamedEntities[QSL("rceil")] = 0x2309;
  m_htmlNamedEntities[QSL("rdquo")] = 0x201d;
  m_htmlNamedEntities[QSL("real")] = 0x211c;
  m_htmlNamedEntities[QSL("reg")] = 0x00ae;
  m_htmlNamedEntities[QSL("rfloor")] = 0x230b;
  m_htmlNamedEntities[QSL("rho")] = 0x03c1;
  m_htmlNamedEntities[QSL("rlm")] = 0x200f;
  m_htmlNamedEntities[QSL("rsaquo")] = 0x203a;
  m_htmlNamedEntities[QSL("rsquo")] = 0x2019;
  m_htmlNamedEntities[QSL("sbquo")] = 0x201a;
  m_htmlNamedEntities[QSL("scaron")] = 0x0161;
  m_htmlNamedEntities[QSL("sdot")] = 0x22c5;
  m_htmlNamedEntities[QSL("sect")] = 0x00a7;
  m_htmlNamedEntities[QSL("shy")] = 0x00ad;
  m_htmlNamedEntities[QSL("sigma")] = 0x03c3;
  m_htmlNamedEntities[QSL("sigmaf")] = 0x03c2;
  m_htmlNamedEntities[QSL("sim")] = 0x223c;
  m_htmlNamedEntities[QSL("spades")] = 0x2660;
  m_htmlNamedEntities[QSL("sub")] = 0x2282;
  m_htmlNamedEntities[QSL("sube")] = 0x2286;
  m_htmlNamedEntities[QSL("sum")] = 0x2211;
  m_htmlNamedEntities[QSL("sup")] = 0x2283;
  m_htmlNamedEntities[QSL("sup1")] = 0x00b9;
  m_htmlNamedEntities[QSL("sup2")] = 0x00b2;
  m_htmlNamedEntities[QSL("sup3")] = 0x00b3;
  m_htmlNamedEntities[QSL("supe")] = 0x2287;
  m_htmlNamedEntities[QSL("szlig")] = 0x00df;
  m_htmlNamedEntities[QSL("tau")] = 0x03c4;
  m_htmlNamedEntities[QSL("there4")] = 0x2234;
  m_htmlNamedEntities[QSL("theta")] = 0x03b8;
  m_htmlNamedEntities[QSL("thetasym")] = 0x03d1;
  m_htmlNamedEntities[QSL("thinsp")] = 0x2009;
  m_htmlNamedEntities[QSL("thorn")] = 0x00fe;
  m_htmlNamedEntities[QSL("tilde")] = 0x02dc;
  m_htmlNamedEntities[QSL("times")] = 0x00d7;
  m_htmlNamedEntities[QSL("trade")] = 0x2122;
  m_htmlNamedEntities[QSL("uArr")] = 0x21d1;
  m_htmlNamedEntities[QSL("uacute")] = 0x00fa;
  m_htmlNamedEntities[QSL("uarr")] = 0x2191;
  m_htmlNamedEntities[QSL("ucirc")] = 0x00fb;
  m_htmlNamedEntities[QSL("ugrave")] = 0x00f9;
  m_htmlNamedEntities[QSL("uml")] = 0x00a8;
  m_htmlNamedEntities[QSL("upsih")] = 0x03d2;
  m_htmlNamedEntities[QSL("upsilon")] = 0x03c5;
  m_htmlNamedEntities[QSL("uuml")] = 0x00fc;
  m_htmlNamedEntities[QSL("weierp")] = 0x2118;
  m_htmlNamedEntities[QSL("xi")] = 0x03be;
  m_htmlNamedEntities[QSL("yacute")] = 0x00fd;
  m_htmlNamedEntities[QSL("yen")] = 0x00a5;
  m_htmlNamedEntities[QSL("yuml")] = 0x00ff;
  m_htmlNamedEntities[QSL("zeta")] = 0x03b6;
  m_htmlNamedEntities[QSL("zwj")] = 0x200d;
  m_htmlNamedEntities[QSL("zwnj")] = 0x200c;
}

QString WebFactory::customUserAgent() const {
  return m_customUserAgent;
}

void WebFactory::setCustomUserAgent(const QString& user_agent) {
  m_customUserAgent = user_agent;
}

#if defined(USE_WEBENGINE)
void WebFactory::cleanupCache() {
  if (MsgBox::show(nullptr,
                   QMessageBox::Icon::Question,
                   tr("Web cache is going to be cleared"),
                   tr("Do you really want to clear web cache?"),
                   {},
                   {},
                   QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No) ==
      QMessageBox::StandardButton::Yes) {
    m_engineProfile->clearHttpCache();

    // NOTE: Manually clear storage.
    try {
      IOFactory::removeFolder(m_engineProfile->persistentStoragePath());
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_CORE << "Removing of webengine storage failed:" << QUOTE_W_SPACE_DOT(ex.message());
    }
  }
}
#endif
