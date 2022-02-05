// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/feedparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDebug>
#include <QRegularExpression>

#include <utility>

FeedParser::FeedParser(QString data) : m_xmlData(std::move(data)), m_mrssNamespace(QSL("http://search.yahoo.com/mrss/")) {
  QString error;

  if (!m_xml.setContent(m_xmlData, true, &error)) {
    throw ApplicationException(QObject::tr("XML problem: %1").arg(error));
  }
}

QString FeedParser::messageRawContents(const QDomElement& msg_element) const {
  QString raw_contents;
  QTextStream str(&raw_contents);

  msg_element.save(str, 0, QDomNode::EncodingPolicy::EncodingFromTextStream);
  return raw_contents;
}

QList<Message> FeedParser::messages() {
  QString feed_author = feedAuthor();
  QList<Message> messages;
  QDateTime current_time = QDateTime::currentDateTime();

  // Pull out all messages.
  QDomNodeList messages_in_xml = messageElements();

  for (int i = 0; i < messages_in_xml.size(); i++) {
    QDomElement message_item = messages_in_xml.item(i).toElement();

    try {
      Message new_message;

      // Fill available data.
      new_message.m_title = qApp->web()->unescapeHtml(messageTitle(message_item));
      new_message.m_contents = messageDescription(message_item);
      new_message.m_author = qApp->web()->unescapeHtml(messageAuthor(message_item));
      new_message.m_url = messageUrl(message_item);
      new_message.m_created = messageDateCreated(message_item);
      new_message.m_customId = messageId(message_item);
      new_message.m_rawContents = messageRawContents(message_item);
      new_message.m_enclosures = messageEnclosures(message_item);
      new_message.m_enclosures.append(mrssGetEnclosures(message_item));

      // Fixup missing data.
      //
      // NOTE: Message must have "title" field, otherwise it is skipped.

      // Author.
      if (new_message.m_author.isEmpty() && !feed_author.isEmpty()) {
        new_message.m_author = feed_author;
      }

      // Created date.
      new_message.m_createdFromFeed = !new_message.m_created.isNull();

      if (!new_message.m_createdFromFeed) {
        // Date was NOT obtained from the feed, set current date as creation date for the message.
        // NOTE: Date is lessened by 1 second for each message to allow for more
        // stable sorting.
        new_message.m_created = current_time.addSecs(-1);
        current_time = new_message.m_created;
      }

      // Enclosures.
      for (Enclosure& enc : new_message.m_enclosures) {
        if (enc.m_mimeType.simplified().isEmpty()) {
          enc.m_mimeType = QSL(DEFAULT_ENCLOSURE_MIME_TYPE);
        }
      }

      // Url.
      new_message.m_url = new_message.m_url.replace(QRegularExpression(QSL("[\\t\\n]")), QString());

      messages.append(new_message);
    }
    catch (const ApplicationException& ex) {
      qDebugNN << LOGSEC_CORE
               << "Problem when extracting message: "
               << ex.message();
    }
  }

  return messages;
}

QList<Enclosure> FeedParser::mrssGetEnclosures(const QDomElement& msg_element) const {
  QList<Enclosure> enclosures;
  auto content_list = msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("content"));

  for (int i = 0; i < content_list.size(); i++) {
    QDomElement elem_content = content_list.at(i).toElement();
    QString url = elem_content.attribute(QSL("url"));
    QString type = elem_content.attribute(QSL("type"));

    if (type.isEmpty()) {
      type = QSL(DEFAULT_ENCLOSURE_MIME_TYPE);
    }

    if (!url.isEmpty() && !type.isEmpty()) {
      enclosures.append(Enclosure(url, type));
    }
  }

  auto thumbnail_list = msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("thumbnail"));

  for (int i = 0; i < thumbnail_list.size(); i++) {
    QDomElement elem_content = thumbnail_list.at(i).toElement();
    QString url = elem_content.attribute(QSL("url"));

    if (!url.isEmpty()) {
      enclosures.append(Enclosure(url, QSL(DEFAULT_ENCLOSURE_MIME_TYPE)));
    }
  }

  return enclosures;
}

QString FeedParser::mrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const {
  QString text = msg_element.elementsByTagNameNS(m_mrssNamespace, xml_path).at(0).toElement().text();

  return text;
}

QString FeedParser::rawXmlChild(const QDomElement& container) const {
  QString raw;
  auto children = container.childNodes();

  for (int i = 0; i < children.size(); i++) {
    if (children.at(i).isCDATASection()) {
      raw += children.at(i).toCDATASection().data();
    }
    else {
      QString raw_ch;
      QTextStream str(&raw_ch);

      children.at(i).save(str, 0);
      raw += qApp->web()->unescapeHtml(raw_ch);
    }
  }

  return raw;
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

    for (const QDomElement& elem : current_elements) {
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
    for (const QDomElement& elem : qAsConst(current_elements)) {
      result.append(elem.text());
    }
  }

  return result;
}

QString FeedParser::feedAuthor() const {
  return QL1S("");
}
