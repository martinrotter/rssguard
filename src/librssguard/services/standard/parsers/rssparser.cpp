// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/rssparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"
#include "services/standard/standardfeed.h"

#include <QDomDocument>
#include <QTextCodec>
#include <QTextStream>

RssParser::RssParser(const QString& data) : FeedParser(data) {}

RssParser::~RssParser() {}

QPair<StandardFeed*, QList<IconLocation>> RssParser::guessFeed(const QByteArray& content,
                                                               const QString& content_type) const {
  QString xml_schema_encoding = QSL(DEFAULT_FEED_ENCODING);
  QString xml_contents_encoded;
  QString enc =
    QRegularExpression(QSL("encoding=\"([A-Z0-9\\-]+)\""), QRegularExpression::PatternOption::CaseInsensitiveOption)
      .match(content)
      .captured(1);

  if (!enc.isEmpty()) {
    // Some "encoding" attribute was found get the encoding
    // out of it.
    xml_schema_encoding = enc;
  }

  QTextCodec* custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());

  if (custom_codec != nullptr) {
    xml_contents_encoded = custom_codec->toUnicode(content);
  }
  else {
    xml_contents_encoded = QString::fromUtf8(content);
  }

  // Feed XML was obtained, guess it now.
  QDomDocument xml_document;
  QString error_msg;
  int error_line, error_column;

  if (!xml_document.setContent(xml_contents_encoded, true, &error_msg, &error_line, &error_column)) {
    throw ApplicationException(QObject::tr("XML is not well-formed, %1").arg(error_msg));
  }

  QDomElement root_element = xml_document.documentElement();

  if (root_element.tagName() != QL1S("rss")) {
    throw ApplicationException(QObject::tr("not a RSS feed"));
  }

  auto* feed = new StandardFeed();
  QList<IconLocation> icon_possible_locations;

  feed->setEncoding(xml_schema_encoding);

  QString rss_type = root_element.attribute(QSL("version"), QSL("2.0"));

  if (rss_type == QL1S("0.91") || rss_type == QL1S("0.92") || rss_type == QL1S("0.93")) {
    feed->setType(StandardFeed::Type::Rss0X);
  }
  else {
    feed->setType(StandardFeed::Type::Rss2X);
  }

  QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

  feed->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
  feed->setDescription(channel_element.namedItem(QSL("description")).toElement().text());

  QString icon_url_link = channel_element.namedItem(QSL("image")).namedItem(QSL("url")).toElement().text();

  if (!icon_url_link.isEmpty()) {
    icon_possible_locations.append({icon_url_link, true});
  }

  auto channel_links = channel_element.elementsByTagName(QSL("link"));

  for (int i = 0; i < channel_links.size(); i++) {
    QString home_page = channel_links.at(i).toElement().text();

    if (!home_page.isEmpty()) {
      icon_possible_locations.prepend({home_page, false});
      break;
    }
  }

  return {feed, icon_possible_locations};
}

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
