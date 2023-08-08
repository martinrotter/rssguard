// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/rssparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>
#include <QTextStream>

RssParser::RssParser(const QString& data) : FeedParser(data) {}

QDomNodeList RssParser::xmlMessageElements() {
  QDomNode channel_elem = m_xml.namedItem(QSL("rss")).namedItem(QSL("channel"));

  if (channel_elem.isNull()) {
    return QDomNodeList();
  }
  else {
    return channel_elem.toElement().elementsByTagName(QSL("item"));
  }
}

QString RssParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return msg_element.namedItem(QSL("title")).toElement().text();
}

QString RssParser::xmlMessageDescription(const QDomElement& msg_element) const {
  QString description = xmlRawChild(msg_element.elementsByTagName(QSL("encoded")).at(0).toElement());

  if (description.isEmpty()) {
    description = xmlRawChild(msg_element.elementsByTagName(QSL("description")).at(0).toElement());
  }

  return description;
}

QString RssParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  QString author = msg_element.namedItem(QSL("author")).toElement().text();

  if (author.isEmpty()) {
    author = msg_element.namedItem(QSL("creator")).toElement().text();
  }

  return author;
}

QDateTime RssParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  QDateTime date_created = TextFactory::parseDateTime(msg_element.namedItem(QSL("pubDate")).toElement().text());

  if (date_created.isNull()) {
    date_created = TextFactory::parseDateTime(msg_element.namedItem(QSL("date")).toElement().text());
  }

  return date_created;
}

QString RssParser::xmlMessageId(const QDomElement& msg_element) const {
  return msg_element.namedItem(QSL("guid")).toElement().text();
}

QString RssParser::xmlMessageUrl(const QDomElement& msg_element) const {
  QString url = msg_element.namedItem(QSL("link")).toElement().text();

  if (url.isEmpty()) {
    // Try to get "href" attribute.
    url = msg_element.namedItem(QSL("link")).toElement().attribute(QSL("href"));
  }

  if (url.isEmpty()) {
    // Fallback non-valid "url" elements.
    url = msg_element.namedItem(QSL("url")).toElement().text();
  }

  return url;
}

QList<Enclosure> RssParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  QString elem_enclosure = msg_element.namedItem(QSL("enclosure")).toElement().attribute(QSL("url"));
  QString elem_enclosure_type = msg_element.namedItem(QSL("enclosure")).toElement().attribute(QSL("type"));

  if (!elem_enclosure.isEmpty()) {
    return {Enclosure(elem_enclosure, elem_enclosure_type)};
  }
  else {
    return {};
  }
}
