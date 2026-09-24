// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/wordpressjsonparser.h"

#include "src/definitions.h"
#include "src/standardfeed.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/definitions/typedefs.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/miscellaneous/textfactory.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

WordpressJsonParser::WordpressJsonParser(const QString& data, const QUrl& source_url)
  : FeedParser(data, DataType::Json), m_sourceUrl(source_url) {}

WordpressJsonParser::~WordpressJsonParser() {}

bool WordpressJsonParser::isWordpressItem(const QJsonObject& item) const {
  return item.contains(QSL("id")) && item.contains(QSL("date")) && item.contains(QSL("link")) &&
         item.contains(QSL("title")) && item[QSL("title")].isObject() && item.contains(QSL("content")) &&
         item[QSL("content")].isObject();
}

QList<StandardFeed*> WordpressJsonParser::discoverFeeds(ServiceRoot* root,
                                                        const QUrl& url,
                                                        bool deep_discovery,
                                                        const QList<DocumentWithUrl>& documents) const {
  QList<QPair<QByteArray, QByteArray>> headers = {
    {HTTP_HEADERS_ACCEPT, StandardFeed::idealHttpAcceptForFeedType(StandardFeed::Type::Json).toLocal8Bit()}};

  QString my_url = url.toString();
  QList<StandardFeed*> feeds;

  // 1. Test direct JSON parsing endpoint.
  // 2. Test "URL/wp-json/wp/v2/posts" endpoint.
  // 3. Test "URL/wp-json/wp/v2/pages" endpoint.

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  // 1.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash);
  QByteArray data;
  auto res = NetworkFactory::performNetworkOperation(my_url,
                                                     timeout,
                                                     {},
                                                     data,
                                                     QNetworkAccessManager::Operation::GetOperation,
                                                     headers,
                                                     {},
                                                     {},
                                                     {},
                                                     root->networkProxy());

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    try {
      auto guessed_feed = guessFeed(data, res);

      feeds.append(guessed_feed.m_feed);
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url)
               << "was not recognized as a WordPress REST API posts/pages feed.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  // 2.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/wp-json/wp/v2/posts");
  res = NetworkFactory::performNetworkOperation(my_url,
                                                timeout,
                                                {},
                                                data,
                                                QNetworkAccessManager::Operation::GetOperation,
                                                headers,
                                                {},
                                                {},
                                                {},
                                                root->networkProxy());

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    try {
      auto guessed_feed = guessFeed(data, res);

      feeds.append(guessed_feed.m_feed);
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "was not recognized as a WordPress REST API posts feed.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  // 3.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/wp-json/wp/v2/pages");
  res = NetworkFactory::performNetworkOperation(my_url,
                                                timeout,
                                                {},
                                                data,
                                                QNetworkAccessManager::Operation::GetOperation,
                                                headers,
                                                {},
                                                {},
                                                {},
                                                root->networkProxy());

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    try {
      auto guessed_feed = guessFeed(data, res);

      feeds.append(guessed_feed.m_feed);
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "was not recognized as a WordPress REST API pages feed.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  return feeds;
}

GuessedFeedWithIcons WordpressJsonParser::guessFeed(const QByteArray& content, const NetworkResult& network_res) const {
  if (network_res.m_contentType.contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive) ||
      content.simplified().startsWith('[')) {
    QJsonParseError json_err;
    QJsonDocument json = QJsonDocument::fromJson(content, &json_err);

    if (json.isNull() && !json_err.errorString().isEmpty()) {
      throw FeedRecognizedButFailedException(QObject::tr("JSON error '%1'").arg(json_err.errorString()));
    }

    if (!json.isArray() || json.array().isEmpty()) {
      throw ApplicationException(QObject::tr("not a WordPress REST API feed"));
    }

    const QJsonObject first_item = json.array().first().toObject();

    if (!isWordpressItem(first_item)) {
      throw ApplicationException(QObject::tr("not a WordPress REST API feed"));
    }

    auto* feed = new StandardFeed();

    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setType(StandardFeed::Type::WordpressJson);
    feed->setTitle(network_res.m_url.toString());
    feed->setSource(network_res.m_url.toString());

    return {feed, {}};
  }
  else {
    throw ApplicationException(QObject::tr("not a JSON Wordpress feed"));
  }
}

QString WordpressJsonParser::jsonMessageTitle(const QJsonObject& msg_element) const {
  return msg_element[QSL("title")].toObject()[QSL("rendered")].toString();
}

QString WordpressJsonParser::jsonMessageUrl(const QJsonObject& msg_element) const {
  return msg_element[QSL("link")].toString();
}

QString WordpressJsonParser::jsonMessageDescription(const QJsonObject& msg_element) const {
  return msg_element[QSL("content")].toObject()[QSL("rendered")].toString();
}

