// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/rssparser.h"

#include "librssguard/exceptions/applicationexception.h"
#include "librssguard/miscellaneous/application.h"
#include "librssguard/miscellaneous/settings.h"
#include "librssguard/miscellaneous/textfactory.h"
#include "librssguard/network-web/networkfactory.h"
#include "src/definitions.h"
#include "src/standardfeed.h"

#include <QDomDocument>
#include <QTextCodec>
#include <QTextStream>

RssParser::RssParser(const QString& data) : FeedParser(data) {}

RssParser::~RssParser() {}

QList<StandardFeed*> RssParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);

  if (!base_result.isEmpty()) {
    return base_result;
  }

  QString my_url = url.toString();
  QList<StandardFeed*> feeds;

  // 1. Test direct URL for a feed.
  // 2. Test embedded RSS feed links from HTML data.
  // 3. Test "URL/feed" endpoint.
  // 4. Test "URL/rss" endpoint.

  // Download URL.
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
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
      // 1.
      auto guessed_feed = guessFeed(data, res.m_contentType);

      guessed_feed.first->setSource(my_url);

      return {guessed_feed.first};
    }
    catch (...) {
      qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }

    // 2.
    static QRegularExpression rx(QSL(RSS_REGEX_MATCHER), QRegularExpression::PatternOption::CaseInsensitiveOption);
    static QRegularExpression rx_href(QSL(RSS_HREF_REGEX_MATCHER),
                                      QRegularExpression::PatternOption::CaseInsensitiveOption);

    rx_href.optimize();

    QRegularExpressionMatchIterator it_rx = rx.globalMatch(QString::fromUtf8(data));

    while (it_rx.hasNext()) {
      QRegularExpressionMatch mat_tx = it_rx.next();
      QString link_tag = mat_tx.captured();
      QString feed_link = rx_href.match(link_tag).captured(1);

      if (feed_link.startsWith(QL1S("//"))) {
        feed_link = QSL(URI_SCHEME_HTTP) + feed_link.mid(2);
      }
      else if (feed_link.startsWith(QL1C('/'))) {
        feed_link = url.toString(QUrl::UrlFormattingOption::RemovePath | QUrl::UrlFormattingOption::RemoveQuery |
                                 QUrl::UrlFormattingOption::StripTrailingSlash) +
                    feed_link;
      }

      QByteArray data;
      auto res = NetworkFactory::performNetworkOperation(feed_link,
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

          guessed_feed.first->setSource(feed_link);
          feeds.append(guessed_feed.first);
        }
        catch (const ApplicationException& ex) {
          qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(feed_link)
                   << " should be direct link to feed file but was not recognized:" << QUOTE_W_SPACE_DOT(ex.message());
        }
      }
    }
  }

  // 3.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/feed");
  res = NetworkFactory::performNetworkOperation(my_url,
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
      feeds.append(guessed_feed.first);
    }
    catch (...) {
      qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }

  // 4.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/rss");
  res = NetworkFactory::performNetworkOperation(my_url,
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
      feeds.append(guessed_feed.first);
    }
    catch (...) {
      qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }

  return feeds;
}

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

  if (date_created.isNull()) {
    date_created = TextFactory::parseDateTime(msg_element.namedItem(QSL("dc:modified")).toElement().text());
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
