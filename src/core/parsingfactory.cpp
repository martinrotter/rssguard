#include "core/parsingfactory.h"

#include "core/textfactory.h"

#include <QDomDocument>
#include <QDomElement>


ParsingFactory::ParsingFactory() {
}

QList<Message> ParsingFactory::parseAsATOM10(const QString &data) {
  // TODO: Implement this.
  return QList<Message>();
}

QList<Message> ParsingFactory::parseAsRSS20(const QString &data) {
  QList<Message> messages;
  QDomDocument xml_file;
  QDateTime current_time = QDateTime::currentDateTime();

  xml_file.setContent(data, true);

  // Pull out all messages.
  QDomNodeList messages_in_xml = xml_file.elementsByTagName("item");

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);

    QDomElement elem_link = message_item.namedItem("link").toElement();
    QDomElement elem_description = message_item.namedItem("description").toElement();
    QDomElement elem_description2 = message_item.namedItem("encoded").toElement();
    QDomElement elem_title = message_item.namedItem("title").toElement();
    QDomElement elem_updated = message_item.namedItem("pubDate").toElement();
    QDomElement elem_author = message_item.namedItem("author").toElement();
    QDomElement elem_author2 = message_item.namedItem("creator").toElement();

    // RSS 1.0 requires to have title and link valid.
    if (elem_description.text().isEmpty() && elem_title.text().isEmpty()) {
      continue;
    }

    Message new_message;
    new_message.m_title = TextFactory::stripTags(elem_title.text().simplified());

    if (new_message.m_title.isEmpty()) {
      new_message.m_title = TextFactory::stripTags(elem_description.text().simplified());
    }

    new_message.m_contents = elem_description2.text();
    if (new_message.m_contents.isEmpty()) {
      new_message.m_contents = elem_description.text();
    }

    new_message.m_url = elem_link.text();
    new_message.m_author = elem_author.text();

    if (new_message.m_author.isEmpty()) {
      new_message.m_author = elem_author2.text();
    }

    // Setup dates.
    new_message.m_created = TextFactory::parseDateTime(elem_updated.text());
    new_message.m_createdFromFeed = !new_message.m_created.isNull();

    if (!new_message.m_createdFromFeed) {
      // Date was NOT obtained from the feed,
      // set current date as creation date for the message.
      new_message.m_created = current_time;
    }

    messages.append(new_message);
  }

  return messages;
}
