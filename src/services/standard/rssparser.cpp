// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/standard/rssparser.h"

#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#include <QDomDocument>


RssParser::RssParser() {
}

RssParser::~RssParser() {
}

QList<Message> RssParser::parseXmlData(const QString &data) {
  QList<Message> messages;
  QDomDocument xml_file;
  QDateTime current_time = QDateTime::currentDateTime();

  xml_file.setContent(data, true);

  // Pull out all messages.
  QDomNodeList messages_in_xml = xml_file.elementsByTagName(QSL("item"));

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);
    Message new_message;

    // Deal with titles & descriptions.
    QString elem_title = message_item.namedItem(QSL("title")).toElement().text().simplified();
    QString elem_description = message_item.namedItem(QSL("encoded")).toElement().text();
    QString elem_enclosure = message_item.namedItem(QSL("enclosure")).toElement().attribute(QSL("url"));
    QString elem_enclosure_type = message_item.namedItem(QSL("enclosure")).toElement().attribute(QSL("type"));

    if (elem_description.isEmpty()) {
      elem_description = message_item.namedItem(QSL("description")).toElement().text();
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
    new_message.m_url = message_item.namedItem(QSL("link")).toElement().text();

    if (new_message.m_url.isEmpty() && !new_message.m_enclosures.isEmpty()) {
      new_message.m_url = new_message.m_enclosures.first().m_url;
    }

    if (new_message.m_url.isEmpty()) {
      // Try to get "href" attribute.
      new_message.m_url = message_item.namedItem(QSL("link")).toElement().attribute(QSL("href"));
    }

    new_message.m_author = message_item.namedItem(QSL("author")).toElement().text();

    if (new_message.m_author.isEmpty()) {
      new_message.m_author = message_item.namedItem(QSL("creator")).toElement().text();
    }

    // Deal with creation date.
    new_message.m_created = TextFactory::parseDateTime(message_item.namedItem(QSL("pubDate")).toElement().text());

    if (new_message.m_created.isNull()) {
      new_message.m_created = TextFactory::parseDateTime(message_item.namedItem(QSL("date")).toElement().text());
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