QString WordpressJsonParser::jsonMessageAuthor(const QJsonObject& msg_element) const {
  const auto embedded_authors = msg_element.value(QSL("_embedded")).toObject().value(QSL("author")).toArray();

  for (const QJsonValue& author : embedded_authors) {
    const QString name = author.toObject().value(QSL("name")).toString();

    if (!name.isEmpty()) {
      return name;
    }
  }

  const auto author_links = msg_element.value(QSL("_links")).toObject().value(QSL("author")).toArray();

  for (const QJsonValue& link : author_links) {
    const QJsonDocument author = linkedResource(link.toObject().value(QSL("href")).toString());
    const QString name = author.object().value(QSL("name")).toString();

    if (!name.isEmpty()) {
      return name;
    }
  }

  const QJsonValue author_id = msg_element.value(QSL("author"));
  return author_id.isDouble() ? QString::number(author_id.toInt()) : QString();
}

QList<QSharedPointer<MessageEnclosure>> WordpressJsonParser::jsonMessageEnclosures(const QJsonObject& msg_element)
  const {
  QList<QSharedPointer<MessageEnclosure>> enclosures;
  QSet<QString> seen_urls;

  const auto add_media = [&enclosures, &seen_urls](const QJsonValue& value) {
    const QJsonObject media = value.toObject();
    const QUrl url(media.value(QSL("source_url")).toString());

    if (!url.isValid() || (url.scheme() != QSL("http") && url.scheme() != QSL("https"))) {
      return;
    }

    const QString url_string = url.toString();

    if (seen_urls.contains(url_string)) {
      return;
    }

    seen_urls.insert(url_string);
    enclosures.append(QSharedPointer<MessageEnclosure>(new MessageEnclosure(url_string,
                                                                            media.value(QSL("mime_type")).toString())));
  };

  const auto embedded_media = msg_element.value(QSL("_embedded")).toObject().value(QSL("wp:attachment")).toArray();

  for (const QJsonValue& media : embedded_media) {
    if (media.isArray()) {
      for (const QJsonValue& item : media.toArray()) {
        add_media(item);
      }
    }
    else {
      add_media(media);
    }
  }

  const auto attachment_links = msg_element.value(QSL("_links")).toObject().value(QSL("wp:attachment")).toArray();

  for (const QJsonValue& link : attachment_links) {
    const QJsonDocument media = linkedResource(link.toObject().value(QSL("href")).toString());

    if (media.isArray()) {
      for (const QJsonValue& item : media.array()) {
        add_media(item);
      }
    }
    else if (media.isObject()) {
      add_media(media.object());
    }
  }

  return enclosures;
}

QDateTime WordpressJsonParser::jsonMessageDateCreated(const QJsonObject& msg_element) {
  QString published = msg_element[QSL("date_gmt")].toString();
  QString updated = msg_element[QSL("modified_gmt")].toString();

  if (!published.isEmpty()) {
    published += QL1C('Z');
  }

  if (!updated.isEmpty()) {
    updated += QL1C('Z');
  }

  return decideArticleDate(published, updated);
}

QString WordpressJsonParser::jsonMessageId(const QJsonObject& msg_element) const {
  return msg_element[QSL("guid")].toObject()[QSL("rendered")].toString();
}

QString WordpressJsonParser::jsonMessageRawContents(const QJsonObject& msg_element) const {
  return QJsonDocument(msg_element).toJson(QJsonDocument::JsonFormat::Compact);
}

QJsonArray WordpressJsonParser::jsonMessageElements() {
  return m_json.array();
}

QJsonDocument WordpressJsonParser::linkedResource(const QString& href) const {
  if (href.isEmpty() || !m_resourceHandler || !m_sourceUrl.isValid()) {
    return {};
  }

  const QUrl href_url(href);
  QUrl url = m_sourceUrl.resolved(href_url);

  // Resource requests reuse the feed's HTTP headers, including authentication.
  const QString scheme = url.scheme().toLower();
  const int default_port = scheme == QSL("https") ? 443 : 80;

  if (!href_url.userInfo().isEmpty() || !url.isValid() || (scheme != QSL("http") && scheme != QSL("https")) ||
      scheme != m_sourceUrl.scheme().toLower() || url.host().compare(m_sourceUrl.host(), Qt::CaseInsensitive) != 0 ||
      url.port(default_port) != m_sourceUrl.port(default_port)) {
    return {};
  }

  url.setFragment({});
  const QString cache_key = url.toString();
  const auto cached = m_linkedResources.constFind(cache_key);

  if (cached != m_linkedResources.cend()) {
    return cached.value();
  }

  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(m_resourceHandler(url), &error);

  if (error.error != QJsonParseError::NoError) {
    qWarningNN << LOGSEC_STANDARD << "Could not parse linked WordPress resource" << QUOTE_W_SPACE_DOT(cache_key)
               << error.errorString();
  }

  m_linkedResources.insert(cache_key, document);
  return document;
}
