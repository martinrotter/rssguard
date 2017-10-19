// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/standard/feedparser.h"

#include "exceptions/applicationexception.h"

#include <QDebug>

FeedParser::FeedParser(const QString& data) : m_xmlData(data) {
  m_xml.setContent(m_xmlData, true);
}

FeedParser::~FeedParser() {}

QList<Message> FeedParser::messages() {
  QString feed_author = feedAuthor();

  QList<Message> messages;
  QDateTime current_time = QDateTime::currentDateTime();

  // Pull out all messages.
  QDomNodeList messages_in_xml = messageElements();

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomNode message_item = messages_in_xml.item(i);

    try {
      Message new_message = extractMessage(message_item.toElement(), current_time);

      if (new_message.m_author.isEmpty()) {
        new_message.m_author = feed_author;
      }

      messages.append(new_message);
    }
    catch (const ApplicationException& ex) {
      qDebug() << ex.message();
    }
  }

  return messages;
}

QStringList FeedParser::textsFromPath(const QDomElement& element, const QString& namespace_uri,
                                      const QString& xml_path, bool only_first) const {
  QStringList paths = xml_path.split('/');
  QStringList result;

  QList<QDomElement> current_elements;
  current_elements.append(element);

  while (!paths.isEmpty()) {
    QList<QDomElement> next_elements;
    QString next_local_name = paths.takeFirst();

    foreach (const QDomElement& elem, current_elements) {
      QDomNodeList elements = elem.elementsByTagNameNS(namespace_uri, next_local_name);

      for (int i = 0; i < elements.size(); i++) {
        next_elements.append(elements.at(i).toElement());

        if (only_first) {
          break;
        }
      }

      if (next_elements.size() == 1 && only_first) {
        break;
      }
    }

    current_elements = next_elements;
  }

  if (!current_elements.isEmpty()) {
    foreach (const QDomElement& elem, current_elements) {
      result.append(elem.text());
    }
  }

  return result;
}

QString FeedParser::feedAuthor() const {
  return "";
}
