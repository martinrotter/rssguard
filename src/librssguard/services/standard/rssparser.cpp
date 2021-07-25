// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/rssparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>
#include <QTextStream>

RssParser::RssParser(const QString& data) : FeedParser(data) {}

QDomNodeList RssParser::messageElements() {
  QDomNode channel_elem = m_xml.namedItem(QSL("rss")).namedItem(QSL("channel"));

  if (channel_elem.isNull()) {
    return QDomNodeList();
  }
  else {
    return channel_elem.toElement().elementsByTagName(QSL("item"));
  }
}

Message RssParser::extractMessage(const QDomElement& msg_element, QDateTime current_time) const {
  Message new_message;

  // Deal with titles & descriptions.
  QString elem_title = msg_element.namedItem(QSL("title")).toElement().text().simplified();
  QString elem_description = rawXmlChild(msg_element.elementsByTagName(QSL("encoded")).at(0).toElement());
  QString elem_enclosure = msg_element.namedItem(QSL("enclosure")).toElement().attribute(QSL("url"));
  QString elem_enclosure_type = msg_element.namedItem(QSL("enclosure")).toElement().attribute(QSL("type"));

  new_message.m_customId = msg_element.namedItem(QSL("guid")).toElement().text();
  new_message.m_url = msg_element.namedItem(QSL("link")).toElement().text();

  if (new_message.m_url.isEmpty() && !new_message.m_enclosures.isEmpty()) {
    new_message.m_url = new_message.m_enclosures.first().m_url;
  }

  if (new_message.m_url.isEmpty()) {
    // Try to get "href" attribute.
    new_message.m_url = msg_element.namedItem(QSL("link")).toElement().attribute(QSL("href"));
  }

  if (elem_description.isEmpty()) {
    elem_description = rawXmlChild(msg_element.elementsByTagName(QSL("description")).at(0).toElement());
  }

  if (elem_description.isEmpty()) {
    elem_description = new_message.m_url;
  }

  // Now we obtained maximum of information for title & description.
  if (elem_title.isEmpty()) {
    if (elem_description.isEmpty()) {
      // BOTH title and description are empty, skip this message.
      throw ApplicationException(QSL("Not enough data for the message."));
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

  if (!elem_enclosure.isEmpty()) {
    new_message.m_enclosures.append(Enclosure(elem_enclosure, elem_enclosure_type));
    qDebugNN << LOGSEC_CORE
             << "Found enclosure"
             << QUOTE_W_SPACE(elem_enclosure)
             << "for the message.";
  }
  else {
    new_message.m_enclosures.append(mrssGetEnclosures(msg_element));
  }

  QString raw_contents;
  QTextStream str(&raw_contents);

  str.setCodec(DEFAULT_FEED_ENCODING);

  msg_element.save(str, 0, QDomNode::EncodingPolicy::EncodingFromTextStream);
  new_message.m_rawContents = raw_contents;

  new_message.m_author = msg_element.namedItem(QSL("author")).toElement().text();

  if (new_message.m_author.isEmpty()) {
    new_message.m_author = msg_element.namedItem(QSL("creator")).toElement().text();
  }

  // Deal with creation date.
  new_message.m_created = TextFactory::parseDateTime(msg_element.namedItem(QSL("pubDate")).toElement().text());

  if (new_message.m_created.isNull()) {
    new_message.m_created = TextFactory::parseDateTime(msg_element.namedItem(QSL("date")).toElement().text());
  }

  if (!(new_message.m_createdFromFeed = !new_message.m_created.isNull())) {
    // Date was NOT obtained from the feed,
    // set current date as creation date for the message.
    new_message.m_created = current_time;
  }

  if (new_message.m_author.isNull()) {
    new_message.m_author = "";
  }

  new_message.m_author = qApp->web()->unescapeHtml(new_message.m_author);

  if (new_message.m_url.isNull()) {
    new_message.m_url = "";
  }

  return new_message;
}
