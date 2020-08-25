// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/rdfparser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#include <QDomDocument>

RdfParser::RdfParser() = default;

RdfParser::~RdfParser() = default;

QList<Message> RdfParser::parseXmlData(const QString& data) {
  QList<Message> messages;
  QDomDocument xml_file;
  QDateTime current_time = QDateTime::currentDateTime();

  xml_file.setContent(data, true);

  // Pull out all messages.
  QDomNodeList messages_in_xml = xml_file.elementsByTagName(QSL("item"));

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);
    Message new_message;

    // Deal with title and description.
    QString elem_title = message_item.namedItem(QSL("title")).toElement().text().simplified();
    QString elem_description = message_item.namedItem(QSL("description")).toElement().text();

    // Now we obtained maximum of information for title & description.
    if (elem_title.isEmpty()) {
      if (elem_description.isEmpty()) {
        // BOTH title and description are empty, skip this message.
        continue;
      }
      else {
        // Title is empty but description is not.
        new_message.m_title = qApp->web()->unescapeHtml(qApp->web()->stripTags(elem_description.simplified()));
        new_message.m_contents = elem_description;
      }
    }
    else {
      // Title is really not empty, description does not matter.
      new_message.m_title = qApp->web()->unescapeHtml(qApp->web()->stripTags(elem_title));
      new_message.m_contents = elem_description;
    }

    // Deal with link and author.
    new_message.m_url = message_item.namedItem(QSL("link")).toElement().text();
    new_message.m_author = message_item.namedItem(QSL("creator")).toElement().text();

    // Deal with creation date.
    QString elem_updated = message_item.namedItem(QSL("date")).toElement().text();

    if (elem_updated.isEmpty()) {
      elem_updated = message_item.namedItem(QSL("dc:date")).toElement().text();
    }

    // Deal with creation date.
    new_message.m_created = TextFactory::parseDateTime(elem_updated);
    new_message.m_createdFromFeed = !new_message.m_created.isNull();

    if (!new_message.m_createdFromFeed) {
      // Date was NOT obtained from the feed, set current date as creation date for the message.
      new_message.m_created = current_time;
    }

    if (new_message.m_author.isNull()) {
      new_message.m_author = "";
    }

    if (new_message.m_url.isNull()) {
      new_message.m_url = "";
    }

    messages.append(new_message);
  }

  return messages;
}
