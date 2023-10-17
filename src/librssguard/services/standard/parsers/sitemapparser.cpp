// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/sitemapparser.h"

#if defined(ENABLE_COMPRESSED_SITEMAP)
#include "3rd-party/qcompressor/qcompressor.h"
#endif

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>
#include <QTextCodec>
#include <QTextStream>

SitemapParser::SitemapParser(const QString& data) : FeedParser(data) {}

SitemapParser::~SitemapParser() {}

QList<StandardFeed*> SitemapParser::discoverFeeds(ServiceRoot* root, const QUrl& url) const {
  return {};
}

QPair<StandardFeed*, QList<IconLocation>> SitemapParser::guessFeed(const QByteArray& content,
                                                                   const QString& content_type) const {
  QByteArray uncompressed_content;

  if (isGzip(content)) {
#if defined(ENABLE_COMPRESSED_SITEMAP)
    QCompressor::gzipDecompress(content, uncompressed_content);
#else
    throw FeedRecognizedButFailedException(QObject::tr("support for gzipped sitemaps is not enabled"));
#endif
  }
  else {
    uncompressed_content = content;
  }

  QString xml_schema_encoding = QSL(DEFAULT_FEED_ENCODING);
  QString xml_contents_encoded;
  QString enc =
    QRegularExpression(QSL("encoding=\"([A-Z0-9\\-]+)\""), QRegularExpression::PatternOption::CaseInsensitiveOption)
      .match(uncompressed_content)
      .captured(1);

  if (!enc.isEmpty()) {
    // Some "encoding" attribute was found get the encoding
    // out of it.
    xml_schema_encoding = enc;
  }

  QTextCodec* custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());

  if (custom_codec != nullptr) {
    xml_contents_encoded = custom_codec->toUnicode(uncompressed_content);
  }
  else {
    xml_contents_encoded = QString::fromUtf8(uncompressed_content);
  }

  // Feed XML was obtained, guess it now.
  QDomDocument xml_document;
  QString error_msg;
  int error_line, error_column;

  if (!xml_document.setContent(xml_contents_encoded, true, &error_msg, &error_line, &error_column)) {
    throw ApplicationException(QObject::tr("XML is not well-formed, %1").arg(error_msg));
  }

  QDomElement root_element = xml_document.documentElement();

  if (root_element.tagName() == QSL("sitemapindex")) {
    throw FeedRecognizedButFailedException(QObject::tr("sitemap indices are not supported"));
  }

  if (root_element.tagName() != QSL("urlset")) {
    throw ApplicationException(QObject::tr("not a Sitemap"));
  }

  auto* feed = new StandardFeed();
  QList<IconLocation> icon_possible_locations;

  feed->setEncoding(xml_schema_encoding);
  feed->setType(StandardFeed::Type::Sitemap);
  feed->setTitle(StandardFeed::typeToString(StandardFeed::Type::Sitemap));

  return {feed, icon_possible_locations};
}

QString SitemapParser::sitemapNamespace() const {
  return QSL("http://www.sitemaps.org/schemas/sitemap/0.9");
}

QString SitemapParser::sitemapNewsNamespace() const {
  return QSL("http://www.google.com/schemas/sitemap-news/0.9");
}

QString SitemapParser::sitemapImageNamespace() const {
  return QSL("http://www.google.com/schemas/sitemap-image/1.1");
}

QString SitemapParser::sitemapVideoNamespace() const {
  return QSL("http://www.google.com/schemas/sitemap-video/1.1");
}

QDomNodeList SitemapParser::xmlMessageElements() {
  return m_xml.elementsByTagNameNS(sitemapNamespace(), QSL("url"));
}

QString SitemapParser::xmlMessageTitle(const QDomElement& msg_element) const {
  QString str_title = msg_element.elementsByTagNameNS(sitemapNewsNamespace(), QSL("title")).at(0).toElement().text();

  if (str_title.isEmpty()) {
    str_title = msg_element.elementsByTagNameNS(sitemapVideoNamespace(), QSL("title")).at(0).toElement().text();
  }

  return str_title;
}

QString SitemapParser::xmlMessageUrl(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(sitemapNamespace(), QSL("loc")).at(0).toElement().text();
}

QString SitemapParser::xmlMessageDescription(const QDomElement& msg_element) const {
  return xmlRawChild(msg_element.elementsByTagNameNS(sitemapVideoNamespace(), QSL("description")).at(0).toElement());
}

QDateTime SitemapParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  QString str_date = msg_element.elementsByTagNameNS(sitemapNamespace(), QSL("lastmod")).at(0).toElement().text();

  if (str_date.isEmpty()) {
    str_date =
      msg_element.elementsByTagNameNS(sitemapNewsNamespace(), QSL("publication_date")).at(0).toElement().text();
  }

  return TextFactory::parseDateTime(str_date);
}

QString SitemapParser::xmlMessageId(const QDomElement& msg_element) const {
  return xmlMessageUrl(msg_element);
}

QList<Enclosure> SitemapParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  QList<Enclosure> enclosures;

  // sitemap-image
  QDomNodeList elem_links = msg_element.elementsByTagNameNS(sitemapImageNamespace(), QSL("image"));

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString loc = link.elementsByTagNameNS(sitemapImageNamespace(), QSL("loc")).at(0).toElement().text();

    if (!loc.isEmpty()) {
      // NOTE: The MIME is made up.
      enclosures.append(Enclosure(loc, QSL("image/png")));
    }
  }

  // sitemap-video
  elem_links = msg_element.elementsByTagNameNS(sitemapVideoNamespace(), QSL("video"));

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString loc = link.elementsByTagNameNS(sitemapVideoNamespace(), QSL("player_loc")).at(0).toElement().text();

    if (loc.isEmpty()) {
      loc = link.elementsByTagNameNS(sitemapVideoNamespace(), QSL("content_loc")).at(0).toElement().text();
    }

    if (!loc.isEmpty()) {
      // NOTE: The MIME is made up.
      enclosures.append(Enclosure(loc, QSL("video/mpeg")));
    }
  }

  return enclosures;
}

bool SitemapParser::isGzip(const QByteArray& content) {
  return ((content[0] & 0xFF) == 0x1f) && ((content[1] & 0xFF) == 0x8b);
}
