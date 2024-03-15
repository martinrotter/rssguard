// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEED_H
#define FEED_H

#include "services/abstract/rootitem.h"

#include "core/message.h"
#include "core/messagefilter.h"

#include <QPointer>
#include <QVariant>

// Base class for "feed" nodes.
class RSSGUARD_DLLSPEC Feed : public RootItem {
    Q_OBJECT

  public:
    struct ArticleIgnoreLimit {
        // Ignoring articles.
        bool m_avoidOldArticles = false;
        bool m_addAnyArticlesToDb = false;
        QDateTime m_dtToAvoid = QDateTime();
        int m_hoursToAvoid = 0;

        // Limitting articles.
        bool m_customizeLimitting = false;
        int m_keepCountOfArticles = 0;
        bool m_doNotRemoveStarred = true;
        bool m_doNotRemoveUnread = true;
        bool m_moveToBinDontPurge = false;

        static ArticleIgnoreLimit fromSettings();
    };

    // Specifies the auto-download strategy for the feed.
    enum class AutoUpdateType {
      DontAutoUpdate = 0,
      DefaultAutoUpdate = 1,
      SpecificAutoUpdate = 2
    };

    // Specifies the actual "status" of the feed.
    // For example if it has new messages, error
    // occurred, and so on.
    enum class Status {
      Normal = 0,
      NewMessages = 1,
      NetworkError = 2,
      AuthError = 3,
      ParsingError = 4,
      OtherError = 5,
      Fetching = 6
    };

    Q_ENUM(Status)

    explicit Feed(RootItem* parent = nullptr);
    explicit Feed(const Feed& other);
    explicit Feed(const QString& title, const QString& custom_id, const QIcon& icon, RootItem* parent = nullptr);

    virtual QList<Message> undeletedMessages() const;
    virtual QString additionalTooltip() const;
    virtual Qt::ItemFlags additionalFlags() const;
    virtual bool markAsReadUnread(ReadStatus status);
    virtual bool cleanMessages(bool clean_read_only);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual bool canBeEdited() const;
    virtual bool isFetching() const;
    virtual QVariant data(int column, int role) const;

    void setCountOfAllMessages(int count_all_messages);
    void setCountOfUnreadMessages(int count_unread_messages);

    int autoUpdateInterval() const;
    void setAutoUpdateInterval(int auto_update_interval);

    AutoUpdateType autoUpdateType() const;
    void setAutoUpdateType(AutoUpdateType auto_update_type);

    Status status() const;
    QString statusString() const;
    void setStatus(Feed::Status status, const QString& status_text = {});

    QString source() const;
    void setSource(const QString& source);

    bool openArticlesDirectly() const;
    void setOpenArticlesDirectly(bool opn);

    bool isSwitchedOff() const;
    void setIsSwitchedOff(bool switched_off);

    bool isQuiet() const;
    void setIsQuiet(bool quiet);

    void appendMessageFilter(MessageFilter* filter);
    QList<QPointer<MessageFilter>> messageFilters() const;
    void setMessageFilters(const QList<QPointer<MessageFilter>>& messageFilters);
    void removeMessageFilter(MessageFilter* filter);

    QDateTime lastUpdated() const;
    void setLastUpdated(const QDateTime& last_updated);

    bool isRtl() const;
    void setIsRtl(bool rtl);

    bool removeUnwantedArticles(QSqlDatabase& db);

    ArticleIgnoreLimit& articleIgnoreLimit();
    const ArticleIgnoreLimit& articleIgnoreLimit() const;
    void setArticleIgnoreLimit(const ArticleIgnoreLimit& ignore_limit);

  public slots:
    virtual void updateCounts(bool including_total_count);

  protected:
    QString getAutoUpdateStatusDescription() const;
    QString getStatusDescription() const;

  private:
    QString m_source;
    Status m_status;
    QString m_statusString;
    AutoUpdateType m_autoUpdateType;
    int m_autoUpdateInterval{}; // In seconds.
    QDateTime m_lastUpdated;
    bool m_isSwitchedOff;
    bool m_isQuiet;
    bool m_openArticlesDirectly;
    bool m_isRtl;

    // NOTE: These are used to filter out older articles
    // than needed. Either absolute value is given (date/time)
    // or relative value given in minutes.
    // Amount
    ArticleIgnoreLimit m_articleIgnoreLimit;

    int m_totalCount{};
    int m_unreadCount{};
    QList<QPointer<MessageFilter>> m_messageFilters;
};

Q_DECLARE_METATYPE(Feed::AutoUpdateType)
Q_DECLARE_METATYPE(Feed::Status)
Q_DECLARE_METATYPE(Feed::ArticleIgnoreLimit)

#endif // FEED_H
