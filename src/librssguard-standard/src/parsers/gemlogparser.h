// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GEMLOGPARSER_H
#define GEMLOGPARSER_H

#include "src/parsers/feedparser.h"

struct GemlogEntry {
    QDateTime m_date;
    QString m_link;
    QString m_title;
    QString m_rawData;
};

Q_DECLARE_METATYPE(GemlogEntry)

class GemlogParser : public FeedParser {
  public:
    explicit GemlogParser(const QString& data);
    virtual ~GemlogParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res) const;

    virtual QVariantList objMessageElements();
    virtual QString objMessageTitle(const QVariant& msg_element) const;
    virtual QString objMessageUrl(const QVariant& msg_element) const;
    virtual QString objMessageDescription(const QVariant& msg_element);
    virtual QString objMessageAuthor(const QVariant& msg_element) const;
    virtual QDateTime objMessageDateCreated(const QVariant& msg_element);
    virtual QString objMessageId(const QVariant& msg_element) const;
    virtual QString objMessageRawContents(const QVariant& msg_element) const;

  private:
    QString extractFeedTitle(const QString& gemlog) const;
    QVariantList extractFeedEntries(const QString& gemlog);

    QVariantList m_entries;
};

#endif // GEMLOGPARSER_H
