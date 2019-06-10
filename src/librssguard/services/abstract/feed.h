// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEED_H
#define FEED_H

#include "services/abstract/rootitem.h"

#include "core/message.h"

#include <QRunnable>
#include <QVariant>

// Base class for "feed" nodes.
class Feed : public RootItem, public QRunnable {
  Q_OBJECT

  public:

    // Specifies the auto-update strategy for the feed.
    enum AutoUpdateType {
      DontAutoUpdate = 0,
      DefaultAutoUpdate = 1,
      SpecificAutoUpdate = 2
    };

    // Specifies the actual "status" of the feed.
    // For example if it has new messages, error
    // occurred, and so on.
    enum Status {
      Normal = 0,
      NewMessages = 1,
      NetworkError = 2,
      AuthError = 3,
      ParsingError = 4,
      OtherError = 5
    };

    // Constructors.
    explicit Feed(RootItem* parent = nullptr);
    explicit Feed(const QSqlRecord& record);
    explicit Feed(const Feed& other);
    virtual ~Feed();

    QList<Message> undeletedMessages() const;

    QString additionalTooltip() const;

    int countOfAllMessages() const;
    int countOfUnreadMessages() const;

    void setCountOfAllMessages(int count_all_messages);
    void setCountOfUnreadMessages(int count_unread_messages);

    QVariant data(int column, int role) const;

    int autoUpdateInitialInterval() const;
    void setAutoUpdateInitialInterval(int auto_update_interval);

    AutoUpdateType autoUpdateType() const;
    void setAutoUpdateType(AutoUpdateType auto_update_type);

    int autoUpdateRemainingInterval() const;
    void setAutoUpdateRemainingInterval(int auto_update_remaining_interval);

    Status status() const;
    void setStatus(const Status& status);

    QString url() const;
    void setUrl(const QString& url);

    // Runs update in thread (thread pooled).
    void run();

    bool markAsReadUnread(ReadStatus status);
    bool cleanMessages(bool clean_read_only);

  public slots:
    void updateCounts(bool including_total_count);
    int updateMessages(const QList<Message>& messages, bool error_during_obtaining);

  protected:
    QString getAutoUpdateStatusDescription() const;
    QString getStatusDescription() const;

  signals:
    void messagesObtained(QList<Message> messages, bool error_during_obtaining);

  private:

    // Performs synchronous obtaining of new messages for this feed.
    virtual QList<Message> obtainNewMessages(bool* error_during_obtaining) = 0;

  private:
    QString m_url;
    Status m_status;
    AutoUpdateType m_autoUpdateType;
    int m_autoUpdateInitialInterval{};
    int m_autoUpdateRemainingInterval{};
    int m_totalCount{};
    int m_unreadCount{};
};

Q_DECLARE_METATYPE(Feed::AutoUpdateType)

#endif // FEED_H
