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
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QString xmlMessageAuthor(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element) const;
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> xmlMessageEnclosures(const QDomElement& msg_element) const;
    virtual QDomNodeList xmlMessageElements();

  private:
    QString m_rdfNamespace;
    QString m_rssNamespace;
    QString m_rssCoNamespace;
    QString m_dcElNamespace;
};

#endif // RDFPARSER_H
