// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/atomparser.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/miscellaneous/xmlencodingdetector.h>

AtomParser::AtomParser(const QString& data)
  : FeedParser(QSL("<feed xmlns=\"http://www.w3.org/2005/Atom\">") + data + QSL("</feed>")) {
  QString version = m_xml.documentElement().attribute(QSL("version"));

  if (version == QSL("0.3")) {
    m_atomNamespace = QSL("http://purl.org/atom/ns#");
  }
  else {
    m_atomNamespace = QSL("http://www.w3.org/2005/Atom");
  }
}

AtomParser::~AtomParser() {}

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

QList<FeedComment> AtomParser::comments(const QDomElement& msg_element) const {
  QDomNodeList links = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("link"));

  for (int i = 0; i < links.size(); i++) {
    QDomElement link_elem = links.at(i).toElement();

    if (link_elem.attribute(QSL("rel")) != QSL("replies") ||
        link_elem.attribute(QSL("type")) != QSL("application/atom+xml")) {
      continue;
    }

    QString comments_atom = link_elem.attribute(QSL("href"));

    if (comments_atom.isEmpty()) {
      continue;
    }

    QByteArray comments_atom_data = m_resourceHandler(comments_atom);
    AtomParser atom_parser(QString::fromUtf8(comments_atom_data));
    QList<Message> extracted_comments = atom_parser.messages();

    if (extracted_comments.isEmpty()) {
      return {};
    }

    QList<FeedComment> cmnts;
    cmnts.reserve(extracted_comments.size());

    for (Message& extracted_comment : extracted_comments) {
      extracted_comment.sanitize(nullptr, false);

      FeedComment cmnt;
      cmnt.m_title = extracted_comment.m_title;
      cmnt.m_contents = extracted_comment.m_contents;
      cmnts.append(cmnt);
    }

    return cmnts;
  }

  return {};
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

  if (fetchComments()) {
    summary += formatComments(comments(msg_element));
  }

  return summary;
}

QDateTime AtomParser::xmlMessageDateCreated(const QDomElement& msg_element) {
  QString published = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("published"), true).join(QSL(", "));
  QString updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("updated"), true).join(QSL(", "));

  if (updated.trimmed().isEmpty()) {
    updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("modified"), true).join(QSL(", "));
  }

  return decideArticleDate(published, updated);
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

QList<QSharedPointer<MessageEnclosure>> AtomParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  QList<QSharedPointer<MessageEnclosure>> enclosures;
  QDomNodeList elem_links = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("link"));

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute == QSL("enclosure")) {
      enclosures.append(QSharedPointer<MessageEnclosure>(new MessageEnclosure(link.attribute(QSL("href")),
                                                                              link.attribute(QSL("type")))));
    }
  }

  return enclosures;
}

QList<QSharedPointer<MessageCategory>> AtomParser::xmlMessageCategories(const QDomElement& msg_element) const {
  QList<QSharedPointer<MessageCategory>> cats;
  QDomNodeList elem_cats = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("category"));

  for (int i = 0; i < elem_cats.size(); i++) {
    QDomElement cat = elem_cats.at(i).toElement();
    QString lbl = cat.attribute(QSL("label"));
    QString term = cat.attribute(QSL("term"));

    cats.append(QSharedPointer<MessageCategory>(new MessageCategory(lbl.isEmpty() ? term : lbl)));
  }

  return cats;
}
