// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webfactory.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

#if defined (USE_WEBENGINE)
#include <QWebEngineProfile>
#endif

WebFactory::WebFactory(QObject* parent)
  : QObject(parent) {
#if defined (USE_WEBENGINE)
  m_engineSettings = nullptr;
#endif
}

WebFactory::~WebFactory() {
#if defined (USE_WEBENGINE)
  if (m_engineSettings != nullptr && m_engineSettings->menu() != nullptr) {
    m_engineSettings->menu()->deleteLater();
  }
#endif
}

bool WebFactory::sendMessageViaEmail(const Message& message) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool()) {
    const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString();
    const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString();

    return IOFactory::startProcessDetached(browser, {}, arguments.arg(message.m_title, stripTags(message.m_contents)));
  }
  else {
    // Send it via mailto protocol.
    // NOTE: http://en.wikipedia.org/wiki/Mailto
    return QDesktopServices::openUrl(QString("mailto:?subject=%1&body=%2").arg(QString(QUrl::toPercentEncoding(message.m_title)),
                                                                               QString(QUrl::toPercentEncoding(stripTags(
                                                                                                                 message.m_contents)))));
  }
}

bool WebFactory::openUrlInExternalBrowser(const QString& url) const {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
    const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
    const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();
    auto nice_args = arguments.arg(url);

    qDebugNN << LOGSEC_NETWORK << "Arguments for external browser:" << QUOTE_W_SPACE_DOT(nice_args);

    const bool result = IOFactory::startProcessDetached(browser, {}, nice_args);

    if (!result) {
      qDebugNN << LOGSEC_NETWORK << "External web browser call failed.";
    }

    return result;
  }
  else {
    return QDesktopServices::openUrl(url);
  }
}

QString WebFactory::stripTags(QString text) {
  return text.remove(QRegularExpression(QSL("<[^>]*>")));
}

QString WebFactory::unescapeHtml(const QString& html) {
  if (m_escapes.isEmpty()) {
    generateUnescapes();
  }

  QString output = html;
  QMapIterator<QString, char16_t> i(m_escapes);

  while (i.hasNext()) {
    i.next();
    output = output.replace(i.key(), QString(QChar(i.value())));
  }

  return output;
}

void WebFactory::updateProxy() {
  const QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy),
                                                                                                                     SETTING(Proxy::Type)).
                                                                                             toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
  }
  else if (selected_proxy_type == QNetworkProxy::DefaultProxy) {
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

    QNetworkProxy::setApplicationProxy(new_proxy);
  }
}

