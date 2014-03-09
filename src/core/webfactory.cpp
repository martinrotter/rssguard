#include "core/webfactory.h"

#include "core/defs.h"
#include "core/settings.h"

#include <QApplication>
#include <QRegExp>
#include <QWebSettings>


QPointer<WebFactory> WebFactory::s_instance;

WebFactory::WebFactory(QObject *parent) : QObject(parent) {
  m_globalSettings = QWebSettings::globalSettings();
}

WebFactory::~WebFactory() {
  qDebug("Destroying WebFactory instance.");
}

void WebFactory::loadState() {
  Settings *settings = Settings::instance();

  switchJavascript(settings->value(APP_CFG_BROWSER, "enable_javascript", true).toBool(),
                   false);
  switchImages(settings->value(APP_CFG_BROWSER, "enable_images", true).toBool(),
               false);
  switchPlugins(settings->value(APP_CFG_BROWSER, "enable_plugins", false).toBool(),
                false);
}

void WebFactory::switchJavascript(bool enable, bool save_settings) {
  if (save_settings) {
    Settings::instance()->setValue(APP_CFG_BROWSER,
                                   "enable_javascript",
                                   enable);
  }

  m_globalSettings->setAttribute(QWebSettings::JavascriptEnabled, enable);
  emit javascriptSwitched(enable);
}

void WebFactory::switchPlugins(bool enable, bool save_settings) {
  if (save_settings) {
    Settings::instance()->setValue(APP_CFG_BROWSER,
                                   "enable_plugins",
                                   enable);
  }

  m_globalSettings->setAttribute(QWebSettings::PluginsEnabled, enable);
  emit pluginsSwitched(enable);
}

void WebFactory::switchImages(bool enable, bool save_settings) {
  if (save_settings) {
    Settings::instance()->setValue(APP_CFG_BROWSER,
                                   "enable_images",
                                   enable);
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
  static QMap<QString, QString> escape_sequences = generetaEscapes();

  QList<QString> keys = escape_sequences.uniqueKeys();
  QString output = html;

  foreach (const QString &key, keys) {
    output.replace(key, escape_sequences.value(key));
  }

  return output;
}

QString WebFactory::deEscapeHtml(const QString &text) {
  static QMap<QString, QString> deescape_sequences = generateDeescapes();

  QList<QString> keys = deescape_sequences.uniqueKeys();
  QString output = text;

  foreach (const QString &key, keys) {
    output.replace(key, deescape_sequences.value(key));
  }

  return output;
}

QMap<QString, QString> WebFactory::generetaEscapes() {
  QMap<QString, QString> sequences;

  sequences["&lt;"]     = '<';
  sequences["&gt;"]     = '>';
  sequences["&amp;"]    = '&';
  sequences["&quot;"]   = '\"';
  sequences["&nbsp;"]   = ' ';
  sequences["&plusmn;"]	= "±";
  sequences["&times;"]  = "×";
  sequences["&#039;"]   = '\'';

  return sequences;
}

QMap<QString, QString> WebFactory::generateDeescapes() {
  QMap<QString, QString> sequences;

  sequences["<"]  = "&lt;";
  sequences[">"]  = "&gt;";
  sequences["&"]  = "&amp;";
  sequences["\""]	= "&quot;";
  sequences["±"]  = "&plusmn;";
  sequences["×"]  = "&times;";
  sequences["\'"] = "&#039;";

  return sequences;
}
