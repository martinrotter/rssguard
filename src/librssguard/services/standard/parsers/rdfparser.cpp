// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/rdfparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"
#include "services/standard/standardfeed.h"

#include <QDomDocument>
#include <QTextCodec>

RdfParser::RdfParser(const QString& data)
  : FeedParser(data), m_rdfNamespace(QSL("http://www.w3.org/1999/02/22-rdf-syntax-ns#")),
    m_rssNamespace(QSL("http://purl.org/rss/1.0/")), m_rssCoNamespace(QSL("http://purl.org/rss/1.0/modules/content/")),
    m_dcElNamespace(QSL("http://purl.org/dc/elements/1.1/")) {}

RdfParser::~RdfParser() {}

QPair<StandardFeed*, QList<IconLocation>> RdfParser::guessFeed(const QByteArray& content,
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

  if (root_element.namespaceURI() != rdfNamespace()) {
    throw ApplicationException(QObject::tr("not an RDF feed"));
  }

  auto* feed = new StandardFeed();
  QList<IconLocation> icon_possible_locations;

  feed->setEncoding(xml_schema_encoding);
  feed->setType(StandardFeed::Type::Rdf);

  QDomElement channel_element = root_element.elementsByTagNameNS(rssNamespace(), QSL("channel")).at(0).toElement();

  feed->setTitle(channel_element.elementsByTagNameNS(rssNamespace(), QSL("title")).at(0).toElement().text());
  feed
    ->setDescription(channel_element.elementsByTagNameNS(rssNamespace(), QSL("description")).at(0).toElement().text());

  QString home_page = channel_element.elementsByTagNameNS(rssNamespace(), QSL("link")).at(0).toElement().text();

  if (!home_page.isEmpty()) {
    icon_possible_locations.prepend({home_page, false});
  }

  return {feed, icon_possible_locations};
}

QDomNodeList RdfParser::xmlMessageElements() {
  return m_xml.elementsByTagNameNS(m_rssNamespace, QSL("item"));
}

QString RdfParser::rssNamespace() const {
  return m_rssNamespace;
}

QString RdfParser::rdfNamespace() const {
  return m_rdfNamespace;
}

QString RdfParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_rssNamespace, QSL("title")).at(0).toElement().text();
}

QString RdfParser::xmlMessageDescription(const QDomElement& msg_element) const {
  QString description = msg_element.elementsByTagNameNS(m_rssCoNamespace, QSL("encoded")).at(0).toElement().text();

  if (description.simplified().isEmpty()) {
    description = msg_element.elementsByTagNameNS(m_rssNamespace, QSL("description")).at(0).toElement().text();
  }

  return description;
}

QString RdfParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("creator")).at(0).toElement().text();
}

QDateTime RdfParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  return TextFactory::parseDateTime(msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("date"))
                                      .at(0)
                                      .toElement()
                                      .text());
}

QString RdfParser::xmlMessageId(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("identifier")).at(0).toElement().text();
}

QString RdfParser::xmlMessageUrl(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_rssNamespace, QSL("link")).at(0).toElement().text();
}

QList<Enclosure> RdfParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  return {};
}