#if defined (USE_WEBENGINE)
QAction* WebFactory::engineSettingsAction() {
  if (m_engineSettings == nullptr) {
    m_engineSettings = new QAction(qApp->icons()->fromTheme(QSL("applications-internet")), tr("Web engine settings"), this);
    m_engineSettings->setMenu(new QMenu());
    createMenu(m_engineSettings->menu());
    connect(m_engineSettings->menu(), SIGNAL(aboutToShow()), this, SLOT(createMenu()));
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

  actions << createEngineSettingsAction(tr("Auto-load images"), QWebEngineSettings::AutoLoadImages);
  actions << createEngineSettingsAction(tr("JS enabled"), QWebEngineSettings::JavascriptEnabled);
  actions << createEngineSettingsAction(tr("JS can open popup windows"), QWebEngineSettings::JavascriptCanOpenWindows);
  actions << createEngineSettingsAction(tr("JS can access clipboard"), QWebEngineSettings::JavascriptCanAccessClipboard);
  actions << createEngineSettingsAction(tr("Hyperlinks can get focus"), QWebEngineSettings::LinksIncludedInFocusChain);
  actions << createEngineSettingsAction(tr("Local storage enabled"), QWebEngineSettings::LocalStorageEnabled);
  actions << createEngineSettingsAction(tr("Local content can access remote URLs"), QWebEngineSettings::LocalContentCanAccessRemoteUrls);
  actions << createEngineSettingsAction(tr("XSS auditing enabled"), QWebEngineSettings::XSSAuditingEnabled);
  actions << createEngineSettingsAction(tr("Spatial navigation enabled"), QWebEngineSettings::SpatialNavigationEnabled);
  actions << createEngineSettingsAction(tr("Local content can access local files"), QWebEngineSettings::LocalContentCanAccessFileUrls);
  actions << createEngineSettingsAction(tr("Hyperlink auditing enabled"), QWebEngineSettings::HyperlinkAuditingEnabled);
  actions << createEngineSettingsAction(tr("Animate scrolling"), QWebEngineSettings::ScrollAnimatorEnabled);
  actions << createEngineSettingsAction(tr("Error pages enabled"), QWebEngineSettings::ErrorPageEnabled);
  actions << createEngineSettingsAction(tr("Plugins enabled"), QWebEngineSettings::PluginsEnabled);
  actions << createEngineSettingsAction(tr("Fullscreen enabled"), QWebEngineSettings::FullScreenSupportEnabled);

#if !defined(Q_OS_LINUX)
  actions << createEngineSettingsAction(tr("Screen capture enabled"), QWebEngineSettings::ScreenCaptureEnabled);
  actions << createEngineSettingsAction(tr("WebGL enabled"), QWebEngineSettings::WebGLEnabled);
  actions << createEngineSettingsAction(tr("Accelerate 2D canvas"), QWebEngineSettings::Accelerated2dCanvasEnabled);
  actions << createEngineSettingsAction(tr("Print element backgrounds"), QWebEngineSettings::PrintElementBackgrounds);
  actions << createEngineSettingsAction(tr("Allow running insecure content"), QWebEngineSettings::AllowRunningInsecureContent);
  actions << createEngineSettingsAction(tr("Allow geolocation on insecure origins"), QWebEngineSettings::AllowGeolocationOnInsecureOrigins);
#endif

  menu->addActions(actions);
}

void WebFactory::webEngineSettingChanged(bool enabled) {
  const QAction* const act = qobject_cast<QAction*>(sender());

  QWebEngineSettings::WebAttribute attribute = static_cast<QWebEngineSettings::WebAttribute>(act->data().toInt());

  qApp->settings()->setValue(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), enabled);
  QWebEngineProfile::defaultProfile()->settings()->setAttribute(attribute, act->isChecked());
}

QAction* WebFactory::createEngineSettingsAction(const QString& title, QWebEngineSettings::WebAttribute attribute) {
  auto* act = new QAction(title, m_engineSettings->menu());

  act->setData(attribute);
  act->setCheckable(true);
  act->setChecked(qApp->settings()->value(WebEngineAttributes::ID, QString::number(static_cast<int>(attribute)), true).toBool());
  QWebEngineProfile::defaultProfile()->settings()->setAttribute(attribute, act->isChecked());
  connect(act, &QAction::toggled, this, &WebFactory::webEngineSettingChanged);
  return act;
}

#endif

