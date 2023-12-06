// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/sitemapparser.h"

#if defined(ENABLE_COMPRESSED_SITEMAP)
#include "3rd-party/qcompressor/qcompressor.h"
#endif

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>
#include <QTextCodec>
#include <QTextStream>

SitemapParser::SitemapParser(const QString& data) : FeedParser(data) {}

SitemapParser::~SitemapParser() {}

QList<StandardFeed*> SitemapParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);
  QHash<QString, StandardFeed*> feeds;

  if (!base_result.isEmpty()) {
    if (greedy) {
      for (StandardFeed* base_fd : base_result) {
        feeds.insert(base_fd->source(), base_fd);
      }
    }
    else {
      return base_result;
    }
  }

  QStringList to_process_sitemaps;
  int sitemap_index_limit = 2;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  // 1. Direct URL test. If sitemap index, process its children. If found, stop if non-recursive
  //    discovery is chosen.
  // 2. Process "URL/robots.txt" file.
  // 3. Process "URLHOST/robots.txt" file.
  // 4. Test "URL/sitemap.xml" endpoint.
  // 5. Test "URL/sitemap.xml.gz" endpoint.

  // 1.
  to_process_sitemaps.append(url.toString());

  // 2.
  // 3.
  QStringList to_process_robots = {
    url.toString(QUrl::UrlFormattingOption::StripTrailingSlash).replace(QRegularExpression(QSL("\\/$")), QString()) +
      QSL("/robots.txt"),
    url.toString(QUrl::UrlFormattingOption::RemovePath | QUrl::UrlFormattingOption::RemoveQuery) + QSL("/robots.txt")};

  to_process_robots.removeDuplicates();

  for (const QString& robots_url : to_process_robots) {
    // Download URL.
    QByteArray data;
    auto res = NetworkFactory::performNetworkOperation(robots_url,
                                                       timeout,
                                                       {},
                                                       data,
                                                       QNetworkAccessManager::Operation::GetOperation,
                                                       {},
                                                       {},
                                                       {},
                                                       {},
                                                       root->networkProxy());

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      QRegularExpression rx(QSL("Sitemap: ?([^\\r\\n]+)"),
                            QRegularExpression::PatternOption::CaseInsensitiveOption |
                              QRegularExpression::PatternOption::MultilineOption);
      QRegularExpressionMatchIterator it_rx = rx.globalMatch(QString::fromUtf8(data));

      while (it_rx.hasNext()) {
        QString sitemap_link = it_rx.next().captured(1);

        to_process_sitemaps.append(sitemap_link);
      }
    }
  }

  // 4.
  to_process_sitemaps.append(url.toString(QUrl::UrlFormattingOption::StripTrailingSlash)
                               .replace(QRegularExpression(QSL("\\/$")), QString()) +
                             QSL("/sitemap.xml"));

  // 5.
  to_process_sitemaps.append(url.toString(QUrl::UrlFormattingOption::StripTrailingSlash)
                               .replace(QRegularExpression(QSL("\\/$")), QString()) +
                             QSL("/sitemap.xml.gz"));

  while (!to_process_sitemaps.isEmpty()) {
    to_process_sitemaps.removeDuplicates();

    QString my_url = to_process_sitemaps.takeFirst();

    if (feeds.contains(my_url)) {
      continue;
    }

    // Download URL.
    QByteArray data;
    auto res = NetworkFactory::performNetworkOperation(my_url,
                                                       timeout,
                                                       {},
                                                       data,
                                                       QNetworkAccessManager::Operation::GetOperation,
                                                       {},
                                                       {},
                                                       {},
                                                       {},
                                                       root->networkProxy());

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      try {
        auto guessed_feed = guessFeed(data, res.m_contentType);

        guessed_feed.first->setSource(my_url);
        guessed_feed.first->setTitle(my_url);

        feeds.insert(my_url, guessed_feed.first);

        if (!greedy) {
          break;
        }
      }
      catch (const FeedRecognizedButFailedException& ex) {
        // This is index.
        if (sitemap_index_limit-- > 0) {
          to_process_sitemaps.append(ex.arbitraryData().toStringList());
        }
      }
      catch (const ApplicationException&) {
        qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(my_url) << "is not a direct sitemap file.";
      }
    }
  }

  return feeds.values();
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
    QStringList locs;
    int i = 0;

    for (QDomNodeList ndl = root_element.elementsByTagNameNS(sitemapNamespace(), QSL("loc")); i < ndl.size(); i++) {
      locs << ndl.at(i).toElement().text();
    }

    throw FeedRecognizedButFailedException(QObject::tr("sitemap indices are not supported"), locs);
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

  if (str_title.isEmpty()) {
    str_title = msg_element.elementsByTagNameNS(sitemapImageNamespace(), QSL("title")).at(0).toElement().text();
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
  return content.size() >= 2 && ((content[0] & 0xFF) == 0x1f) && ((content[1] & 0xFF) == 0x8b);
}
