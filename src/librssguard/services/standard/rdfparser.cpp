// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/rdfparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>

RdfParser::RdfParser(const QString& data)
  : FeedParser(data),
  m_rdfNamespace(QSL("http://www.w3.org/1999/02/22-rdf-syntax-ns#")),
  m_rssNamespace(QSL("http://purl.org/rss/1.0/")) {}

QDomNodeList RdfParser::messageElements() {
  QDomDocument xml_file;

  xml_file.setContent(m_xmlData, true);

  // Pull out all messages.
  return xml_file.elementsByTagName(QSL("item"));
}

Message RdfParser::extractMessage(const QDomElement& msg_element, QDateTime current_time) const {
  Message new_message;

  // Deal with title and description.
  QString elem_title = msg_element.namedItem(QSL("title")).toElement().text().simplified();
  QString elem_description = rawXmlChild(msg_element.namedItem(QSL("description")).toElement());

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

  QString raw_contents;
  QTextStream str(&raw_contents);

  str.setCodec(DEFAULT_FEED_ENCODING);

  msg_element.save(str, 0, QDomNode::EncodingPolicy::EncodingFromTextStream);
  new_message.m_rawContents = raw_contents;

  // Deal with link and author.
  new_message.m_url = msg_element.namedItem(QSL("link")).toElement().text();
  new_message.m_author = msg_element.namedItem(QSL("creator")).toElement().text();

  // Deal with creation date.
  QString elem_updated = msg_element.namedItem(QSL("date")).toElement().text();

  if (elem_updated.isEmpty()) {
    elem_updated = msg_element.namedItem(QSL("dc:date")).toElement().text();
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

  return new_message;
}

QString RdfParser::rssNamespace() const {
  return m_rssNamespace;
}

QString RdfParser::rdfNamespace() const {
  return m_rdfNamespace;
}
