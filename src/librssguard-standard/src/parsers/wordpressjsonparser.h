// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WORDPRESSJSONPARSER_H
#define WORDPRESSJSONPARSER_H

#include "src/parsers/feedparser.h"

#include <librssguard/core/message.h>

#include <QHash>

class WordpressJsonParser : public FeedParser {
  public:
    explicit WordpressJsonParser(const QString& data, const QUrl& source_url = {});
    virtual ~WordpressJsonParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root,
                                               const QUrl& url,
                                               bool deep_discovery,
                                               const QList<DocumentWithUrl>& documents) const;

    virtual GuessedFeedWithIcons guessFeed(const QByteArray& content, const NetworkResult& network_res) const;

  protected:
    virtual QJsonArray jsonMessageElements();

    virtual QString jsonMessageTitle(const QJsonObject& msg_element) const;
    virtual QString jsonMessageUrl(const QJsonObject& msg_element) const;
    virtual QString jsonMessageDescription(const QJsonObject& msg_element) const;
    virtual QString jsonMessageAuthor(const QJsonObject& msg_element) const;
    virtual QList<QSharedPointer<MessageEnclosure>> jsonMessageEnclosures(const QJsonObject& msg_element) const;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& msg_element);
    virtual QString jsonMessageId(const QJsonObject& msg_element) const;
    virtual QString jsonMessageRawContents(const QJsonObject& msg_element) const;

  private:
    bool isWordpressItem(const QJsonObject& item) const;
    QJsonDocument linkedResource(const QString& href) const;

    QUrl m_sourceUrl;
    mutable QHash<QString, QJsonDocument> m_linkedResources;
};

#endif // WORDPRESSJSONPARSER_H
