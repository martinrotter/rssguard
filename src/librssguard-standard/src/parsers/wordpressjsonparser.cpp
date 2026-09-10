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

WordpressJsonParser::WordpressJsonParser(const QString& data) : FeedParser(data, DataType::Json) {}

WordpressJsonParser::~WordpressJsonParser() {}

bool WordpressJsonParser::isWordpressItem(
    const QJsonObject& item) const {

  return item.contains(QSL("id")) &&
         item.contains(QSL("date")) &&
         item.contains(QSL("link")) &&
         item.contains(QSL("title")) &&
         item[QSL("title")].isObject() &&
         item.contains(QSL("content")) &&
         item[QSL("content")].isObject();
}

QList<StandardFeed*> WordpressJsonParser::discoverFeeds(ServiceRoot* root,
                                               const QUrl& url,
                                               bool deep_discovery,
                                               const QList<DocumentWithUrl>& documents) const {
  qDebugNN << LOGSEC_STANDARD << " test darco";


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
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "was not recognized as a WordPress REST API posts/pages feed.";
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

    const QJsonObject first_item = json.array().first().toObject();

    if (!isWordpressItem(first_item)) {
      throw ApplicationException(
          QObject::tr("not a WordPress REST API feed"));
    }

    auto* feed = new StandardFeed();
    //QList<IconLocation> icon_possible_locations;

    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setType(StandardFeed::Type::WordpressJson);
    feed->setTitle(network_res.m_url.toString());
    //feed->setDescription(json.object()[QSL("description")].toString());
    feed->setSource(network_res.m_url.toString());


    return {feed, {}};
  }
  else {
    throw ApplicationException(QObject::tr("not a JSON Wordpress feed"));
  }
}

QString WordpressJsonParser::jsonMessageTitle(const QJsonObject& msg_element) const {
  return msg_element[QSL("title")]
        .toObject()[QSL("rendered")]
        .toString();
}

QString WordpressJsonParser::jsonMessageUrl(const QJsonObject& msg_element) const {
  return msg_element[QSL("link")].toString();
}

// description is named "content" on wordpress context
QString WordpressJsonParser::jsonMessageDescription(const QJsonObject& msg_element) const {
  return msg_element[QSL("content")]
        .toObject()[QSL("rendered")]
        .toString();
}

// author is an ID which need call to another endpoint for author...
QString WordpressJsonParser::jsonMessageAuthor(const QJsonObject& msg_element) const {
  return QString(); // to replace by API call to gather real user
}

// on wordpress context, all date could be on Greewich time (GMT)
QDateTime WordpressJsonParser::jsonMessageDateCreated(const QJsonObject& msg_element) {
  QString published = msg_element[QSL("date_gmt")].toString();
  QString updated = msg_element[QSL("modified_gmt")].toString();

  return decideArticleDate(published, updated);
}


QString WordpressJsonParser::jsonMessageId(const QJsonObject& msg_element) const {
  return msg_element[QSL("id")].toString();
}

QString WordpressJsonParser::jsonMessageRawContents(const QJsonObject& msg_element) const {
  return QJsonDocument(msg_element).toJson(QJsonDocument::JsonFormat::Compact);
}

QJsonArray WordpressJsonParser::jsonMessageElements() {
  return m_json.array(); // override of the json format reader because wordpress return directly a JSON array instead of "items" containing array
}