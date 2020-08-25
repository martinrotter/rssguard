// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/atomparser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

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

AtomParser::~AtomParser() = default;

QString AtomParser::feedAuthor() const {
  QDomNodeList authors = m_xml.documentElement().elementsByTagNameNS(m_atomNamespace, QSL("author"));
  QStringList author_str;

  for (int i = 0; i < authors.size(); i++) {
    QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

    if (!names.isEmpty()) {
      const QString name = names.at(0).toElement().text();

      if (!name.isEmpty() && !author_str.contains(name)) {
        author_str.append(name);
      }
    }
  }

  return author_str.join(", ");
}

Message AtomParser::extractMessage(const QDomElement& msg_element, QDateTime current_time) const {
  Message new_message;
  QString title = textsFromPath(msg_element, m_atomNamespace, QSL("title"), true).join(QSL(", "));
  QString summary = textsFromPath(msg_element, m_atomNamespace, QSL("content"), true).join(QSL(", "));

  if (summary.isEmpty()) {
    summary = textsFromPath(msg_element, m_atomNamespace, QSL("summary"), true).join(QSL(", "));

    if (summary.isEmpty()) {
      summary = mrssTextFromPath(msg_element, QSL("description"));
    }
  }

  // Now we obtained maximum of information for title & description.
  if (title.isEmpty() && summary.isEmpty()) {
    // BOTH title and description are empty, skip this message.
    throw ApplicationException(QSL("Not enough data for the message."));
  }

  // Title is not empty, description does not matter.
  new_message.m_title = qApp->web()->unescapeHtml(qApp->web()->stripTags(title));
  new_message.m_contents = summary;
  new_message.m_author = qApp->web()->unescapeHtml(messageAuthor(msg_element));

  QString updated = textsFromPath(msg_element, m_atomNamespace, QSL("updated"), true).join(QSL(", "));

  if (updated.isEmpty()) {
    updated = textsFromPath(msg_element, m_atomNamespace, QSL("modified"), true).join(QSL(", "));
  }

  // Deal with creation date.
  new_message.m_created = TextFactory::parseDateTime(updated);
  new_message.m_createdFromFeed = !new_message.m_created.isNull();

  if (!new_message.m_createdFromFeed) {
    // Date was NOT obtained from the feed, set current date as creation date for the message.
    new_message.m_created = current_time;
  }

  // Deal with links
  QDomNodeList elem_links = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("link"));
  QString last_link_alternate, last_link_other;

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute == QSL("enclosure")) {
      new_message.m_enclosures.append(Enclosure(link.attribute(QSL("href")), link.attribute(QSL("type"))));
      qDebugNN << LOGSEC_CORE
               << "Found enclosure"
               << QUOTE_W_SPACE(new_message.m_enclosures.last().m_url)
               << "for the message.";
    }
    else if (attribute.isEmpty() || attribute == QSL("alternate")) {
      last_link_alternate = link.attribute(QSL("href"));
    }
    else {
      last_link_other = link.attribute(QSL("href"));
    }
  }

  // Obtain MRSS enclosures.
  new_message.m_enclosures.append(mrssGetEnclosures(msg_element));

  if (!last_link_alternate.isEmpty()) {
    new_message.m_url = last_link_alternate;
  }
  else if (!last_link_other.isEmpty()) {
    new_message.m_url = last_link_other;
  }
  else if (!new_message.m_enclosures.isEmpty()) {
    new_message.m_url = new_message.m_enclosures.first().m_url;
  }

  return new_message;
}

QString AtomParser::messageAuthor(const QDomElement& msg_element) const {
  QDomNodeList authors = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("author"));
  QStringList author_str;

  for (int i = 0; i < authors.size(); i++) {
    QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

    if (!names.isEmpty()) {
      author_str.append(names.at(0).toElement().text());
    }
  }

  return author_str.join(", ");
}

QDomNodeList AtomParser::messageElements() {
  return m_xml.elementsByTagNameNS(m_atomNamespace, QSL("entry"));
}
