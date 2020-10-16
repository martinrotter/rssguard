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
  QString global_author = json.object()["author"].toObject()["name"].toString();

  if (global_author.isEmpty()) {
    global_author = json.object()["authors"].toArray().at(0).toObject()["name"].toString();
  }

  for (const QJsonValue& msg_val : json.object()["items"].toArray()) {
    QJsonObject msg_obj = msg_val.toObject();
    Message msg;

    msg.m_title = msg_obj["title"].toString();
    msg.m_url = msg_obj["url"].toString();
    msg.m_contents = msg_obj.contains("content_html") ? msg_obj["content_html"].toString() : msg_obj["content_text"].toString();

    msg.m_created = TextFactory::parseDateTime(msg_obj.contains("date_modified")
                                               ? msg_obj["date_modified"].toString()
                                               : msg_obj["date_published"].toString());

    if (!msg.m_created.isValid()) {
      msg.m_created = QDateTime::currentDateTime();
      msg.m_createdFromFeed = false;
    }
    else {
      msg.m_createdFromFeed = true;
    }

    if (msg_obj.contains("author")) {
      msg.m_author = msg_obj["author"].toObject()["name"].toString();
    }
    else if (msg_obj.contains("authors")) {
      msg.m_author = msg_obj["authors"].toArray().at(0).toObject()["name"].toString();
    }
    else if (!global_author.isEmpty()) {
      msg.m_author = global_author;
    }

    for (const QJsonValue& att : msg_obj["attachments"].toArray()) {
      QJsonObject att_obj = att.toObject();
      auto xx = att_obj["url"].toString();

      msg.m_enclosures.append(Enclosure(att_obj["url"].toString(), att_obj["mime_type"].toString()));
    }

    if (msg.m_title.isEmpty() && !msg.m_url.isEmpty()) {
      msg.m_title = msg.m_url;
    }

    msgs.append(msg);
  }

  return msgs;
}