void WebFactory::generateUnescapes() {
  m_escapes[QSL("&AElig;")] = 0x00c6;
  m_escapes[QSL("&AMP;")] = 38;
  m_escapes[QSL("&Aacute;")] = 0x00c1;
  m_escapes[QSL("&Acirc;")] = 0x00c2;
  m_escapes[QSL("&Agrave;")] = 0x00c0;
  m_escapes[QSL("&Alpha;")] = 0x0391;
  m_escapes[QSL("&Aring;")] = 0x00c5;
  m_escapes[QSL("&Atilde;")] = 0x00c3;
  m_escapes[QSL("&Auml;")] = 0x00c4;
  m_escapes[QSL("&Beta;")] = 0x0392;
  m_escapes[QSL("&Ccedil;")] = 0x00c7;
  m_escapes[QSL("&Chi;")] = 0x03a7;
  m_escapes[QSL("&Dagger;")] = 0x2021;
  m_escapes[QSL("&Delta;")] = 0x0394;
  m_escapes[QSL("&ETH;")] = 0x00d0;
  m_escapes[QSL("&Eacute;")] = 0x00c9;
  m_escapes[QSL("&Ecirc;")] = 0x00ca;
  m_escapes[QSL("&Egrave;")] = 0x00c8;
  m_escapes[QSL("&Epsilon;")] = 0x0395;
  m_escapes[QSL("&Eta;")] = 0x0397;
  m_escapes[QSL("&Euml;")] = 0x00cb;
  m_escapes[QSL("&GT;")] = 62;
  m_escapes[QSL("&Gamma;")] = 0x0393;
  m_escapes[QSL("&Iacute;")] = 0x00cd;
  m_escapes[QSL("&Icirc;")] = 0x00ce;
  m_escapes[QSL("&Igrave;")] = 0x00cc;
  m_escapes[QSL("&Iota;")] = 0x0399;
  m_escapes[QSL("&Iuml;")] = 0x00cf;
  m_escapes[QSL("&Kappa;")] = 0x039a;
  m_escapes[QSL("&LT;")] = 60;
  m_escapes[QSL("&Lambda;")] = 0x039b;
  m_escapes[QSL("&Mu;")] = 0x039c;
  m_escapes[QSL("&Ntilde;")] = 0x00d1;
  m_escapes[QSL("&Nu;")] = 0x039d;
  m_escapes[QSL("&OElig;")] = 0x0152;
  m_escapes[QSL("&Oacute;")] = 0x00d3;
  m_escapes[QSL("&Ocirc;")] = 0x00d4;
  m_escapes[QSL("&Ograve;")] = 0x00d2;
  m_escapes[QSL("&Omega;")] = 0x03a9;
  m_escapes[QSL("&Omicron;")] = 0x039f;
  m_escapes[QSL("&Oslash;")] = 0x00d8;
  m_escapes[QSL("&Otilde;")] = 0x00d5;
  m_escapes[QSL("&Ouml;")] = 0x00d6;
  m_escapes[QSL("&Phi;")] = 0x03a6;
  m_escapes[QSL("&Pi;")] = 0x03a0;
  m_escapes[QSL("&Prime;")] = 0x2033;
  m_escapes[QSL("&Psi;")] = 0x03a8;
  m_escapes[QSL("&QUOT;")] = 34;
  m_escapes[QSL("&Rho;")] = 0x03a1;
  m_escapes[QSL("&Scaron;")] = 0x0160;
  m_escapes[QSL("&Sigma;")] = 0x03a3;
  m_escapes[QSL("&THORN;")] = 0x00de;
  m_escapes[QSL("&Tau;")] = 0x03a4;
  m_escapes[QSL("&Theta;")] = 0x0398;
  m_escapes[QSL("&Uacute;")] = 0x00da;
  m_escapes[QSL("&Ucirc;")] = 0x00db;
  m_escapes[QSL("&Ugrave;")] = 0x00d9;
  m_escapes[QSL("&Upsilon;")] = 0x03a5;
  m_escapes[QSL("&Uuml;")] = 0x00dc;
  m_escapes[QSL("&Xi;")] = 0x039e;
  m_escapes[QSL("&Yacute;")] = 0x00dd;
  m_escapes[QSL("&Yuml;")] = 0x0178;
  m_escapes[QSL("&Zeta;")] = 0x0396;
  m_escapes[QSL("&aacute;")] = 0x00e1;
  m_escapes[QSL("&acirc;")] = 0x00e2;
  m_escapes[QSL("&acute;")] = 0x00b4;
  m_escapes[QSL("&aelig;")] = 0x00e6;
  m_escapes[QSL("&agrave;")] = 0x00e0;
  m_escapes[QSL("&alefsym;")] = 0x2135;
  m_escapes[QSL("&alpha;")] = 0x03b1;
  m_escapes[QSL("&amp;")] = 38;
  m_escapes[QSL("&and;")] = 0x22a5;
  m_escapes[QSL("&ang;")] = 0x2220;
  m_escapes[QSL("&apos;")] = 0x0027;
  m_escapes[QSL("&aring;")] = 0x00e5;
  m_escapes[QSL("&asymp;")] = 0x2248;
  m_escapes[QSL("&atilde;")] = 0x00e3;
  m_escapes[QSL("&auml;")] = 0x00e4;
  m_escapes[QSL("&bdquo;")] = 0x201e;
  m_escapes[QSL("&beta;")] = 0x03b2;
  m_escapes[QSL("&brvbar;")] = 0x00a6;
  m_escapes[QSL("&bull;")] = 0x2022;
  m_escapes[QSL("&cap;")] = 0x2229;
  m_escapes[QSL("&ccedil;")] = 0x00e7;
  m_escapes[QSL("&cedil;")] = 0x00b8;
  m_escapes[QSL("&cent;")] = 0x00a2;
  m_escapes[QSL("&chi;")] = 0x03c7;
  m_escapes[QSL("&circ;")] = 0x02c6;
  m_escapes[QSL("&clubs;")] = 0x2663;
  m_escapes[QSL("&cong;")] = 0x2245;
  m_escapes[QSL("&copy;")] = 0x00a9;
  m_escapes[QSL("&crarr;")] = 0x21b5;
  m_escapes[QSL("&cup;")] = 0x222a;
  m_escapes[QSL("&curren;")] = 0x00a4;
  m_escapes[QSL("&dArr;")] = 0x21d3;
  m_escapes[QSL("&dagger;")] = 0x2020;
  m_escapes[QSL("&darr;")] = 0x2193;
  m_escapes[QSL("&deg;")] = 0x00b0;
  m_escapes[QSL("&delta;")] = 0x03b4;
  m_escapes[QSL("&diams;")] = 0x2666;
  m_escapes[QSL("&divide;")] = 0x00f7;
  m_escapes[QSL("&eacute;")] = 0x00e9;
  m_escapes[QSL("&ecirc;")] = 0x00ea;
  m_escapes[QSL("&egrave;")] = 0x00e8;
  m_escapes[QSL("&empty;")] = 0x2205;
  m_escapes[QSL("&emsp;")] = 0x2003;
  m_escapes[QSL("&ensp;")] = 0x2002;
  m_escapes[QSL("&epsilon;")] = 0x03b5;
  m_escapes[QSL("&equiv;")] = 0x2261;
  m_escapes[QSL("&eta;")] = 0x03b7;
  m_escapes[QSL("&eth;")] = 0x00f0;
  m_escapes[QSL("&euml;")] = 0x00eb;
  m_escapes[QSL("&euro;")] = 0x20ac;
  m_escapes[QSL("&exist;")] = 0x2203;
  m_escapes[QSL("&fnof;")] = 0x0192;
  m_escapes[QSL("&forall;")] = 0x2200;
  m_escapes[QSL("&frac12;")] = 0x00bd;
  m_escapes[QSL("&frac14;")] = 0x00bc;
  m_escapes[QSL("&frac34;")] = 0x00be;
  m_escapes[QSL("&frasl;")] = 0x2044;
  m_escapes[QSL("&gamma;")] = 0x03b3;
  m_escapes[QSL("&ge;")] = 0x2265;
  m_escapes[QSL("&gt;")] = 62;
  m_escapes[QSL("&hArr;")] = 0x21d4;
  m_escapes[QSL("&harr;")] = 0x2194;
  m_escapes[QSL("&hearts;")] = 0x2665;
  m_escapes[QSL("&hellip;")] = 0x2026;
  m_escapes[QSL("&iacute;")] = 0x00ed;
  m_escapes[QSL("&icirc;")] = 0x00ee;
  m_escapes[QSL("&iexcl;")] = 0x00a1;
  m_escapes[QSL("&igrave;")] = 0x00ec;
  m_escapes[QSL("&image;")] = 0x2111;
  m_escapes[QSL("&infin;")] = 0x221e;
  m_escapes[QSL("&int;")] = 0x222b;
  m_escapes[QSL("&iota;")] = 0x03b9;
  m_escapes[QSL("&iquest;")] = 0x00bf;
  m_escapes[QSL("&isin;")] = 0x2208;
  m_escapes[QSL("&iuml;")] = 0x00ef;
  m_escapes[QSL("&kappa;")] = 0x03ba;
  m_escapes[QSL("&lArr;")] = 0x21d0;
  m_escapes[QSL("&lambda;")] = 0x03bb;
  m_escapes[QSL("&lang;")] = 0x2329;
  m_escapes[QSL("&laquo;")] = 0x00ab;
  m_escapes[QSL("&larr;")] = 0x2190;
  m_escapes[QSL("&lceil;")] = 0x2308;
  m_escapes[QSL("&ldquo;")] = 0x201c;
  m_escapes[QSL("&le;")] = 0x2264;
  m_escapes[QSL("&lfloor;")] = 0x230a;
  m_escapes[QSL("&lowast;")] = 0x2217;
  m_escapes[QSL("&loz;")] = 0x25ca;
  m_escapes[QSL("&lrm;")] = 0x200e;
  m_escapes[QSL("&lsaquo;")] = 0x2039;
  m_escapes[QSL("&lsquo;")] = 0x2018;
  m_escapes[QSL("&lt;")] = 60;
  m_escapes[QSL("&macr;")] = 0x00af;
  m_escapes[QSL("&mdash;")] = 0x2014;
  m_escapes[QSL("&micro;")] = 0x00b5;
  m_escapes[QSL("&middot;")] = 0x00b7;
  m_escapes[QSL("&minus;")] = 0x2212;
  m_escapes[QSL("&mu;")] = 0x03bc;
  m_escapes[QSL("&nabla;")] = 0x2207;
  m_escapes[QSL("&nbsp;")] = 0x00a0;
  m_escapes[QSL("&ndash;")] = 0x2013;
  m_escapes[QSL("&ne;")] = 0x2260;
  m_escapes[QSL("&ni;")] = 0x220b;
  m_escapes[QSL("&not;")] = 0x00ac;
  m_escapes[QSL("&notin;")] = 0x2209;
  m_escapes[QSL("&nsub;")] = 0x2284;
  m_escapes[QSL("&ntilde;")] = 0x00f1;
  m_escapes[QSL("&nu;")] = 0x03bd;
  m_escapes[QSL("&oacute;")] = 0x00f3;
  m_escapes[QSL("&ocirc;")] = 0x00f4;
  m_escapes[QSL("&oelig;")] = 0x0153;
  m_escapes[QSL("&ograve;")] = 0x00f2;
  m_escapes[QSL("&oline;")] = 0x203e;
  m_escapes[QSL("&omega;")] = 0x03c9;
  m_escapes[QSL("&omicron;")] = 0x03bf;
  m_escapes[QSL("&oplus;")] = 0x2295;
  m_escapes[QSL("&or;")] = 0x22a6;
  m_escapes[QSL("&ordf;")] = 0x00aa;
  m_escapes[QSL("&ordm;")] = 0x00ba;
  m_escapes[QSL("&oslash;")] = 0x00f8;
  m_escapes[QSL("&otilde;")] = 0x00f5;
  m_escapes[QSL("&otimes;")] = 0x2297;
  m_escapes[QSL("&ouml;")] = 0x00f6;
  m_escapes[QSL("&para;")] = 0x00b6;
  m_escapes[QSL("&part;")] = 0x2202;
  m_escapes[QSL("&percnt;")] = 0x0025;
  m_escapes[QSL("&permil;")] = 0x2030;
  m_escapes[QSL("&perp;")] = 0x22a5;
  m_escapes[QSL("&phi;")] = 0x03c6;
  m_escapes[QSL("&pi;")] = 0x03c0;
  m_escapes[QSL("&piv;")] = 0x03d6;
  m_escapes[QSL("&plusmn;")] = 0x00b1;
  m_escapes[QSL("&pound;")] = 0x00a3;
  m_escapes[QSL("&prime;")] = 0x2032;
  m_escapes[QSL("&prod;")] = 0x220f;
  m_escapes[QSL("&prop;")] = 0x221d;
  m_escapes[QSL("&psi;")] = 0x03c8;
  m_escapes[QSL("&quot;")] = 34;
  m_escapes[QSL("&rArr;")] = 0x21d2;
  m_escapes[QSL("&radic;")] = 0x221a;
  m_escapes[QSL("&rang;")] = 0x232a;
  m_escapes[QSL("&raquo;")] = 0x00bb;
  m_escapes[QSL("&rarr;")] = 0x2192;
  m_escapes[QSL("&rceil;")] = 0x2309;
  m_escapes[QSL("&rdquo;")] = 0x201d;
  m_escapes[QSL("&real;")] = 0x211c;
  m_escapes[QSL("&reg;")] = 0x00ae;
  m_escapes[QSL("&rfloor;")] = 0x230b;
  m_escapes[QSL("&rho;")] = 0x03c1;
  m_escapes[QSL("&rlm;")] = 0x200f;
  m_escapes[QSL("&rsaquo;")] = 0x203a;
  m_escapes[QSL("&rsquo;")] = 0x2019;
  m_escapes[QSL("&sbquo;")] = 0x201a;
  m_escapes[QSL("&scaron;")] = 0x0161;
  m_escapes[QSL("&sdot;")] = 0x22c5;
  m_escapes[QSL("&sect;")] = 0x00a7;
  m_escapes[QSL("&shy;")] = 0x00ad;
  m_escapes[QSL("&sigma;")] = 0x03c3;
  m_escapes[QSL("&sigmaf;")] = 0x03c2;
  m_escapes[QSL("&sim;")] = 0x223c;
  m_escapes[QSL("&spades;")] = 0x2660;
  m_escapes[QSL("&sub;")] = 0x2282;
  m_escapes[QSL("&sube;")] = 0x2286;
  m_escapes[QSL("&sum;")] = 0x2211;
  m_escapes[QSL("&sup;")] = 0x2283;
  m_escapes[QSL("&sup1;")] = 0x00b9;
  m_escapes[QSL("&sup2;")] = 0x00b2;
  m_escapes[QSL("&sup3;")] = 0x00b3;
  m_escapes[QSL("&supe;")] = 0x2287;
  m_escapes[QSL("&szlig;")] = 0x00df;
  m_escapes[QSL("&tau;")] = 0x03c4;
  m_escapes[QSL("&there4;")] = 0x2234;
  m_escapes[QSL("&theta;")] = 0x03b8;
  m_escapes[QSL("&thetasym;")] = 0x03d1;
  m_escapes[QSL("&thinsp;")] = 0x2009;
  m_escapes[QSL("&thorn;")] = 0x00fe;
  m_escapes[QSL("&tilde;")] = 0x02dc;
  m_escapes[QSL("&times;")] = 0x00d7;
  m_escapes[QSL("&trade;")] = 0x2122;
  m_escapes[QSL("&uArr;")] = 0x21d1;
  m_escapes[QSL("&uacute;")] = 0x00fa;
  m_escapes[QSL("&uarr;")] = 0x2191;
  m_escapes[QSL("&ucirc;")] = 0x00fb;
  m_escapes[QSL("&ugrave;")] = 0x00f9;
  m_escapes[QSL("&uml;")] = 0x00a8;
  m_escapes[QSL("&upsih;")] = 0x03d2;
  m_escapes[QSL("&upsilon;")] = 0x03c5;
  m_escapes[QSL("&uuml;")] = 0x00fc;
  m_escapes[QSL("&weierp;")] = 0x2118;
  m_escapes[QSL("&xi;")] = 0x03be;
  m_escapes[QSL("&yacute;")] = 0x00fd;
  m_escapes[QSL("&yen;")] = 0x00a5;
  m_escapes[QSL("&yuml;")] = 0x00ff;
  m_escapes[QSL("&zeta;")] = 0x03b6;
  m_escapes[QSL("&zwj;")] = 0x200d;
  m_escapes[QSL("&zwnj;")] = 0x200c;
  m_escapes[QSL("&#039;")] = 0x27;
}
