#include "core/webfactory.h"

#include <QApplication>
#include <QRegExp>


QPointer<WebFactory> WebFactory::s_instance;

WebFactory::WebFactory(QObject *parent) : QObject(parent) {
}

WebFactory::~WebFactory() {
  qDebug("Destroying WebFactory instance.");
}

WebFactory *WebFactory::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebFactory(qApp);
  }

  return s_instance;
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
