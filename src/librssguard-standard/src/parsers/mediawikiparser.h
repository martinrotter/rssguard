// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MEDIAWIKIPARSER_H
#define MEDIAWIKIPARSER_H

#include "src/parsers/feedparser.h"

#include <QHash>

#if defined(rssguard_standard_EXPORTS)
#define MEDIAWIKI_PARSER_DLLSPEC Q_DECL_EXPORT
#else
#define MEDIAWIKI_PARSER_DLLSPEC Q_DECL_IMPORT
#endif

class MEDIAWIKI_PARSER_DLLSPEC MediaWikiParser : public FeedParser {
  public:
    explicit MediaWikiParser(const QString& data,
                             const QUrl& source_url = {},
                             std::function<QByteArray(const QUrl&)> resource_handler = {});

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root,
                                               const QUrl& url,
                                               bool deep_discovery,
                                               const QList<DocumentWithUrl>& documents) const override;
    virtual GuessedFeedWithIcons guessFeed(const QByteArray& content, const NetworkResult& network_res) const override;

  protected:
    virtual QJsonArray jsonMessageElements() override;
    virtual QString jsonMessageTitle(const QJsonObject& item) const override;
    virtual QString jsonMessageUrl(const QJsonObject& item) const override;
    virtual QString jsonMessageDescription(const QJsonObject& item) const override;
    virtual QString jsonMessageAuthor(const QJsonObject& item) const override;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& item) override;
    virtual QString jsonMessageId(const QJsonObject& item) const override;
    virtual QString jsonMessageRawContents(const QJsonObject& item) const override;

  private:
    QUrl m_sourceUrl;
    QHash<qint64, QString> m_articleHtml;
};

#endif // MEDIAWIKIPARSER_H
