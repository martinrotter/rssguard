// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RDFPARSER_H
#define RDFPARSER_H

#include "services/standard/feedparser.h"

#include "core/message.h"

#include <QList>

class RdfParser : public FeedParser {
  public:
    explicit RdfParser(const QString& data);

    QString rdfNamespace() const;
    QString rssNamespace() const;

  private:
    QDomNodeList messageElements();
    Message extractMessage(const QDomElement& msg_element, QDateTime current_time) const;

    QString m_rdfNamespace;
    QString m_rssNamespace;
};

#endif // RDFPARSER_H
