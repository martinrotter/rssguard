// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/jsonparser.h"

#include "miscellaneous/textfactory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

JsonParser::JsonParser(const QString& data) : m_jsonData(data) {}

QList<Message> JsonParser::messages() const {
  QList<Message> msgs;
  QJsonDocument json = QJsonDocument::fromJson(m_jsonData.toUtf8());
  QString global_author = json.object()[QSL("author")].toObject()[QSL("name")].toString();

  if (global_author.isEmpty()) {
    global_author = json.object()[QSL("authors")].toArray().at(0).toObject()[QSL("name")].toString();
  }

  auto json_items = json.object()[QSL("items")].toArray();

  for (const QJsonValue& msg_val : qAsConst(json_items)) {
    QJsonObject msg_obj = msg_val.toObject();
    Message msg;

    msg.m_customId = msg_obj[QSL("id")].toString();
    msg.m_title = msg_obj[QSL("title")].toString();
    msg.m_url = msg_obj[QSL("url")].toString();
    msg.m_contents = msg_obj.contains(QSL("content_html"))
                                      ? msg_obj[QSL("content_html")].toString()
                                      : msg_obj[QSL("content_text")].toString();
    msg.m_rawContents = QJsonDocument(msg_obj).toJson(QJsonDocument::JsonFormat::Compact);

    msg.m_created = TextFactory::parseDateTime(msg_obj.contains(QSL("date_modified"))
                                               ? msg_obj[QSL("date_modified")].toString()
                                               : msg_obj[QSL("date_published")].toString());

    if (!msg.m_created.isValid()) {
      msg.m_created = QDateTime::currentDateTime();
      msg.m_createdFromFeed = false;
    }
    else {
      msg.m_createdFromFeed = true;
    }

    if (msg_obj.contains(QSL("author"))) {
      msg.m_author = msg_obj[QSL("author")].toObject()[QSL("name")].toString();
    }
    else if (msg_obj.contains(QSL("authors"))) {
      msg.m_author = msg_obj[QSL("authors")].toArray().at(0).toObject()[QSL("name")].toString();
    }
    else if (!global_author.isEmpty()) {
      msg.m_author = global_author;
    }

    auto json_att = msg_obj[QSL("attachments")].toArray();

    for (const QJsonValue& att : qAsConst(json_att)) {
      QJsonObject att_obj = att.toObject();

      msg.m_enclosures.append(Enclosure(att_obj[QSL("url")].toString(), att_obj[QSL("mime_type")].toString()));
    }

    if (msg.m_title.isEmpty() && !msg.m_url.isEmpty()) {
      msg.m_title = msg.m_url;
    }

    msgs.append(msg);
  }

  return msgs;
}
