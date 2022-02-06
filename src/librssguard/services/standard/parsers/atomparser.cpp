// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/atomparser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include "exceptions/applicationexception.h"

AtomParser::AtomParser(const QString& data) : FeedParser(data) {
  QString version = m_xml.documentElement().attribute(QSL("version"));

  if (version == QSL("0.3")) {
    m_atomNamespace = QSL("http://purl.org/atom/ns#");
  }
  else {
    m_atomNamespace = QSL("http://www.w3.org/2005/Atom");
  }
}

QString AtomParser::feedAuthor() const {
  auto authors = m_xml.documentElement().elementsByTagNameNS(m_atomNamespace, QSL("author"));

  for (int i = 0; i < authors.size(); i++) {
    QDomNode auth = authors.at(i);

    if (auth.parentNode() == m_xml.documentElement()) {
      return auth.toElement().elementsByTagNameNS(m_atomNamespace, QSL("name")).at(0).toElement().text();
    }
  }

  return {};
}

QString AtomParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  QDomNodeList authors = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("author"));
  QStringList author_str;

  for (int i = 0; i < authors.size(); i++) {
    QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

    if (!names.isEmpty()) {
      author_str.append(names.at(0).toElement().text());
    }
  }

  return author_str.join(QSL(", "));
}

QString AtomParser::atomNamespace() const {
  return m_atomNamespace;
}

QDomNodeList AtomParser::xmlMessageElements() {
  return m_xml.elementsByTagNameNS(m_atomNamespace, QSL("entry"));
}

QString AtomParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return xmlTextsFromPath(msg_element, m_atomNamespace, QSL("title"), true).join(QSL(", "));
}

QString AtomParser::xmlMessageDescription(const QDomElement& msg_element) const {
  QString summary = xmlRawChild(msg_element.elementsByTagNameNS(m_atomNamespace, QSL("content")).at(0).toElement());

  if (summary.isEmpty()) {
    summary = xmlRawChild(msg_element.elementsByTagNameNS(m_atomNamespace, QSL("summary")).at(0).toElement());

    if (summary.isEmpty()) {
      summary = xmlRawChild(msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("description")).at(0).toElement());
    }
  }

  return summary;
}

QDateTime AtomParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  QString updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("updated"), true).join(QSL(", "));

  if (updated.simplified().isEmpty()) {
    updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("modified"), true).join(QSL(", "));
  }

  return TextFactory::parseDateTime(updated);
}

QString AtomParser::xmlMessageId(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_atomNamespace, QSL("id")).at(0).toElement().text();
}

QString AtomParser::xmlMessageUrl(const QDomElement& msg_element) const {
  QDomNodeList elem_links = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("link"));
  QString last_link_other;

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute.isEmpty() || attribute == QSL("alternate")) {
      return link.attribute(QSL("href"));
    }
    else if (attribute != QSL("enclosure")) {
      last_link_other = link.attribute(QSL("href"));
    }
  }

  if (!last_link_other.isEmpty()) {
    return last_link_other;
  }
  else {
    return {};
  }
}

QList<Enclosure> AtomParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  QList<Enclosure> enclosures;
  QDomNodeList elem_links = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("link"));

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute == QSL("enclosure")) {
      enclosures.append(Enclosure(link.attribute(QSL("href")), link.attribute(QSL("type"))));
    }
  }

  return enclosures;
}
