// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/jsonparser.h"

#include "src/definitions.h"
#include "src/standardfeed.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/definitions/typedefs.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

JsonParser::JsonParser(const QString& data) : FeedParser(data, DataType::Json) {}

JsonParser::~JsonParser() {}

QList<StandardFeed*> JsonParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);

  if (!base_result.isEmpty()) {
    return base_result;
  }

  QString my_url = url.toString();
  QList<StandardFeed*> feeds;

  // 1. Test direct URL for a feed.
  // 2. Test embedded JSON feed links from HTML data.

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
    static QRegularExpression rx(QSL(JSON_REGEX_MATCHER), QRegularExpression::PatternOption::CaseInsensitiveOption);
    static QRegularExpression rx_href(QSL(JSON_HREF_REGEX_MATCHER),
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

  return feeds;
}

QPair<StandardFeed*, QList<IconLocation>> JsonParser::guessFeed(const QByteArray& content,
                                                                const QString& content_type) const {
  if (content_type.contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive) ||
      content.simplified().startsWith('{')) {
    QJsonParseError json_err;
    QJsonDocument json = QJsonDocument::fromJson(content, &json_err);

    if (json.isNull() && !json_err.errorString().isEmpty()) {
      throw FeedRecognizedButFailedException(QObject::tr("JSON error '%1'").arg(json_err.errorString()));
    }

    if (!json.object().contains(QSL("version"))) {
      throw ApplicationException(QObject::tr("not a JSON feed"));
    }

    auto* feed = new StandardFeed();
    QList<IconLocation> icon_possible_locations;

    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setType(StandardFeed::Type::Json);
    feed->setTitle(json.object()[QSL("title")].toString());
    feed->setDescription(json.object()[QSL("description")].toString());

    auto home_page = json.object()[QSL("home_page_url")].toString();

    if (!home_page.isEmpty()) {
      icon_possible_locations.prepend({home_page, false});
    }

    auto icon = json.object()[QSL("favicon")].toString();

    if (icon.isEmpty()) {
      icon = json.object()[QSL("icon")].toString();
    }

    if (!icon.isEmpty()) {
      // Low priority, download directly.
      icon_possible_locations.append({icon, true});
    }

    return QPair<StandardFeed*, QList<IconLocation>>(feed, icon_possible_locations);
  }
  else {
    throw ApplicationException(QObject::tr("not a JSON feed"));
  }
}

QString JsonParser::feedAuthor() const {
  QString global_author = m_json.object()[QSL("author")].toObject()[QSL("name")].toString();

  if (global_author.isEmpty()) {
    global_author = m_json.object()[QSL("authors")].toArray().at(0).toObject()[QSL("name")].toString();
  }

  return global_author;
}

QJsonArray JsonParser::jsonMessageElements() {
  return m_json.object()[QSL("items")].toArray();
}

QString JsonParser::jsonMessageTitle(const QJsonObject& msg_element) const {
  return msg_element[QSL("title")].toString();
}

QString JsonParser::jsonMessageUrl(const QJsonObject& msg_element) const {
  return msg_element[QSL("url")].toString();
}

QString JsonParser::jsonMessageDescription(const QJsonObject& msg_element) const {
  return msg_element.contains(QSL("content_html")) ? msg_element[QSL("content_html")].toString()
                                                   : msg_element[QSL("content_text")].toString();
}

QString JsonParser::jsonMessageAuthor(const QJsonObject& msg_element) const {
  if (msg_element.contains(QSL("author"))) {
    return msg_element[QSL("author")].toObject()[QSL("name")].toString();
  }
  else if (msg_element.contains(QSL("authors"))) {
    return msg_element[QSL("authors")].toArray().at(0).toObject()[QSL("name")].toString();
  }
  else {
    return {};
  }
}

QDateTime JsonParser::jsonMessageDateCreated(const QJsonObject& msg_element) {
  return TextFactory::parseDateTime(msg_element.contains(QSL("date_modified"))
                                      ? msg_element[QSL("date_modified")].toString()
                                      : msg_element[QSL("date_published")].toString(),
                                    &m_dateTimeFormat);
}

QString JsonParser::jsonMessageId(const QJsonObject& msg_element) const {
  return msg_element[QSL("id")].toString();
}

QList<Enclosure> JsonParser::jsonMessageEnclosures(const QJsonObject& msg_element) const {
  auto json_att = msg_element[QSL("attachments")].toArray();
  QList<Enclosure> enc;

  for (const QJsonValue& att : std::as_const(json_att)) {
    QJsonObject att_obj = att.toObject();

    enc.append(Enclosure(att_obj[QSL("url")].toString(), att_obj[QSL("mime_type")].toString()));
  }

  return enc;
}

QString JsonParser::jsonMessageRawContents(const QJsonObject& msg_element) const {
  return QJsonDocument(msg_element).toJson(QJsonDocument::JsonFormat::Compact);
}
