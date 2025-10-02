// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SITEMAPPARSER_H
#define SITEMAPPARSER_H

#include "src/parsers/feedparser.h"
#include "src/standardfeed.h"

class SitemapParser : public FeedParser {
  public:
    explicit SitemapParser(const QString& data);
    virtual ~SitemapParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res) const;

    static bool isGzip(const QByteArray& content);

  protected:
    virtual QDomNodeList xmlMessageElements();
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element);
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QList<Enclosure*> xmlMessageEnclosures(const QDomElement& msg_element) const;

  private:
    QString sitemapNamespace() const;
    QString sitemapNewsNamespace() const;
    QString sitemapImageNamespace() const;
    QString sitemapVideoNamespace() const;
};

#endif // SITEMAPPARSER_H
