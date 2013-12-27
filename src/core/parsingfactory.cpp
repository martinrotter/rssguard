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
    Message new_message;

    // Deal with titles & descriptions.
    QString elem_title = message_item.namedItem("title").toElement().text().simplified();
    QString elem_description = message_item.namedItem("description").toElement().text();

    if (elem_description.isEmpty()) {
      elem_description = message_item.namedItem("encoded").toElement().text();
    }

    // Now we obtained maximum of informations for title & description.
    if (elem_title.isEmpty()) {
      if (elem_description.isEmpty()) {
        // BOTH title and description are empty, skip this message.
        continue;
      }
      else {
        // Title is empty but description is not.
        new_message.m_title = TextFactory::stripTags(elem_description);
        new_message.m_contents = elem_description;
      }
    }
    else {
      if (elem_description.isEmpty()) {
        // Title is not empty but description is empty.
        new_message.m_title = TextFactory::stripTags(elem_title);
        new_message.m_contents = QObject::tr("This message has not contents.");
      }
      else {
        // Both description and title are not empty.
        new_message.m_title = TextFactory::stripTags(elem_title);
        new_message.m_contents = elem_description;
      }
    }

    // Deal with link and author.
    new_message.m_url = message_item.namedItem("link").toElement().text();
    new_message.m_author = message_item.namedItem("author").toElement().text();

    if (new_message.m_author.isEmpty()) {
      new_message.m_author = message_item.namedItem("creator").toElement().text();
    }

    // Deal with creation date.
    new_message.m_created = TextFactory::parseDateTime(message_item.namedItem("pubDate").toElement().text());
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
