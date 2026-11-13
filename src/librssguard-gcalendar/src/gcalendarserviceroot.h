// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARSERVICEROOT_H
#define GCALENDARSERVICEROOT_H

#include "calendarreminders.h"
#include "gcalendarnetworkfactory.h"

#include <librssguard/services/abstract/serviceroot.h>
#include <mutex>

#include <QFutureWatcher>
#include <QMutex>
#include <QPointer>
#include <QWaitCondition>

class OAuth2Service;
class QTimer;
class GCalendarFeed;
class CalendarEventPreviewer;

class GCalendarServiceRoot : public ServiceRoot {
    Q_OBJECT

  public:
    explicit GCalendarServiceRoot(RootItem* parent = nullptr, GCalendarNetworkFactory::Transport transport = {});
    virtual ~GCalendarServiceRoot() override;

    virtual QString code() const override;
    virtual bool isSyncable() const override;
    virtual bool supportsFeedAdding() const override;
    virtual bool supportsCategoryAdding() const override;
    virtual bool displaysEnclosures() const override;
    virtual bool canBeEdited() const override;
    virtual bool canBeDeleted() const override;
    virtual FormAccountDetails* accountSetupDialog() const override;
    virtual void editItems(const QList<RootItem*>& items) override;
    virtual MessageStoragePolicy messageStoragePolicy() const override;
    virtual QVariantHash customDatabaseData() const override;
    virtual void setCustomDatabaseData(const QVariantHash& data) override;
    virtual void saveAccountDataToDatabase() override;
    virtual QList<ArticleColumn> articleColumns() const override;
    virtual QVariant articleColumnData(const Message& message, const QString& column_id, int role) const override;
    virtual QList<QAction*> contextMenuFeedsList(const QList<RootItem*>& items, QMenu* parent_menu) override;
    virtual QList<QAction*> serviceMenu() override;
    virtual QList<QAction*> addItemMenu() override;
    virtual QList<QAction*> contextMenuMessagesList(const QList<Message>& messages, QMenu* parent_menu) override;
    virtual CustomMessagePreviewer* customMessagePreviewer() override;
    virtual QString additionalTooltip() const override;
    virtual void start(bool freshly_activated) override;
    virtual void stop() override;
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages) override;
    virtual FeedFetchResult obtainNewMessagesForUpdate(Feed* feed,
                                                       const QHash<BagOfMessages, QStringList>& stated_messages,
                                                       const QHash<QString, QStringList>& tagged_messages) override;

    QJsonObject calendarSettings() const;
    CalendarReminderSettings reminderSettings() const;
    void applySettings(const QString& name,
                       const QString& client_id,
                       const QString& client_secret,
                       int history_days,
                       int future_days,
                       CalendarReminderSettings reminders,
                       const QString& refresh_token = QString(),
                       bool refresh_calendar_list_on_startup = false);
    void setCalendarReminderOverride(GCalendarFeed* feed, QJsonObject settings);
    void saveCalendarFeedSettings(const QList<Feed*>& feeds,
                                  const QJsonObject& reminder_override,
                                  const std::function<void(Feed*)>& apply_common = {});
    Message cachedMessage(int message_id) const;
    quint64 loadRemoteEvent(const QString& calendar_id, const QString& event_id);
    quint64 saveRemoteEvent(const QString& calendar_id,
                            const QJsonObject& baseline,
                            const QJsonObject& changes,
                            bool creating,
                            bool notify_attendees,
                            int preferred_message_id = 0);

  public slots:
    void authorize();
    virtual void requestSyncIn() override;
    void checkReminders();
    void openCachedEvent(int message_id);
    void createEvent(const QString& calendar_id = QString());
    void editEvent(const Message& message);

  signals:
    void calendarDiscoveryFinished(bool succeeded);
    void statusChanged(const QString& status);
    void remindersChecked(bool succeeded);
    void eventOperationFinished(quint64 request_id,
                                QJsonObject resource,
                                int status,
                                QString error,
                                bool confirmed,
                                int message_id,
                                bool recovered_creation);
    void eventSaved(int message_id);
    void accountStopped();

  protected:
    virtual GCalendarNetworkFactory::Context requestContext();
    virtual void refreshAccessToken();
    virtual QDateTime reminderNow() const;
    virtual void deliverReminder(const CalendarReminder& reminder);

  private:
    struct Discovery {
        QList<QJsonObject> m_calendars;
        QString m_error;
    };

    struct ReminderScan {
        QList<CalendarReminder> m_due;
        QString m_error;
    };

    struct EventOperation {
        QJsonObject m_resource;
        QString m_error;
        int m_status = 0;
        int m_messageId = 0;
        bool m_confirmed = false;
        bool m_recoveredCreation = false;
    };

    void ensureOAuth();
    void requestAccessToken();
    void authenticationFinished(const QString& error);
    QString authorizedBearer(const QString& rejected, const std::shared_ptr<std::atomic_bool>& cancelled, int timeout);
    void saveAccountDataToDatabase(const QString& refresh_token);
    void loadCachedTree();
    bool applyCalendars(const QList<QJsonObject>& calendars);
    void finishCalendarDiscovery();
    FeedFetchResult fetchSnapshot(Feed* feed, const GCalendarNetworkFactory::Context& context);
    void storeSnapshot(Feed* feed, FeedFetchResult snapshot);
    int storeConfirmedEvent(Feed* feed, const QJsonObject& resource, const GCalendarNetworkFactory::Context& context);
    quint64 startEventOperation(const QString& calendar_id,
                                const QJsonObject& baseline,
                                const QJsonObject& changes,
                                bool creating,
                                bool reading,
                                bool notify_attendees,
                                int preferred_message_id);
    void reconcileSnapshot(int feed_id,
                           const QString& calendar_id,
                           const QByteArray& zone,
                           const QDateTime& from,
                           const QDateTime& until,
                           const QHash<QString, QJsonObject>& resources,
                           const std::shared_ptr<std::atomic_bool>& cancelled,
                           bool retire_missing = true);
    void setStatus(const QString& status);
    void restartReminderTimer(bool check_now);
    void checkpointReminders();
    static QList<CalendarReminder> remindersFromDatabase(const QSqlDatabase& db,
                                                         int account_id,
                                                         const QDateTime& now,
                                                         const QJsonObject& pending,
                                                         const QSet<int>& only_messages = {});
    static QJsonObject metadata(const QVariantHash& data);

    mutable QMutex m_stateMutex;
    QVariantHash m_data;
    GCalendarNetworkFactory::Context m_context;
    QDateTime m_bearerExpires;
    QWaitCondition m_authorizationReady;
    QString m_authorizationError;
    quint64 m_authorizationGeneration = 0;
    bool m_authorizationPending = false;
    GCalendarNetworkFactory::Transport m_transport;
    std::mutex m_mutationMutex;
    QPointer<OAuth2Service> m_oauth;
    QFutureWatcher<Discovery>* m_discovery = nullptr;
    QFutureWatcher<ReminderScan>* m_reminderScan = nullptr;
    QTimer* m_reminderTimer = nullptr;
    QJsonObject m_pendingLedger;
    std::atomic<quint64> m_configurationGeneration{0};
    bool m_checkRunning = false;
    QString m_status;
    QList<QAction*> m_actions;
    QAction* m_createEventAction = nullptr;
    QPointer<CalendarEventPreviewer> m_previewer;
    QList<QFutureWatcher<EventOperation>*> m_eventOperations;
    quint64 m_requestId = 0;
    bool m_loaded = false;
    std::atomic_bool m_started{false};
    bool m_discoverAfterAuth = false;
};

#endif // GCALENDARSERVICEROOT_H
