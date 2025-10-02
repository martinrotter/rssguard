// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "src/parsers/feedparser.h"

#include <librssguard/core/message.h>

class JsonParser : public FeedParser {
  public:
    explicit JsonParser(const QString& data);
    virtual ~JsonParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res) const;

  protected:
    virtual QString feedAuthor() const;
    virtual QJsonArray jsonMessageElements();

    virtual QString jsonMessageTitle(const QJsonObject& msg_element) const;
    virtual QString jsonMessageUrl(const QJsonObject& msg_element) const;
    virtual QString jsonMessageDescription(const QJsonObject& msg_element) const;
    virtual QString jsonMessageAuthor(const QJsonObject& msg_element) const;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& msg_element);
    virtual QString jsonMessageId(const QJsonObject& msg_element) const;
    virtual QList<Enclosure*> jsonMessageEnclosures(const QJsonObject& msg_element) const;
    virtual QString jsonMessageRawContents(const QJsonObject& msg_element) const;
};

#endif // JSONPARSER_H
