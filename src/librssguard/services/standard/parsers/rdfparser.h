// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RDFPARSER_H
#define RDFPARSER_H

#include "services/standard/parsers/feedparser.h"

#include "core/message.h"

#include <QList>

class RdfParser : public FeedParser {
  public:
    explicit RdfParser(const QString& data);

    QString rdfNamespace() const;
    QString rssNamespace() const;

  protected:
    virtual QString messageTitle(const QDomElement& msg_element) const;
    virtual QString messageDescription(const QDomElement& msg_element) const;
    virtual QString messageAuthor(const QDomElement& msg_element) const;
    virtual QDateTime messageDateCreated(const QDomElement& msg_element) const;
    virtual QString messageId(const QDomElement& msg_element) const;
    virtual QString messageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> messageEnclosures(const QDomElement& msg_element) const;
    virtual QDomNodeList messageElements();

  private:
    QString m_rdfNamespace;
    QString m_rssNamespace;
    QString m_rssCoNamespace;
    QString m_dcElNamespace;
};

#endif // RDFPARSER_H
