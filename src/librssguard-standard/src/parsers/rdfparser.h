// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RDFPARSER_H
#define RDFPARSER_H

#include "src/parsers/feedparser.h"

#include "librssguard/core/message.h"

#include <QList>

class RdfParser : public FeedParser {
  public:
    explicit RdfParser(const QString& data);
    virtual ~RdfParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const QString& content_type) const;

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
    QString rdfNamespace() const;
    QString rssNamespace() const;

    QString m_rdfNamespace;
    QString m_rssNamespace;
    QString m_rssCoNamespace;
    QString m_dcElNamespace;
};

#endif // RDFPARSER_H
