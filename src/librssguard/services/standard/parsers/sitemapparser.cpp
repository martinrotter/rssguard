// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/sitemapparser.h"

#if defined(ENABLE_COMPRESSED_SITEMAP)
#include "3rd-party/qcompressor/qcompressor.h"
#endif

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "services/standard/definitions.h"
#include "services/standard/standardfeed.h"

#include <QDomDocument>
#include <QTextCodec>
#include <QTextStream>

SitemapParser::SitemapParser(const QString& data) : FeedParser(data) {}

SitemapParser::~SitemapParser() {}

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

  if (root_element.tagName() != QSL("urlset") && root_element.tagName() != QSL("sitemapindex")) {
    throw ApplicationException(QObject::tr("not a Sitemap"));
  }

  auto* feed = new StandardFeed();
  QList<IconLocation> icon_possible_locations;

  feed->setEncoding(xml_schema_encoding);

  if (root_element.tagName() == QSL("urlset")) {
    // Sitemap.
    feed->setType(StandardFeed::Type::Sitemap);
    feed->setTitle(StandardFeed::typeToString(StandardFeed::Type::Sitemap));
  }
  else {
    // Sitemap index.
    feed->setType(StandardFeed::Type::SitemapIndex);
    feed->setTitle(StandardFeed::typeToString(StandardFeed::Type::SitemapIndex));
  }

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
  return {};
}

// TODO: implement

QString SitemapParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return {};
}

QString SitemapParser::xmlMessageUrl(const QDomElement& msg_element) const {
  return {};
}

QString SitemapParser::xmlMessageDescription(const QDomElement& msg_element) const {
  return {};
}

QString SitemapParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  return {};
}

QDateTime SitemapParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  return {};
}

QString SitemapParser::xmlMessageId(const QDomElement& msg_element) const {
  return {};
}

QList<Enclosure> SitemapParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  return {};
}

QList<MessageCategory> SitemapParser::xmlMessageCategories(const QDomElement& msg_element) const {
  return {};
}

QString SitemapParser::xmlMessageRawContents(const QDomElement& msg_element) const {
  return {};
}

bool SitemapParser::isGzip(const QByteArray& content) {
  return ((content[0] & 0xFF) == 0x1f) && ((content[1] & 0xFF) == 0x8b);
}
