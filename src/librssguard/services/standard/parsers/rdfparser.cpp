// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/rdfparser.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDomDocument>

RdfParser::RdfParser(const QString& data)
  : FeedParser(data),
  m_rdfNamespace(QSL("http://www.w3.org/1999/02/22-rdf-syntax-ns#")),
  m_rssNamespace(QSL("http://purl.org/rss/1.0/")),
  m_rssCoNamespace(QSL("http://purl.org/rss/1.0/modules/content/")),
  m_dcElNamespace(QSL("http://purl.org/dc/elements/1.1/")) {}

QDomNodeList RdfParser::xmlMessageElements() {
  return m_xml.elementsByTagNameNS(m_rssNamespace, QSL("item"));
}

QString RdfParser::rssNamespace() const {
  return m_rssNamespace;
}

QString RdfParser::rdfNamespace() const {
  return m_rdfNamespace;
}

QString RdfParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_rssNamespace, QSL("title")).at(0).toElement().text();
}

QString RdfParser::xmlMessageDescription(const QDomElement& msg_element) const {
  QString description = msg_element.elementsByTagNameNS(m_rssCoNamespace, QSL("encoded")).at(0).toElement().text();

  if (description.simplified().isEmpty()) {
    description = msg_element.elementsByTagNameNS(m_rssNamespace, QSL("description")).at(0).toElement().text();
  }

  return description;
}

QString RdfParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("creator")).at(0).toElement().text();
}

QDateTime RdfParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  return TextFactory::parseDateTime(msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("date")).at(0).toElement().text());
}

QString RdfParser::xmlMessageId(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_dcElNamespace, QSL("identifier")).at(0).toElement().text();
}

QString RdfParser::xmlMessageUrl(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_rssNamespace, QSL("link")).at(0).toElement().text();
}

QList<Enclosure> RdfParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  return {};
}
