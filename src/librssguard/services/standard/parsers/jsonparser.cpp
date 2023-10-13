// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/jsonparser.h"

#include "definitions/definitions.h"
#include "definitions/typedefs.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"
#include "services/standard/standardfeed.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

JsonParser::JsonParser(const QString& data) : FeedParser(data, false) {}

JsonParser::~JsonParser() {}

QPair<StandardFeed*, QList<IconLocation>> JsonParser::guessFeed(const QByteArray& content,
                                                                const QString& content_type) const {
  if (content_type.contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive) ||
      content.simplified().startsWith('{')) {
    QJsonParseError json_err;
    QJsonDocument json = QJsonDocument::fromJson(content, &json_err);

    if (json.isNull() && !json_err.errorString().isEmpty()) {
      throw FeedRecognizedButFailedException(QObject::tr("JSON error '%1'").arg(json_err.errorString()));
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

QDateTime JsonParser::jsonMessageDateCreated(const QJsonObject& msg_element) const {
  return TextFactory::parseDateTime(msg_element.contains(QSL("date_modified"))
                                      ? msg_element[QSL("date_modified")].toString()
                                      : msg_element[QSL("date_published")].toString());
}

QString JsonParser::jsonMessageId(const QJsonObject& msg_element) const {
  return msg_element[QSL("id")].toString();
}

QList<Enclosure> JsonParser::jsonMessageEnclosures(const QJsonObject& msg_element) const {
  auto json_att = msg_element[QSL("attachments")].toArray();
  QList<Enclosure> enc;

  for (const QJsonValue& att : qAsConst(json_att)) {
    QJsonObject att_obj = att.toObject();

    enc.append(Enclosure(att_obj[QSL("url")].toString(), att_obj[QSL("mime_type")].toString()));
  }

  return enc;
}

QString JsonParser::jsonMessageRawContents(const QJsonObject& msg_element) const {
  return QJsonDocument(msg_element).toJson(QJsonDocument::JsonFormat::Compact);
}
