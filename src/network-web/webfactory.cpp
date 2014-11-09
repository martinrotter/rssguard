#include "network-web/webfactory.h"

#include "miscellaneous/application.h"

#include <QRegExp>
#include <QWebSettings>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>


QPointer<WebFactory> WebFactory::s_instance;

WebFactory::WebFactory(QObject *parent)
  : QObject(parent), m_escapes(QMap<QString, QString>()),
    m_deEscapes(QMap<QString, QString>()),
    m_globalSettings(QWebSettings::globalSettings()) {
}

WebFactory::~WebFactory() {
  qDebug("Destroying WebFactory instance.");
}

void WebFactory::loadState() {
  Settings *settings = qApp->settings();

  switchJavascript(settings->value(GROUP(Browser), SETTING(Browser::JavascriptEnabled)).toBool(), false);
  switchImages(settings->value(GROUP(Browser), SETTING(Browser::ImagesEnabled)).toBool(), false);
  switchPlugins(settings->value(GROUP(Browser), SETTING(Browser::PluginsEnabled)).toBool(), false);
}

bool WebFactory::openUrlInExternalBrowser(const QString &url) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
    QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
    QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();

    return QProcess::startDetached(browser, QStringList() << arguments.arg(url));
  }
  else {
    return QDesktopServices::openUrl(url);
  }
}

void WebFactory::switchJavascript(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::JavascriptEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebSettings::JavascriptEnabled, enable);
  emit javascriptSwitched(enable);
}

void WebFactory::switchPlugins(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::PluginsEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebSettings::PluginsEnabled, enable);
  emit pluginsSwitched(enable);
}

void WebFactory::switchImages(bool enable, bool save_settings) {
  if (save_settings) {
    qApp->settings()->setValue(GROUP(Browser), Browser::ImagesEnabled, enable);
  }

  m_globalSettings->setAttribute(QWebSettings::AutoLoadImages, enable);
  emit imagesLoadingSwitched(enable);
}

WebFactory *WebFactory::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebFactory(qApp);
  }

  return s_instance;
}

bool WebFactory::javascriptEnabled() const {
  return m_globalSettings->testAttribute(QWebSettings::JavascriptEnabled);
}

bool WebFactory::pluginsEnabled() const {
  return m_globalSettings->testAttribute(QWebSettings::PluginsEnabled);
}

bool WebFactory::autoloadImages() const {
  return m_globalSettings->testAttribute(QWebSettings::AutoLoadImages);
}

QString WebFactory::stripTags(QString text) {
  return text.remove(QRegExp("<[^>]*>"));
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

void WebFactory::generetaEscapes() {
  m_escapes["&lt;"]     = '<';
  m_escapes["&gt;"]     = '>';
  m_escapes["&amp;"]    = '&';
  m_escapes["&quot;"]   = '\"';
  m_escapes["&nbsp;"]   = ' ';
  m_escapes["&plusmn;"]	= "±";
  m_escapes["&times;"]  = "×";
  m_escapes["&#039;"]   = '\'';
}

void WebFactory::generateDeescapes() {
  m_deEscapes["<"]  = "&lt;";
  m_deEscapes[">"]  = "&gt;";
  m_deEscapes["&"]  = "&amp;";
  m_deEscapes["\""]	= "&quot;";
  m_deEscapes["±"]  = "&plusmn;";
  m_deEscapes["×"]  = "&times;";
  m_deEscapes["\'"] = "&#039;";
}
