// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WORDPRESSJSONPARSER_H
#define WORDPRESSJSONPARSER_H

#include "src/parsers/jsonparser.h"

#include <librssguard/core/message.h>

class WordpressJsonParser : public FeedParser { // reuse of JsonParser with the wordpress posts/pages format
  public:
    explicit WordpressJsonParser(const QString& data);
    virtual ~WordpressJsonParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root,
                                               const QUrl& url,
                                               bool deep_discovery,
                                               const QList<DocumentWithUrl>& documents) const;

    virtual GuessedFeedWithIcons guessFeed(const QByteArray& content, const NetworkResult& network_res) const;

  protected:
    //virtual QString feedAuthor() const;
    virtual QJsonArray jsonMessageElements() override; // override of the json format reader because wordpress return directly a JSON array

    virtual QString jsonMessageTitle(const QJsonObject& msg_element) const;
    virtual QString jsonMessageUrl(const QJsonObject& msg_element) const;
    virtual QString jsonMessageDescription(const QJsonObject& msg_element) const;
    virtual QString jsonMessageAuthor(const QJsonObject& msg_element) const;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& msg_element);
    virtual QString jsonMessageId(const QJsonObject& msg_element) const;
    virtual QString jsonMessageRawContents(const QJsonObject& msg_element) const;

    virtual bool isWordpressItem(const QJsonObject& item) const;
};

#endif // WORDPRESSJSONPARSER_H
