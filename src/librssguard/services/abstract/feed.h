// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEED_H
#define FEED_H

#include "services/abstract/rootitem.h"

#include "core/message.h"
#include "core/messagefilter.h"

#include <QPointer>
#include <QVariant>

// Base class for "feed" nodes.
class Feed : public RootItem {
  Q_OBJECT

  public:

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
      OtherError = 5
    };

    Q_ENUM(Status)

    explicit Feed(RootItem* parent = nullptr);
    explicit Feed(const Feed& other);
    explicit Feed(const QString& title, const QString& custom_id, const QIcon& icon, RootItem* parent = nullptr);

    virtual QList<Message> undeletedMessages() const;
    virtual QString additionalTooltip() const;
    virtual bool markAsReadUnread(ReadStatus status);
    virtual bool cleanMessages(bool clean_read_only);
    virtual int countOfAllMessages() const;
    virtual int countOfUnreadMessages() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual QVariant data(int column, int role) const;

    void setCountOfAllMessages(int count_all_messages);
    void setCountOfUnreadMessages(int count_unread_messages);

    int autoUpdateInitialInterval() const;
    void setAutoUpdateInitialInterval(int auto_update_interval);

    AutoUpdateType autoUpdateType() const;
    void setAutoUpdateType(AutoUpdateType auto_update_type);

    int autoUpdateRemainingInterval() const;
    void setAutoUpdateRemainingInterval(int auto_update_remaining_interval);

    Status status() const;
    QString statusString() const;
    void setStatus(Feed::Status status, const QString& status_text = {});

    QString source() const;
    void setSource(const QString& source);

    bool openArticlesDirectly() const;
    void setOpenArticlesDirectly(bool opn);

    bool isSwitchedOff() const;
    void setIsSwitchedOff(bool switched_off);

    void appendMessageFilter(MessageFilter* filter);
    QList<QPointer<MessageFilter>> messageFilters() const;
    void setMessageFilters(const QList<QPointer<MessageFilter>>& messageFilters);
    void removeMessageFilter(MessageFilter* filter);

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
    int m_autoUpdateInitialInterval{};
    int m_autoUpdateRemainingInterval{};
    bool m_isSwitchedOff;
    bool m_openArticlesDirectly;
    int m_totalCount{};
    int m_unreadCount{};
    QList<QPointer<MessageFilter>> m_messageFilters;
};

Q_DECLARE_METATYPE(Feed::AutoUpdateType)

#endif // FEED_H
