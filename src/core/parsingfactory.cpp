#include "core/parsingfactory.h"

#include "core/textfactory.h"

#include <QDomDocument>
#include <QDomElement>


ParsingFactory::ParsingFactory() {
}

QList<Message> ParsingFactory::parseAsRSS20(const QString &data) {
  QList<Message> messages;
  QDomDocument xml_file;
  xml_file.setContent(data, true);
  QDomNodeList messages_in_xml = xml_file.elementsByTagName("item");

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomElement elem_link = messages_in_xml.item(i).namedItem("link").toElement();
    QDomElement elem_description = messages_in_xml.item(i).namedItem("description").toElement();
    QDomElement elem_description2 = messages_in_xml.item(i).namedItem("encoded").toElement();
    QDomElement elem_title = messages_in_xml.item(i).namedItem("title").toElement();
    QDomElement elem_updated = messages_in_xml.item(i).namedItem("pubDate").toElement();
    QDomElement elem_author = messages_in_xml.item(i).namedItem("author").toElement();
    QDomElement elem_author2 = messages_in_xml.item(i).namedItem("creator").toElement();

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

    new_message.m_created = TextFactory::parseDateTime(elem_updated.text());

    messages.append(new_message);
  }

  return messages;
}
