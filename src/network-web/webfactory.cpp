#include "network-web/webfactory.h"

#include "miscellaneous/application.h"

#include <QRegExp>
#include <QWebEngineSettings>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>


QPointer<WebFactory> WebFactory::s_instance;

WebFactory::WebFactory(QObject *parent)
  : QObject(parent), m_escapes(QMap<QString, QString>()),
    m_deEscapes(QMap<QString, QString>()),
    m_globalSettings(QWebEngineSettings::globalSettings()) {
}

WebFactory::~WebFactory() {
}

void WebFactory::loadState() {
  const Settings *settings = qApp->settings();

  switchJavascript(settings->value(GROUP(Browser), SETTING(Browser::JavascriptEnabled)).toBool(), false);
  switchImages(settings->value(GROUP(Browser), SETTING(Browser::ImagesEnabled)).toBool(), false);
  switchPlugins(settings->value(GROUP(Browser), SETTING(Browser::PluginsEnabled)).toBool(), false);
}

bool WebFactory::sendMessageViaEmail(const Message &message) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool()) {
    const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString();
    const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString();

    return QProcess::startDetached(QString("\"") + browser + QSL("\" ") + arguments.arg(message.m_title,
                                                                                        stripTags(message.m_contents)));
  }
  else {
    // Send it via mailto protocol.
    // NOTE: http://en.wikipedia.org/wiki/Mailto
    return QDesktopServices::openUrl(QString("mailto:?subject=%1&body=%2").arg(QString(QUrl::toPercentEncoding(message.m_title)),
                                                                               QString(QUrl::toPercentEncoding(stripTags(message.m_contents)))));
  }
}

bool WebFactory::openUrlInExternalBrowser(const QString &url) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
    const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
    const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();

    const QString call_line = "\"" + browser + "\" \"" + arguments.arg(url) + "\"";

    qDebug("Running command '%s'.", qPrintable(call_line));

    const bool result = QProcess::startDetached(call_line);

    if (!result) {
      qDebug("External web browser call failed.");
    }

    return result;
  }
  else {
    return QDesktopServices::openUrl(url);
  }
}

void WebFactory::switchJavascript(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::JavascriptEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, enable);
  emit javascriptSwitched(enable);
}

void WebFactory::switchPlugins(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::PluginsEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebEngineSettings::PluginsEnabled, enable);
  emit pluginsSwitched(enable);
}

void WebFactory::switchImages(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::ImagesEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebEngineSettings::AutoLoadImages, enable);
  emit imagesLoadingSwitched(enable);
}

WebFactory *WebFactory::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebFactory(qApp);
  }

  return s_instance;
}

bool WebFactory::javascriptEnabled() const {
  return m_globalSettings->testAttribute(QWebEngineSettings::JavascriptEnabled);
}

bool WebFactory::pluginsEnabled() const {
  return m_globalSettings->testAttribute(QWebEngineSettings::PluginsEnabled);
}

bool WebFactory::autoloadImages() const {
  return m_globalSettings->testAttribute(QWebEngineSettings::AutoLoadImages);
}

QString WebFactory::stripTags(QString text) {
  return text.remove(QRegExp(QSL("<[^>]*>")));
}

QString WebFactory::escapeHtml(const QString &html) {
  if (m_escapes.isEmpty()) {
    generetaEscapes();
  }

  QString output = html;

  foreach (const QString &key, m_escapes.keys()) {
    output = output.replace(key, m_escapes.value(key));
  }

  return output;
}

QString WebFactory::deEscapeHtml(const QString &text) {
  if (m_deEscapes.isEmpty()) {
    generateDeescapes();
  }

  QString output = text;

  foreach (const QString &key, m_deEscapes.keys()) {
    output = output.replace(key, m_deEscapes.value(key));
  }

  return output;
}

QString WebFactory::toSecondLevelDomain(const QUrl &url) {
#if QT_VERSION >= 0x040800
  const QString top_level_domain = url.topLevelDomain();
  const QString url_host = url.host();

  if (top_level_domain.isEmpty() || url_host.isEmpty()) {
    return QString();
  }

  QString domain = url_host.left(url_host.size() - top_level_domain.size());

  if (domain.count(QL1C('.')) == 0) {
    return url_host;
  }

  while (domain.count(QL1C('.')) != 0) {
    domain = domain.mid(domain.indexOf(QL1C('.')) + 1);
  }

  return domain + top_level_domain;
#else
  QString domain = url.host();

  if (domain.count(QL1C('.')) == 0) {
    return QString();
  }

  while (domain.count(QL1C('.')) != 1) {
    domain = domain.mid(domain.indexOf(QL1C('.')) + 1);
  }

  return domain;
#endif
}

void WebFactory::generetaEscapes() {
  m_escapes[QSL("&lt;")]     = QL1C('<');
  m_escapes[QSL("&gt;")]     = QL1C('>');
  m_escapes[QSL("&amp;")]    = QL1C('&');
  m_escapes[QSL("&quot;")]   = QL1C('\"');
  m_escapes[QSL("&nbsp;")]   = QL1C(' ');
  m_escapes[QSL("&plusmn;")]	= QSL("±");
  m_escapes[QSL("&times;")]  = QSL("×");
  m_escapes[QSL("&#039;")]   = QL1C('\'');
}

void WebFactory::generateDeescapes() {
  m_deEscapes[QSL("<")]  = QSL("&lt;");
  m_deEscapes[QSL(">")]  = QSL("&gt;");
  m_deEscapes[QSL("&")]  = QSL("&amp;");
  m_deEscapes[QSL("\"")]	= QSL("&quot;");
  m_deEscapes[QSL("±")]  = QSL("&plusmn;");
  m_deEscapes[QSL("×")]  = QSL("&times;");
  m_deEscapes[QSL("\'")] = QSL("&#039;");
}
