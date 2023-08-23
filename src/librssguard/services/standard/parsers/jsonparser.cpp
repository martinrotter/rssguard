// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/jsonparser.h"

#include "miscellaneous/textfactory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

JsonParser::JsonParser(const QString& data) : FeedParser(data, false) {}

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
