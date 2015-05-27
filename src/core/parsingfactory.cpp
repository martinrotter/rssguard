// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "core/parsingfactory.h"

#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#include <QDomDocument>
#include <QDomElement>


ParsingFactory::ParsingFactory() {
}

QList<Message> ParsingFactory::parseAsATOM10(const QString &data) {
  QList<Message> messages;
  QDomDocument xml_file;
  QDateTime current_time = QDateTime::currentDateTime();

  xml_file.setContent(data, true);

  // Pull out all messages.
  QDomNodeList messages_in_xml = xml_file.elementsByTagName("entry");

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);
    Message new_message;

    // Deal with titles & descriptions.
    QString elem_title = message_item.namedItem("title").toElement().text().simplified();
    QString elem_summary = message_item.namedItem("summary").toElement().text();

    if (elem_summary.isEmpty()) {
      elem_summary = message_item.namedItem("content").toElement().text();
    }

    // Now we obtained maximum of information for title & description.
    if (elem_title.isEmpty()) {
      if (elem_summary.isEmpty()) {
        // BOTH title and description are empty, skip this message.
        continue;
      }
      else {
        // Title is empty but description is not.
        new_message.m_title = WebFactory::instance()->stripTags(elem_summary.simplified());
        new_message.m_contents = elem_summary;
      }
    }
    else {
      // Title is not empty, description does not matter.
      new_message.m_title = WebFactory::instance()->stripTags(elem_title);
      new_message.m_contents = elem_summary;
    }

    // Deal with link.
    QDomNodeList elem_links = message_item.toElement().elementsByTagName("link");

    for (int i = 0; i < elem_links.size(); i++) {
      QDomElement link = elem_links.at(i).toElement();

      if (link.attribute("rel") == "enclosure") {
        new_message.m_enclosures.append(Enclosure(link.attribute("href"), link.attribute("type")));

        qDebug("Adding enclosure '%s' for the message.", qPrintable(new_message.m_enclosures.last().m_url));
      }
      else {
        new_message.m_url = link.attribute("href");
      }
    }

    if (new_message.m_url.isEmpty() && !new_message.m_enclosures.isEmpty()) {
      new_message.m_url = new_message.m_enclosures.first().m_url;
    }

    // Deal with authors.
    new_message.m_author = WebFactory::instance()->escapeHtml(message_item.namedItem("author").namedItem("name").toElement().text());

    // Deal with creation date.
    new_message.m_created = TextFactory::parseDateTime(message_item.namedItem("updated").toElement().text());
    new_message.m_createdFromFeed = !new_message.m_created.isNull();

    if (!new_message.m_createdFromFeed) {
      // Date was NOT obtained from the feed, set current date as creation date for the message.
      new_message.m_created = current_time;
    }

    // TODO: There is a difference between "" and QString() in terms of NULL SQL values!
    // This is because of difference in QString::isNull() and QString::isEmpty(), the "" is not null
    // while QString() is.
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

QList<Message> ParsingFactory::parseAsRDF(const QString &data) {
  QList<Message> messages;
  QDomDocument xml_file;
  QDateTime current_time = QDateTime::currentDateTime();

  xml_file.setContent(data, true);

  // Pull out all messages.
  QDomNodeList messages_in_xml = xml_file.elementsByTagName("item");

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);
    Message new_message;

    // Deal with title and description.
    QString elem_title = message_item.namedItem("title").toElement().text().simplified();
    QString elem_description = message_item.namedItem("description").toElement().text();

    // Now we obtained maximum of information for title & description.
    if (elem_title.isEmpty()) {
      if (elem_description.isEmpty()) {
        // BOTH title and description are empty, skip this message.
        continue;
      }
      else {
        // Title is empty but description is not.
        new_message.m_title = WebFactory::instance()->escapeHtml(WebFactory::instance()->stripTags(elem_description.simplified()));
        new_message.m_contents = elem_description;
      }
    }
    else {
      // Title is really not empty, description does not matter.
      new_message.m_title = WebFactory::instance()->escapeHtml(WebFactory::instance()->stripTags(elem_title));
      new_message.m_contents = elem_description;
    }


    // Deal with link and author.
    new_message.m_url = message_item.namedItem("link").toElement().text();
    new_message.m_author = message_item.namedItem("creator").toElement().text();

    // Deal with creation date.
    QString elem_updated = message_item.namedItem("date").toElement().text();

    if (elem_updated.isEmpty()) {
      elem_updated = message_item.namedItem("dc:date").toElement().text();
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
    QString elem_enclosure = message_item.namedItem("enclosure").toElement().attribute("url");
    QString elem_enclosure_type = message_item.namedItem("enclosure").toElement().attribute("type");

    if (elem_description.isEmpty()) {
      elem_description = message_item.namedItem("encoded").toElement().text();
    }

    // Now we obtained maximum of information for title & description.
    if (elem_title.isEmpty()) {
      if (elem_description.isEmpty()) {
        // BOTH title and description are empty, skip this message.
        continue;
      }
      else {
        // Title is empty but description is not.
        new_message.m_title = WebFactory::instance()->stripTags(elem_description.simplified());
        new_message.m_contents = elem_description;
      }
    }
    else {
      // Title is really not empty, description does not matter.
      new_message.m_title = WebFactory::instance()->stripTags(elem_title);
      new_message.m_contents = elem_description;
    }

    if (!elem_enclosure.isEmpty()) {
      new_message.m_enclosures.append(Enclosure(elem_enclosure, elem_enclosure_type));

      qDebug("Adding enclosure '%s' for the message.", qPrintable(elem_enclosure));
    }

    // Deal with link and author.
    new_message.m_url = message_item.namedItem("link").toElement().text();

    if (new_message.m_url.isEmpty() && !new_message.m_enclosures.isEmpty()) {
      new_message.m_url = new_message.m_enclosures.first().m_url;
    }

    new_message.m_author = message_item.namedItem("author").toElement().text();

    if (new_message.m_author.isEmpty()) {
      new_message.m_author = message_item.namedItem("creator").toElement().text();
    }

    // Deal with creation date.
    new_message.m_created = TextFactory::parseDateTime(message_item.namedItem("pubDate").toElement().text());

    if (new_message.m_created.isNull()) {
      new_message.m_created = TextFactory::parseDateTime(message_item.namedItem("date").toElement().text());
    }

    if (!(new_message.m_createdFromFeed = !new_message.m_created.isNull())) {
      // Date was NOT obtained from the feed,
      // set current date as creation date for the message.
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
