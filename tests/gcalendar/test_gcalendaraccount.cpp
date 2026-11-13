// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gcalendarfeed.h"
#include "src/gcalendarserviceroot.h"
#include "src/gui/calendareventpreviewer.h"
#include "src/gui/formcalendarevent.h"
#include "src/gui/formeditgcalendaraccount.h"
#include "src/gui/formgcalendarfeeddetails.h"
#include "src/gui/gcalendaraccountdetails.h"
#include "src/gui/gcalendarreminderdetails.h"

#include <algorithm>
#include <librssguard/database/databasequeries.h>
#include <librssguard/gui/dialogs/formmain.h>
#include <librssguard/gui/feedmessageviewer.h>
#include <librssguard/gui/feedsview.h>
#include <librssguard/gui/messagesview.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/feedreader.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/miscellaneous/pluginfactory.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/oauth2service.h>
#include <librssguard/services/abstract/recyclebin.h>
#include <librssguard/services/abstract/serviceentrypoint.h>
#include <memory>

#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QGroupBox>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlRecord>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent>

namespace {
  void saveSnapshot(QWidget* widget, const QString& name) {
    const auto path = qEnvironmentVariable("RSSGUARD_TEST_UI_SNAPSHOTS");
    if (path.isEmpty()) {
      return;
    }
    QVERIFY(QDir().mkpath(path));
    widget->ensurePolished();
    QCoreApplication::processEvents();
    QVERIFY(widget->grab().save(QDir(path).filePath(name + QSL(".png"))));
  }

  class AccountDialog : public FormEditGCalendarAccount {
    public:
      explicit AccountDialog(GCalendarServiceRoot* account) {
        m_account = account;
        loadAccountData();
      }
      using FormEditGCalendarAccount::apply;
  };

  class FeedDialog : public FormGCalendarFeedDetails {
    public:
      FeedDialog(GCalendarServiceRoot* account, const QList<Feed*>& feeds) : FormGCalendarFeedDetails(account) {
        m_creatingNew = false;
        m_isBatchEdit = feeds.size() > 1;
        m_feeds = feeds;
        loadFeedData();
      }
      using FormGCalendarFeedDetails::apply;
  };

  QJsonObject localOverride(int before, int repeat = 0) {
    return {{QSL("mode"), QSL("local")},
            {QSL("rules"),
             QJsonArray{QJsonObject{{QSL("id"), QSL("local-%1").arg(before)},
                                    {QSL("minutes_before"), before},
                                    {QSL("repeat_minutes"), repeat}}}}};
  }

  QJsonObject storedCalendarData(const QVariantHash& row) {
    return QJsonDocument::fromJson(row.value(QSL("custom_data")).toString().toUtf8()).object();
  }

  struct Server {
      QMutex m_mutex;
      QNetworkProxy m_lastProxy;
      QList<QJsonObject> m_calendars;
      QList<QJsonObject> m_events;
      bool m_failCalendarPage = false;
      bool m_failEventPage = false;
      int m_eventCalls = 0;
      QByteArray m_rejectedBearer;
      bool m_rejectAllBearers = false;

      int requestCount() {
        QMutexLocker lock(&m_mutex);
        return m_requests.size();
      }
      int m_ambiguousInserts = 0;
      int m_version = 0;
      bool m_filterWindow = false;
      int m_firstInstanceOffset = 0;
      QHash<QString, QJsonObject> m_resources;
      QList<GCalendarNetworkFactory::Request> m_requests;
      std::atomic_bool m_holdWrites{false};
      std::atomic_bool m_holdReads{false};
      QSemaphore m_writeEntered;
      QSemaphore m_readEntered;

      static GCalendarNetworkFactory::Response response(int status, const QJsonObject& body) {
        return {status, QJsonDocument(body).toJson(), {}, {}};
      }

      void store(const QJsonObject& resource) {
        const auto id = resource.value(QSL("id")).toString();
        m_resources[id] = resource;
        m_events.erase(std::remove_if(m_events.begin(),
                                      m_events.end(),
                                      [&](const QJsonObject& event) {
                                        return event.value(QSL("id")).toString() == id ||
                                               event.value(QSL("recurringEventId")).toString() == id;
                                      }),
                       m_events.end());
        if (resource.value(QSL("recurrence")).toArray().isEmpty()) {
          m_events.append(resource);
        }
        else {
          for (int day = 0; day < 2; ++day) {
            auto occurrence = resource;
            occurrence.remove(QSL("recurrence"));
            occurrence[QSL("id")] = QString(id + QSL("_instance_%1").arg(day));
            occurrence[QSL("recurringEventId")] = id;
            for (const auto& key : {QSL("start"), QSL("end")}) {
              auto time = resource.value(key).toObject();
              if (time.contains(QSL("date"))) {
                time[QSL("date")] = QDate::fromString(time.value(QSL("date")).toString(), Qt::ISODate)
                                      .addDays(day + m_firstInstanceOffset)
                                      .toString(Qt::ISODate);
              }
              else {
                time[QSL("dateTime")] = QDateTime::fromString(time.value(QSL("dateTime")).toString(), Qt::ISODate)
                                          .addDays(day + m_firstInstanceOffset)
                                          .toString(Qt::ISODate);
              }
              occurrence[key] = time;
            }
            occurrence[QSL("originalStartTime")] = occurrence.value(QSL("start"));
            m_events.append(occurrence);
          }
        }
      }

      GCalendarNetworkFactory::Response reply(const GCalendarNetworkFactory::Request& request,
                                              const GCalendarNetworkFactory::Context& context) {
        if (request.m_method == "GET" && m_holdReads.load()) {
          m_readEntered.release();
          while (m_holdReads.load() && !context.m_cancelled->load()) {
            QThread::msleep(1);
          }
          if (context.m_cancelled->load()) {
            return {0, {}, {}, QSL("Fixture cancelled")};
          }
        }
        if (request.m_method != "GET" && m_holdWrites.load()) {
          m_writeEntered.release();
          while (m_holdWrites.load() && !context.m_cancelled->load()) {
            QThread::msleep(1);
          }
          if (context.m_cancelled->load()) {
            return {0, {}, {}, QSL("Fixture cancelled")};
          }
        }
        QMutexLocker lock(&m_mutex);
        m_lastProxy = context.m_proxy;
        m_requests.append(request);
        if (m_rejectAllBearers ||
            (!m_rejectedBearer.isEmpty() && request.m_headers.value("Authorization") == m_rejectedBearer)) {
          return response(401, {{QSL("error"), QJsonObject{{QSL("message"), QSL("Fixture rejected token")}}}});
        }
        const auto path = request.m_url.path();
        const bool events = path.contains(QSL("/events"));
        const bool collection = path.endsWith(QSL("/events"));
        const bool instances = path.endsWith(QSL("/instances"));
        if (request.m_method != "GET") {
          const auto body = QJsonDocument::fromJson(request.m_body).object();
          const auto id = collection ? body.value(QSL("id")).toString() : path.section(QLatin1Char('/'), -1);
          QJsonObject previous = m_resources.value(id);
          if (previous.isEmpty()) {
            for (const auto& event : m_events) {
              if (event.value(QSL("id")).toString() == id) {
                previous = event;
                break;
              }
            }
          }
          if (request.m_method == "POST" && !previous.isEmpty()) {
            return response(409, {{QSL("error"), QJsonObject{{QSL("message"), QSL("Duplicate fixture ID")}}}});
          }
          if (request.m_method == "PATCH" &&
              request.m_headers.value("If-Match") != previous.value(QSL("etag")).toString().toUtf8()) {
            return response(412, {{QSL("error"), QJsonObject{{QSL("message"), QSL("Fixture conflict")}}}});
          }
          auto saved = previous;
          for (auto it = body.constBegin(); it != body.constEnd(); ++it) {
            saved[it.key()] = it.value();
          }
          saved[QSL("id")] = id;
          saved[QSL("etag")] = QSL("etag-%1").arg(++m_version);
          store(saved);
          if (request.m_method == "POST" && m_ambiguousInserts > 0) {
            --m_ambiguousInserts;
            return {0, {}, {}, QSL("Connection lost after fixture insert")};
          }
          return response(200, saved);
        }
        if (events && !collection && !instances) {
          const auto id = path.section(QLatin1Char('/'), -1);
          if (m_resources.contains(id)) {
            return response(200, m_resources.value(id));
          }
          for (const auto& event : m_events) {
            if (event.value(QSL("id")).toString() == id) {
              return response(200, event);
            }
          }
          return response(404, {{QSL("error"), QJsonObject{{QSL("message"), QSL("Fixture not found")}}}});
        }
        const bool fail_page = events ? m_failEventPage : m_failCalendarPage;
        if (events) {
          ++m_eventCalls;
        }
        if (fail_page && !QUrlQuery(request.m_url).queryItemValue(QSL("pageToken")).isEmpty()) {
          return {500, QByteArray(), {}, QSL("fixture page failed")};
        }
        QJsonArray items;
        for (const auto& item : events ? m_events : m_calendars) {
          if (instances && item.value(QSL("recurringEventId")).toString() != path.section(QLatin1Char('/'), -2, -2)) {
            continue;
          }
          if (events && m_filterWindow) {
            const QUrlQuery query(request.m_url);
            const auto from = QDateTime::fromString(query.queryItemValue(QSL("timeMin")), Qt::ISODate);
            const auto until = QDateTime::fromString(query.queryItemValue(QSL("timeMax")), Qt::ISODate);
            const auto event = CalendarEvent::fromJson(QSL("first"), item, "Europe/Prague");
            if (event.m_start.isValid() && event.m_end.isValid() && (event.m_end <= from || event.m_start >= until)) {
              continue;
            }
          }
          items.append(item);
        }
        QJsonObject resource{{QSL("kind"), events ? QSL("calendar#events") : QSL("calendar#calendarList")},
                             {QSL("items"), items}};
        if (fail_page) {
          resource[QSL("nextPageToken")] = QSL("next");
        }
        return {200, QJsonDocument(resource).toJson(), {}, {}};
      }
  };

  class SetupDetails : public GCalendarAccountDetails {
    public:
      explicit SetupDetails(const std::shared_ptr<Server>& server, bool deny_events = false)
        : GCalendarAccountDetails(nullptr, [server, deny_events](const auto& request, const auto& context) {
            if (deny_events && request.m_url.path().contains(QSL("/events"))) {
              return Server::response(403,
                                      {{QSL("error"), QJsonObject{{QSL("message"), QSL("Calendar access denied")}}}});
            }
            return server->reply(request, context);
          }) {
        m_ui.m_txtName->lineEdit()->setText(QSL("Setup fixture"));
        m_ui.m_txtClientId->lineEdit()->setText(QSL("fixture-client"));
        m_ui.m_txtClientSecret->lineEdit()->setText(QSL("fixture-secret"));
      }

      OAuth2Service* m_authorization = nullptr;
      QUrl m_authorizationUrl;

      void grant(const QString& access = QSL("setup-access"), const QString& refresh = QSL("tested-refresh")) {
        m_authorization->setAccessToken(access);
        m_authorization->setRefreshToken(refresh);
        m_authorization->setTokensExpireIn(QDateTime::currentDateTimeUtc().addSecs(3600));
        emit m_authorization->tokensRetrieved(access, refresh, 3600);
      }

    protected:
      virtual void startAuthorization(OAuth2Service* oauth) override {
        m_authorization = oauth;
        m_authorizationUrl = oauth->createAuthorizationUrl();
      }
  };

  class Account : public GCalendarServiceRoot {
    public:
      explicit Account(const std::shared_ptr<Server>& server)
        : GCalendarServiceRoot(nullptr, [server](const auto& request, const auto& context) {
            return server->reply(request, context);
          }) {}

      QDateTime m_now = QDateTime::currentDateTimeUtc();
      QList<CalendarReminder> m_delivered;
      int m_refreshes = 0;
      bool m_delayAuthorization = false;
      QString m_refreshError;

      void grantAuthorization() {
        auto* oauth = findChild<OAuth2Service*>();
        oauth->setTokensExpireIn(QDateTime::currentDateTimeUtc().addSecs(3600));
        emit oauth->tokensRetrieved(QSL("renewed"), {}, 3600);
      }

      GCalendarNetworkFactory::Context networkContext() {
        return requestContext();
      }

      virtual void start(bool) override {
        GCalendarServiceRoot::start(false);
        auto* oauth = findChild<OAuth2Service*>();
        oauth->setTokensExpireIn(QDateTime::currentDateTimeUtc().addSecs(3600));
        emit oauth->tokensRetrieved(QSL("fixture"), {}, 3600);
      }

    protected:
      virtual void refreshAccessToken() override {
        ++m_refreshes;
        if (!m_delayAuthorization) {
          QPointer<OAuth2Service> oauth = findChild<OAuth2Service*>();
          QTimer::singleShot(0, this, [this, oauth]() {
            if (oauth == nullptr) {
              return;
            }
            if (m_refreshError.isEmpty()) {
              grantAuthorization();
            }
            else {
              emit findChild<OAuth2Service*>() -> tokensRetrieveError({}, m_refreshError);
            }
          });
        }
      }

      virtual QDateTime reminderNow() const override {
        return m_now;
      }
      virtual void deliverReminder(const CalendarReminder& reminder) override {
        m_delivered.append(reminder);
      }
      virtual GCalendarNetworkFactory::Context requestContext() override {
        auto context = GCalendarServiceRoot::requestContext();
        context.m_retryDelay = 0;
        return context;
      }
  };

  QJsonObject calendar(const QString& id, const QString& title) {
    return {{QSL("id"), id},
            {QSL("summary"), title},
            {QSL("timeZone"), QSL("Europe/Prague")},
            {QSL("accessRole"), QSL("owner")}};
  }

  QJsonObject calendarEvent(const QString& id, const QString& title = QSL("Meeting")) {
    const auto now = QDateTime::currentDateTimeUtc().addDays(1);
    return {{QSL("id"), id},
            {QSL("summary"), title},
            {QSL("status"), QSL("confirmed")},
            {QSL("etag"), QSL("etag-one")},
            {QSL("start"), QJsonObject{{QSL("dateTime"), now.toString(Qt::ISODate)}}},
            {QSL("end"), QJsonObject{{QSL("dateTime"), now.addSecs(3600).toString(Qt::ISODate)}}}};
  }

  struct Record {
      int m_id;
      bool m_read;
      bool m_important;
      bool m_deleted;
      bool m_permanent;
      QString m_data;
      qint64 m_retrieved;
  };
} // namespace

class TestGCalendarAccount : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void cleanup();
    void discoveryPreservesFeedSettingsAndFreshMetadata();
    void calendarIconsPersistAndFollowColors();
    void startupCalendarDiscovery_data();
    void startupCalendarDiscovery();
    void startupFetchWaitsForAuthorization();
    void startupFetchWaitsForAuthorization_data();
    void rejectedTokenRefreshesOnce();
    void rejectedTokenRefreshesOnce_data();
    void concurrentRequestsShareAuthorization();
    void calendarDiscoveryWaitsForUpdates();
    void stoppingAccountCancelsAuthorizationWait();
    void authorizationWaitTimesOut();
    void failedAuthorizationPreservesCache();
    void failedDiscoveryPreservesTree();
    void settingsPreserveFreshTokensLedgerAndUnknownKeys();
    void failedSettingsPreserveCorruptAccount();
    void failedSettingsPreserveCorruptAccount_data();
    void failedDialogSettingsRestoreRuntimeState();
    void removalInvalidatesPendingExpansions();
    void completeSnapshotsReconcileMissingAndCancelledEvents();
    void failedPageAndSavePreserveCachedEvents();
    void localFieldsAndTombstonesSurviveRefresh();
    void cancelledOccurrenceCanBeReinstated();
    void oauthPkceFlowUsesActualRedirectAndVerifier();
    void remindersUseDatabaseWithoutFetchingAndGroupRules();
    void reminderCheckpointFailureDoesNotRepeatDelivery();
    void remindersSurviveAccountRestart();
    void calendarOverrideAndRetirementUseCustomData();
    void cacheWindowCoversCalendarOverrides();
    void accountDialogUsesDesignerForms();
    void calendarFeedEditorUsesSharedReminderTab();
    void batchCalendarEditorKeepsOverridesByDefault();
    void batchCalendarEditorAppliesSelectedMode_data();
    void batchCalendarEditorAppliesSelectedMode();
    void failedCalendarEditsPreserveDatabaseAndMemory_data();
    void failedCalendarEditsPreserveDatabaseAndMemory();
    void cancelledCalendarEditPreservesSelection();
    void calendarReminderWidgetPreservesDependencies();
    void calendarCommonSettingsPreserveRemoteData_data();
    void calendarCommonSettingsPreserveRemoteData();
    void commonOnlyCalendarEditsPreserveReminders_data();
    void commonOnlyCalendarEditsPreserveReminders();
    void emptyOffsetsPreserveRepeatAfterReopening_data();
    void emptyOffsetsPreserveRepeatAfterReopening();
    void calendarEditorRejectsUnsupportedDataWithoutLockingUpdates();
    void calendarEditorRejectsBusySave();
    void setupTestUsesEnteredSettingsAndStagesGrant();
    void setupTestFailuresKeepAccountUntouched_data();
    void setupTestFailuresKeepAccountUntouched();
    void closingSetupCancelsNetworkCheck();
    void createsEditsAndPreservesLocalAndRemoteFields();
    void ambiguousCreationReusesDraftId();
    void ambiguousCreationReusesDraftId_data();
    void editorConflictKeepsDraftAndReappliesSparseChanges();
    void seriesOperationsStoreOccurrencesAndPreserveRecurrence();
    void savedEventsOutsideWindowRemainVisible();
    void failedCacheWriteReportsRemoteSuccess();
    void stopCancelsOwnedWriteWithoutLateCacheUpdates();
    void stopCancelsNativeReadWithoutRetiringCache();
    void expiredTombstonesRetireConservatively();
    void queuedEditFollowsSnapshotStorage();
    void previewUsesCachedDataAndRespectsPermissions();
    void remindersContinueDuringRemoteReads();
    void localDeleteRestoreUsesCommonCountRefresh();
    void seriesOutsideWindowPinsARealOccurrence();
    void writesRecheckCurrentCalendarPermissions();
    void unsupportedDatesPreserveCacheAndDoNotSendWrites();
    void oldSeriesCachesModernOccurrences();
    void unsupportedMetadataPreservesCache_data();
    void unsupportedMetadataPreservesCache();
    void bundledPluginsLoadIndependentCachedRoots();
    void calendarLayoutsSurviveRecreationAndAbsence_data();
    void calendarLayoutsSurviveRecreationAndAbsence();

  private:
    bool discover();
    QList<Feed*> calendarsForEditing();
    QHash<int, QVariantHash> feedRows() const;
    void fetch();
    void restartAccount(int refresh_calendar_list = -1);
    GCalendarFeed* feed() const;
    QHash<QString, Record> records() const;
    bool scan();
    QDateTime prepareReminder();
    QList<QVariant> operation(const std::function<quint64()>& start);
    std::shared_ptr<Server> m_server;
    Account* m_account = nullptr;
};

void TestGCalendarAccount::init() {
  m_server = std::make_shared<Server>();
  m_server->m_calendars = {calendar(QSL("first"), QSL("First calendar"))};
  m_account = new Account(m_server);
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 365, {});
  qApp->feedReader()->feedsModel()->addServiceAccount(m_account, false);
}

void TestGCalendarAccount::cleanup() {
  if (m_account == nullptr) {
    return;
  }
  m_account->stop();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    for (const auto& name :
         {QSL("reject_calendar_update"), QSL("reject_calendar_insert"), QSL("reject_reminder_checkpoint")}) {
      SqlQuery query(db);
      query.exec(QSL("DROP TRIGGER IF EXISTS %1;").arg(name));
    }
    DatabaseQueries::deleteAccount(db, m_account);
  });
  qApp->feedReader()->feedsModel()->removeItem(m_account, false);
  m_account = nullptr;
}

bool TestGCalendarAccount::discover() {
  QSignalSpy completed(m_account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  m_account->requestSyncIn();
  return completed.wait(5000) && completed.first().first().toBool();
}

GCalendarFeed* TestGCalendarAccount::feed() const {
  return dynamic_cast<GCalendarFeed*>(m_account->getHashedSubTreeFeeds().value(QSL("first")));
}

void TestGCalendarAccount::fetch() {
  auto snapshot = m_account->obtainNewMessagesForUpdate(feed(), {}, {});
  for (auto& message : snapshot.messages) {
    message.m_accountId = m_account->accountId();
    message.m_feedId = feed()->id();
    message.sanitize(feed(), true);
  }
  m_account->updateMessages(snapshot.messages, feed(), false).throwIfFailed();
  snapshot.afterMessagesStored();
}

QHash<QString, Record> TestGCalendarAccount::records() const {
  return qApp->database()->worker()->read<QHash<QString, Record>>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT id, is_read, is_important, is_deleted, is_pdeleted, custom_data, date_retrieved "
                      "FROM Messages WHERE account_id = :account;"));
    query.bindValue(QSL(":account"), m_account->accountId());
    query.exec();
    QHash<QString, Record> result;
    while (query.next()) {
      const auto metadata = QJsonDocument::fromJson(query.value(5).toString().toUtf8())
                              .object()
                              .value(QSL("gcalendar"))
                              .toObject()
                              .value(QSL("event"))
                              .toObject();
      result.insert(metadata.value(QSL("id")).toString(),
                    {query.value(0).toInt(),
                     query.value(1).toBool(),
                     query.value(2).toBool(),
                     query.value(3).toBool(),
                     query.value(4).toBool(),
                     query.value(5).toString(),
                     query.value(6).toLongLong()});
    }
    return result;
  });
}

void TestGCalendarAccount::discoveryPreservesFeedSettingsAndFreshMetadata() {
  QVERIFY(discover());
  QVERIFY(feed() != nullptr);
  QCOMPARE(feed()->thread(), qApp->thread());
  const int original_id = feed()->id();
  feed()->setAutoUpdateInterval(83);
  feed()->setIsSwitchedOff(true);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteFeed(db, feed(), m_account->accountId(), NO_PARENT_CATEGORY);
    auto data = DatabaseQueries::loadFeedCustomData(db, feed());
    auto metadata = data.value(QSL("gcalendar")).toHash();
    metadata[QSL("local_override")] = QSL("preserve-me");
    data[QSL("gcalendar")] = metadata;
    data[QSL("unknown")] = 42;
    DatabaseQueries::storeCustomData(db, QSL("Feeds"), original_id, data);
  });
  m_server->m_calendars = {calendar(QSL("first"), QSL("Renamed")), calendar(QSL("second"), QSL("Second"))};
  QVERIFY(discover());
  QCOMPARE(feed()->id(), original_id);
  QCOMPARE(feed()->title(), QSL("Renamed"));
  QCOMPARE(feed()->autoUpdateInterval(), 83);
  QVERIFY(feed()->isSwitchedOff());
  QCOMPARE(feed()->calendarMetadata().value(QSL("local_override")).toString(), QSL("preserve-me"));
  QCOMPARE(feed()->customDatabaseData().value(QSL("unknown")).toInt(), 42);
  QCOMPARE(m_account->getSubTreeFeeds().size(), 2);
  m_server->m_calendars = {calendar(QSL("first"), QSL("Renamed"))};
  QVERIFY(discover());
  QCOMPARE(m_account->getSubTreeFeeds().size(), 1);
}

void TestGCalendarAccount::restartAccount(int refresh_calendar_list) {
  m_account->stop();
  const auto data = qApp->database()->worker()->write<QVariantHash>([&](const QSqlDatabase& db) {
    auto data = DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
    data[QSL("refresh_token")] = QSL("fixture-refresh");
    auto calendar = data.value(QSL("gcalendar")).toHash();
    if (refresh_calendar_list < 0) {
      calendar.remove(QSL("refresh_calendar_list_on_startup"));
    }
    else {
      calendar[QSL("refresh_calendar_list_on_startup")] = refresh_calendar_list != 0;
    }
    data[QSL("gcalendar")] = calendar;
    DatabaseQueries::storeCustomData(db, QSL("Accounts"), m_account->accountId(), data);
    return data;
  });
  m_account->setCustomDatabaseData(data);
  m_account->GCalendarServiceRoot::start(false);
}

void TestGCalendarAccount::startupCalendarDiscovery_data() {
  QTest::addColumn<bool>("cached");
  QTest::addColumn<int>("refresh_setting");
  QTest::addColumn<bool>("discover_on_startup");
  QTest::newRow("legacy-default-off") << true << -1 << false;
  QTest::newRow("explicit-off") << true << 0 << false;
  QTest::newRow("enabled") << true << 1 << true;
  QTest::newRow("empty-cache") << false << 0 << true;
}

void TestGCalendarAccount::startupCalendarDiscovery() {
  QFETCH(bool, cached);
  QFETCH(int, refresh_setting);
  QFETCH(bool, discover_on_startup);
  if (cached) {
    QVERIFY(discover());
  }
  m_server->m_requests.clear();
  QSignalSpy finished(m_account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  restartAccount(refresh_setting);
  QTRY_COMPARE(m_account->networkContext().m_bearer, QSL("Bearer renewed"));
  QCOMPARE(m_account->m_refreshes, 1);
  if (discover_on_startup) {
    QTRY_COMPARE(finished.size(), 1);
    QVERIFY(finished.first().first().toBool());
    QCOMPARE(m_server->requestCount(), 1);
  }
  else {
    QCOMPARE(finished.size(), 0);
    QCOMPARE(m_server->requestCount(), 0);
  }
  QCOMPARE(m_account->getSubTreeFeeds().size(), 1);
  QCOMPARE(m_server->m_eventCalls, 0);
  m_account->grantAuthorization();
  QCoreApplication::processEvents();
  QCOMPARE(finished.size(), int(discover_on_startup));
  QCOMPARE(m_server->requestCount(), int(discover_on_startup));
}

void TestGCalendarAccount::startupFetchWaitsForAuthorization_data() {
  QTest::addColumn<bool>("refresh_calendars");
  QTest::newRow("cached-list") << false;
  QTest::newRow("refresh-list") << true;
}

void TestGCalendarAccount::startupFetchWaitsForAuthorization() {
  QFETCH(bool, refresh_calendars);
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("startup-event"))};
  m_server->m_requests.clear();
  m_account->m_delayAuthorization = true;
  const auto update_type = feed()->autoUpdateType();
  const auto update_interval = feed()->autoUpdateInterval();
  QSignalSpy calendars(m_account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  restartAccount(refresh_calendars);
  QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_COMPARE(feed()->status(), Feed::Status::Fetching);
  QCOMPARE(finished.size(), 0);
  QCOMPARE(m_server->requestCount(), 0);
  m_account->grantAuthorization();
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(records().contains(QSL("startup-event")));
  QCOMPARE(m_account->m_refreshes, 1);
  if (refresh_calendars) {
    QTRY_COMPARE(calendars.size(), 1);
    QVERIFY(calendars.first().first().toBool());
  }
  QCOMPARE(m_server->requestCount(), 1 + int(refresh_calendars));
  QCOMPARE(feed()->autoUpdateType(), update_type);
  QCOMPARE(feed()->autoUpdateInterval(), update_interval);
}

void TestGCalendarAccount::rejectedTokenRefreshesOnce_data() {
  QTest::addColumn<bool>("failed_refresh");
  QTest::addColumn<bool>("reject_twice");
  QTest::addColumn<bool>("expired");
  QTest::newRow("recovered") << false << false << false;
  QTest::newRow("refresh-denied") << true << false << false;
  QTest::newRow("second-401") << false << true << false;
  QTest::newRow("expired") << false << false << true;
}

void TestGCalendarAccount::rejectedTokenRefreshesOnce() {
  QFETCH(bool, failed_refresh);
  QFETCH(bool, reject_twice);
  QFETCH(bool, expired);
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("recovered-event"))};
  m_server->m_requests.clear();
  m_server->m_rejectedBearer = "Bearer fixture";
  m_server->m_rejectAllBearers = reject_twice;
  m_account->findChild<OAuth2Service*>()->setRefreshToken(QSL("fixture-refresh"));
  if (expired) {
    auto* oauth = m_account->findChild<OAuth2Service*>();
    oauth->setTokensExpireIn(QDateTime::currentDateTimeUtc().addSecs(-1));
    emit oauth->tokensRetrieved(QSL("fixture"), {}, 0);
  }
  m_account->m_refreshError = failed_refresh ? QSL("Fixture refresh denied") : QString();
  QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_COMPARE(finished.size(), 1);
  QCOMPARE(m_account->m_refreshes, 1);
  QCOMPARE(m_server->requestCount(), (failed_refresh ? 1 : 2) - int(expired));
  QCOMPARE(records().contains(QSL("recovered-event")), !failed_refresh && !reject_twice);
  if (!failed_refresh) {
    QCOMPARE(m_server->m_requests.last().m_headers.value("Authorization"), QByteArray("Bearer renewed"));
  }
}

void TestGCalendarAccount::concurrentRequestsShareAuthorization() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("shared-event"))};
  m_server->m_requests.clear();
  m_server->m_rejectedBearer = "Bearer fixture";
  m_account->findChild<OAuth2Service*>()->setRefreshToken(QSL("fixture-refresh"));
  m_account->m_delayAuthorization = true;
  QSignalSpy calendars(m_account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  QSignalSpy articles(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  m_account->requestSyncIn();
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_VERIFY(m_server->requestCount() >= 1);
  QTRY_COMPARE(m_account->m_refreshes, 1);
  m_account->grantAuthorization();
  QTRY_COMPARE(articles.size(), 1);
  QTRY_COMPARE(calendars.size(), 1);
  QVERIFY(calendars.first().first().toBool());
  QCOMPARE(m_account->m_refreshes, 1);
  const int rejected =
    std::count_if(m_server->m_requests.constBegin(), m_server->m_requests.constEnd(), [](const auto& request) {
      return request.m_headers.value("Authorization") == QByteArray("Bearer fixture");
    });
  QVERIFY(rejected >= 1 && rejected <= 2);
  QCOMPARE(m_server->requestCount(), 2 + rejected);
  QVERIFY(records().contains(QSL("shared-event")));
}

void TestGCalendarAccount::calendarDiscoveryWaitsForUpdates() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("cached-event"))};
  fetch();
  m_server->m_requests.clear();
  m_server->m_calendars.clear();
  auto* update_lock = qApp->feedUpdateLock();
  QVERIFY(update_lock->tryLock());
  bool locked = true;
  const auto unlock = qScopeGuard([&]() {
    if (locked) {
      update_lock->unlock();
    }
  });
  QSignalSpy finished(m_account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  QSignalSpy status(m_account, &GCalendarServiceRoot::statusChanged);
  m_account->requestSyncIn();
  QTRY_COMPARE(status.size(), 2);
  QCOMPARE(finished.size(), 0);
  QCOMPARE(m_account->getSubTreeFeeds().size(), 1);
  QVERIFY(records().contains(QSL("cached-event")));
  locked = false;
  update_lock->unlock();
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(finished.first().first().toBool());
  QCOMPARE(m_account->getSubTreeFeeds().size(), 0);
  QVERIFY(records().isEmpty());
  QCOMPARE(m_server->requestCount(), 1);
}

void TestGCalendarAccount::stoppingAccountCancelsAuthorizationWait() {
  QVERIFY(discover());
  m_server->m_requests.clear();
  m_account->m_delayAuthorization = true;
  restartAccount();
  QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_COMPARE(feed()->status(), Feed::Status::Fetching);
  m_account->stop();
  QTRY_COMPARE(finished.size(), 1);
  QCOMPARE(m_server->requestCount(), 0);
  QVERIFY(records().isEmpty());
}

void TestGCalendarAccount::authorizationWaitTimesOut() {
  QVERIFY(discover());
  m_server->m_requests.clear();
  m_account->m_delayAuthorization = true;
  const auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout));
  qApp->settings()->setValue(GROUP(Feeds), Feeds::UpdateTimeout, 50);
  restartAccount();
  qApp->settings()->setValue(GROUP(Feeds), Feeds::UpdateTimeout, timeout);
  QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(feed()->statusString().contains(QSL("Timed out")));
  QCOMPARE(m_account->m_refreshes, 1);
  QCOMPARE(m_server->requestCount(), 0);
}

void TestGCalendarAccount::failedAuthorizationPreservesCache() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("cached-event"))};
  fetch();
  const auto before = records().value(QSL("cached-event")).m_data;
  m_server->m_events.clear();
  m_server->m_requests.clear();
  m_server->m_rejectedBearer = "Bearer fixture";
  m_account->findChild<OAuth2Service*>()->setRefreshToken(QSL("fixture-refresh"));
  m_account->m_delayAuthorization = true;
  QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
  qApp->feedReader()->updateFeeds({feed()});
  QTRY_COMPARE(m_account->m_refreshes, 1);
  emit m_account->findChild<OAuth2Service*>()->tokensRetrieveError({}, QSL("Fixture authorization failed"));
  QTRY_COMPARE(finished.size(), 1);
  QCOMPARE(m_account->m_refreshes, 1);
  QCOMPARE(m_server->requestCount(), 1);
  QCOMPARE(records().value(QSL("cached-event")).m_data, before);
}

void TestGCalendarAccount::calendarIconsPersistAndFollowColors() {
  m_server->m_calendars = {calendar(QSL("first"), QSL("First")),
                           calendar(QSL("second"), QSL("Second")),
                           calendar(QSL("missing"), QSL("Missing color")),
                           calendar(QSL("invalid"), QSL("Invalid color"))};
  m_server->m_calendars[0][QSL("backgroundColor")] = QSL("#0088aa");
  m_server->m_calendars[1][QSL("backgroundColor")] = QSL("#e67c73");
  m_server->m_calendars[3][QSL("backgroundColor")] = QSL("invalid-color");
  const auto load_icons = [&]() {
    return qApp->database()->worker()->read<QHash<QString, QByteArray>>([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.prepare(QSL("SELECT custom_id, icon FROM Feeds WHERE account_id = :account;"));
      query.bindValue(QSL(":account"), m_account->accountId());
      query.exec();
      QHash<QString, QByteArray> icons;
      while (query.next()) {
        icons.insert(query.value(0).toString(), query.value(1).toByteArray());
      }
      return icons;
    });
  };
  QVERIFY(discover());
  auto stored = load_icons();
  QCOMPARE(stored.size(), 4);
  const auto feeds = m_account->getHashedSubTreeFeeds();
  for (int i = 0; i < 2; ++i) {
    const auto resource = m_server->m_calendars.at(i);
    const auto id = resource.value(QSL("id")).toString();
    const QColor color(resource.value(QSL("backgroundColor")).toString());
    const auto displayed = feeds.value(id)->icon().pixmap(64, 64).toImage();
    QCOMPARE(displayed.pixelColor(32, 32), color);
    QCOMPARE(displayed.pixelColor(0, 0).alpha(), 0);
    const auto saved = IconFactory::fromByteArray(stored.value(id)).pixmap(64, 64).toImage();
    QCOMPARE(saved, displayed);
  }
  for (const auto& id : {QSL("missing"), QSL("invalid")}) {
    QCOMPARE(IconFactory::iconGuid(feeds.value(id)->icon()), IconFactory::iconGuid(m_account->icon()));
    QCOMPARE(IconFactory::iconGuid(IconFactory::fromByteArray(stored.value(id))),
             IconFactory::iconGuid(m_account->icon()));
  }
  m_server->m_calendars[0][QSL("backgroundColor")] = QSL("#33aa55");
  m_server->m_calendars[1].remove(QSL("backgroundColor"));
  QVERIFY(discover());
  stored = load_icons();
  QCOMPARE(feed()->icon().pixmap(64, 64).toImage().pixelColor(32, 32), QColor(QSL("#33aa55")));
  QCOMPARE(IconFactory::fromByteArray(stored.value(QSL("first"))).pixmap(64, 64).toImage().pixelColor(32, 32),
           QColor(QSL("#33aa55")));
  QCOMPARE(IconFactory::iconGuid(feeds.value(QSL("second"))->icon()), IconFactory::iconGuid(m_account->icon()));
  // Load the persisted icons through the real plugin without another remote discovery.
  QString error;
  auto* plugin = PluginFactory::loadPlugin(QString::fromUtf8(TEST_GCALENDAR_PLUGIN), &error);
  QVERIFY2(plugin != nullptr, qPrintable(error));
  const auto roots = plugin->initializeSubtree();
  QCOMPARE(roots.size(), 1);
  std::unique_ptr<ServiceRoot> cached(roots.first());
  cached->start(false);
  QCOMPARE(cached->getSubTreeFeeds().size(), stored.size());
  for (auto* calendar : cached->getSubTreeFeeds()) {
    QCOMPARE(IconFactory::iconGuid(calendar->icon()),
             IconFactory::iconGuid(IconFactory::fromByteArray(stored.value(calendar->customId()))));
  }
  cached->stop();
}

void TestGCalendarAccount::failedDiscoveryPreservesTree() {
  m_server->m_calendars[0][QSL("backgroundColor")] = QSL("#0088aa");
  QVERIFY(discover());
  const int original = feed()->id();
  const auto original_icon = IconFactory::toByteArray(feed()->icon());
  m_server->m_calendars.clear();
  m_server->m_failCalendarPage = true;
  QVERIFY(!discover());
  QVERIFY(feed() != nullptr);
  QCOMPARE(feed()->id(), original);
  QCOMPARE(m_account->getSubTreeFeeds().size(), 1);
  QCOMPARE(IconFactory::toByteArray(feed()->icon()), original_icon);
  m_server->m_failCalendarPage = false;
  m_server->m_calendars = {calendar(QSL("first"), QSL("Renamed"))};
  m_server->m_calendars[0][QSL("backgroundColor")] = QSL("#e67c73");
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_calendar_update BEFORE UPDATE ON Feeds "
                   "BEGIN SELECT RAISE(ABORT, 'fixture failure'); END;"));
  });
  QVERIFY(!discover());
  QCOMPARE(feed()->title(), QSL("First calendar"));
  QCOMPARE(IconFactory::toByteArray(feed()->icon()), original_icon);
  const auto stored_icon = qApp->database()->worker()->read<QByteArray>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT icon FROM Feeds WHERE id = :id;"));
    query.bindValue(QSL(":id"), original);
    query.exec();
    return query.next() ? query.value(0).toByteArray() : QByteArray();
  });
  QCOMPARE(stored_icon, original_icon);
}

void TestGCalendarAccount::settingsPreserveFreshTokensLedgerAndUnknownKeys() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::modifyAccountCustomData(db, m_account->accountId(), [](QVariantHash& data) {
      data[QSL("refresh_token")] = QSL("rotated-token");
      data[QSL("unknown")] = 42;
      auto calendar = QJsonObject::fromVariantHash(data.value(QSL("gcalendar")).toHash());
      calendar[QSL("ledger")] = QJsonObject{{QSL("version"), 1}, {QSL("entries"), QJsonObject()}};
      calendar[QSL("runtime_key")] = QSL("retained");
      data[QSL("gcalendar")] = calendar.toVariantHash();
    });
  });
  m_account->applySettings(QSL("Updated name"), QSL("fixture-client"), {}, 15, 180, {});
  const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  QCOMPARE(data.value(QSL("refresh_token")).toString(), QSL("rotated-token"));
  QCOMPARE(data.value(QSL("unknown")).toInt(), 42);
  QVERIFY(m_account->calendarSettings().contains(QSL("ledger")));
  QCOMPARE(m_account->calendarSettings().value(QSL("runtime_key")).toString(), QSL("retained"));
  QCOMPARE(m_account->title(), QSL("Updated name"));
  QCOMPARE(m_account->reminderSettings().m_checkIntervalMinutes, 30);
}

void TestGCalendarAccount::failedSettingsPreserveCorruptAccount_data() {
  QTest::addColumn<QString>("serialized");
  QTest::newRow("malformed-json") << QSL("{broken");
  QTest::newRow("future-version") << QSL("{\"gcalendar\":{\"version\":99}}");
  QTest::newRow("future-reminders") << QSL("{\"gcalendar\":{\"version\":1,\"reminders\":{\"version\":99}}}");
  QTest::newRow("future-ledger") << QSL("{\"gcalendar\":{\"version\":1,\"ledger\":{\"version\":99,\"entries\":{}}}}");
}

void TestGCalendarAccount::failedSettingsPreserveCorruptAccount() {
  QFETCH(QString, serialized);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("UPDATE Accounts SET custom_data = :data WHERE id = :id;"));
    query.bindValue(QSL(":data"), serialized);
    query.bindValue(QSL(":id"), m_account->accountId());
    query.exec();
  });
  QVERIFY_EXCEPTION_THROWN(m_account->applySettings(QSL("Rename"), QSL("fixture-client"), {}, 15, 180, {}),
                           ApplicationException);
  QCOMPARE(m_account->title(), QSL("Calendar fixture"));
  const auto data = qApp->database()->worker()->read<QString>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_data FROM Accounts WHERE id = :id;"));
    query.bindValue(QSL(":id"), m_account->accountId());
    query.exec();
    query.next();
    return query.value(0).toString();
  });
  QCOMPARE(data, serialized);
}

void TestGCalendarAccount::failedDialogSettingsRestoreRuntimeState() {
  const auto original = m_account->customDatabaseData();
  const auto proxy = m_account->networkProxy();
  AccountDialog dialog(m_account);
  dialog.findChild<QCheckBox*>(QSL("m_cbUnread"))->setChecked(!m_account->nodeShowUnread());
  dialog.findChild<NetworkProxyDetails*>()->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy,
                                                                   QSL("fixture-proxy"),
                                                                   8080));
  dialog.findChild<LineEditWithStatus*>(QSL("m_txtName"))->lineEdit()->clear();
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
  QCOMPARE(m_account->customDatabaseData(), original);
  QCOMPARE(m_account->networkProxy(), proxy);

  dialog.findChild<LineEditWithStatus*>(QSL("m_txtName"))->lineEdit()->setText(QSL("Rejected by database"));
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_calendar_update BEFORE UPDATE ON Accounts "
                   "BEGIN SELECT RAISE(ABORT, 'fixture failure'); END;"));
  });
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
  QCOMPARE(m_account->customDatabaseData(), original);
  QCOMPARE(m_account->networkProxy(), proxy);
}

void TestGCalendarAccount::removalInvalidatesPendingExpansions() {
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  const auto index = view->sourceModel()->indexForItem(m_account);
  QVERIFY(index.isValid());
  QVERIFY(QMetaObject::invokeMethod(view, "reloadItemExpandState", Qt::DirectConnection, Q_ARG(QModelIndex, index)));
  cleanup();
  QVERIFY(QMetaObject::invokeMethod(view, "reloadDelayedExpansions", Qt::DirectConnection));
  QCOMPARE(view->sourceModel()->rowCount(QModelIndex()), 0);
}

void TestGCalendarAccount::completeSnapshotsReconcileMissingAndCancelledEvents() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("kept")), calendarEvent(QSL("missing")), calendarEvent(QSL("cancelled"))};
  fetch();
  QCOMPARE(records().size(), 3);
  m_server->m_events = {calendarEvent(QSL("kept")),
                        QJsonObject{{QSL("id"), QSL("cancelled")}, {QSL("status"), QSL("cancelled")}}};
  fetch();
  const auto rows = records();
  QCOMPARE(rows.size(), 2);
  QVERIFY(!rows.contains(QSL("missing")));
  QVERIFY(rows.value(QSL("cancelled")).m_deleted);
  Message cancelled;
  cancelled.m_customData = rows.value(QSL("cancelled")).m_data;
  const auto parsed = CalendarEvent::fromMessage(cancelled);
  QVERIFY(parsed.isCancelled());
  QVERIFY(parsed.m_start.isValid());
  QVERIFY(parsed.m_end.isValid());
}

void TestGCalendarAccount::failedPageAndSavePreserveCachedEvents() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("existing"))};
  fetch();
  m_server->m_events.clear();
  m_server->m_failEventPage = true;
  QVERIFY_EXCEPTION_THROWN(fetch(), ApplicationException);
  QVERIFY(records().contains(QSL("existing")));
  m_server->m_failEventPage = false;
  m_server->m_events = {calendarEvent(QSL("new"), QSL("reject-write"))};
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_calendar_insert BEFORE INSERT ON Messages WHEN NEW.title = 'reject-write' "
                   "BEGIN SELECT RAISE(ABORT, 'fixture failure'); END;"));
  });
  QVERIFY_EXCEPTION_THROWN(fetch(), ApplicationException);
  QVERIFY(records().contains(QSL("existing")));
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER reject_calendar_insert;"));
  });
}

void TestGCalendarAccount::localFieldsAndTombstonesSurviveRefresh() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("local")), calendarEvent(QSL("tombstone"))};
  fetch();
  const auto before = records();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    const QString id = QString::number(before.value(QSL("local")).m_id);
    DatabaseQueries::markMessagesReadUnread(db, {id}, RootItem::ReadStatus::Read);
    DatabaseQueries::markMessageImportant(db, id.toInt(), RootItem::Importance::Important);
    DatabaseQueries::deleteOrRestoreMessagesToFromBin(db, {id}, true);
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(before.value(QSL("tombstone")).m_id)});
  });
  m_server->m_events[0][QSL("location")] = QSL("Changed metadata only");
  fetch();
  const auto after = records();
  QCOMPARE(after.size(), 2);
  const auto local = after.value(QSL("local"));
  QVERIFY(local.m_read);
  QVERIFY(local.m_important);
  QVERIFY(local.m_deleted);
  QCOMPARE(local.m_retrieved, before.value(QSL("local")).m_retrieved);
  QVERIFY(local.m_data.contains(QSL("Changed metadata only")));
  QVERIFY(after.value(QSL("tombstone")).m_permanent);
  QCOMPARE(after.value(QSL("tombstone")).m_id, before.value(QSL("tombstone")).m_id);
}

void TestGCalendarAccount::cancelledOccurrenceCanBeReinstated() {
  QVERIFY(discover());
  const auto original = calendarEvent(QSL("reinstated"));
  m_server->m_events = {original};
  fetch();
  const int id = records().value(QSL("reinstated")).m_id;
  m_server->m_events = {QJsonObject{{QSL("id"), QSL("reinstated")}, {QSL("status"), QSL("cancelled")}}};
  fetch();
  QVERIFY(records().value(QSL("reinstated")).m_deleted);
  m_server->m_events = {original};
  fetch();
  const auto restored = records().value(QSL("reinstated"));
  QCOMPARE(restored.m_id, id);
  QVERIFY(!restored.m_deleted);
}

void TestGCalendarAccount::oauthPkceFlowUsesActualRedirectAndVerifier() {
  QTcpServer endpoint;
  QVERIFY(endpoint.listen(QHostAddress::LocalHost));
  QByteArray received;
  connect(&endpoint, &QTcpServer::newConnection, &endpoint, [&]() {
    auto* socket = endpoint.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
      received += socket->readAll();
      const int header_end = received.indexOf("\r\n\r\n");
      if (header_end < 0) {
        return;
      }
      const auto headers = received.left(header_end).split('\n');
      int content_length = -1;
      for (const auto& header : headers) {
        if (header.toLower().startsWith("content-length:")) {
          content_length = header.mid(15).trimmed().toInt();
        }
      }
      if (content_length < 0 || received.size() < header_end + 4 + content_length) {
        return;
      }
      const auto body = QJsonDocument(QJsonObject{{QSL("access_token"), QSL("fixture-access")},
                                                  {QSL("refresh_token"), QSL("fixture-refresh")},
                                                  {QSL("expires_in"), 3600}})
                          .toJson(QJsonDocument::Compact);
      socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                    QByteArray::number(body.size()) + "\r\n\r\n" + body);
      socket->disconnectFromHost();
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  });
  OAuth2Service oauth(QSL("https://example.invalid/authorize"),
                      QSL("http://127.0.0.1:%1/token").arg(endpoint.serverPort()),
                      QSL("fixture-client"),
                      {},
                      QSL("fixture-scope"));
  oauth.setPkceEnabled(true);
  oauth.setNetworkProxy(QNetworkProxy::NoProxy);
  oauth.setRedirectUrl(QSL("http://127.0.0.1:0"), true);
  const QUrlQuery first(oauth.createAuthorizationUrl());
  const QUrlQuery second(oauth.createAuthorizationUrl());
  QVERIFY(first.queryItemValue(QSL("state")) != second.queryItemValue(QSL("state")));
  QVERIFY(QUrl(second.queryItemValue(QSL("redirect_uri"))).port() > 0);
  QCOMPARE(second.queryItemValue(QSL("code_challenge_method")), QSL("S256"));
  QSignalSpy tokens(&oauth, &OAuth2Service::tokensRetrieved);
  oauth.retrieveAccessToken(QSL("fixture-code"));
  QVERIFY(tokens.wait(5000));
  const QUrlQuery form(QString::fromUtf8(received.mid(received.indexOf("\r\n\r\n") + 4)));
  QCOMPARE(form.queryItemValue(QSL("code")), QSL("fixture-code"));
  QCOMPARE(form.queryItemValue(QSL("redirect_uri")), second.queryItemValue(QSL("redirect_uri")));
  QCOMPARE(OAuth2Service::pkceChallenge(form.queryItemValue(QSL("code_verifier"))),
           second.queryItemValue(QSL("code_challenge")));
}

bool TestGCalendarAccount::scan() {
  QSignalSpy completed(m_account, &GCalendarServiceRoot::remindersChecked);
  m_account->checkReminders();
  return completed.wait(5000) && completed.first().first().toBool();
}

QDateTime TestGCalendarAccount::prepareReminder() {
  const auto start = m_account->m_now.addSecs(1200);
  auto resource = calendarEvent(QSL("reminder"));
  resource[QSL("start")] = QJsonObject{{QSL("dateTime"), start.toString(Qt::ISODateWithMs)}};
  resource[QSL("end")] = QJsonObject{{QSL("dateTime"), start.addSecs(3600).toString(Qt::ISODateWithMs)}};
  m_server->m_events = {resource};
  fetch();
  auto settings = m_account->reminderSettings();
  settings.m_enabled = true;
  settings.m_rules = {{QSL("at-start"), 0, 0}, {QSL("before"), 10, 0}};
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 365, settings);
  return start;
}

void TestGCalendarAccount::remindersUseDatabaseWithoutFetchingAndGroupRules() {
  QVERIFY(discover());
  const auto start = prepareReminder();
  QVERIFY(scan());
  QVERIFY(m_account->m_delivered.isEmpty());
  m_account->m_now = start.addSecs(60);
  const int network_calls = m_server->m_eventCalls;
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  QCOMPARE(m_server->m_eventCalls, network_calls);
  QCOMPARE(CalendarReminders::entries(m_account->calendarSettings().value(QSL("ledger")).toObject()).size(), 2);
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  const auto timers = m_account->findChildren<QTimer*>();
  QVERIFY(std::any_of(timers.constBegin(), timers.constEnd(), [](QTimer* timer) {
    return timer->isActive() && timer->interval() == 30 * 60000;
  }));
}

void TestGCalendarAccount::reminderCheckpointFailureDoesNotRepeatDelivery() {
  QVERIFY(discover());
  const auto start = prepareReminder();
  QVERIFY(scan());
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_reminder_checkpoint BEFORE UPDATE OF custom_data ON Accounts "
                   "BEGIN SELECT RAISE(ABORT, 'fixture checkpoint failure'); END;"));
  });
  m_account->m_now = start.addSecs(60);
  QVERIFY(!scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  QVERIFY(!scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER reject_reminder_checkpoint;"));
  });
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  QCOMPARE(CalendarReminders::entries(m_account->calendarSettings().value(QSL("ledger")).toObject()).size(), 2);
}

void TestGCalendarAccount::remindersSurviveAccountRestart() {
  QVERIFY(discover());
  const auto start = prepareReminder();
  QVERIFY(scan());
  m_account->m_now = start;
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  const int id = m_account->accountId();
  const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, id);
  });
  m_account->stop();
  qApp->feedReader()->feedsModel()->removeItem(m_account, false);
  m_account = new Account(m_server);
  m_account->setAccountId(id);
  m_account->setCustomDatabaseData(data);
  m_account->m_now = start.addSecs(60);
  qApp->feedReader()->feedsModel()->addServiceAccount(m_account, false);
  QVERIFY(scan());
  QVERIFY(m_account->m_delivered.isEmpty());
  QVERIFY(feed() != nullptr);
  QCOMPARE(feed()->thread(), qApp->thread());
}

void TestGCalendarAccount::calendarOverrideAndRetirementUseCustomData() {
  QVERIFY(discover());
  const auto start = prepareReminder();
  QVERIFY(scan());
  m_account->setCalendarReminderOverride(feed(), QJsonObject{{QSL("mode"), QSL("disabled")}});
  m_server->m_calendars = {calendar(QSL("first"), QSL("Renamed with override"))};
  QVERIFY(discover());
  QCOMPARE(feed()->calendarMetadata().value(QSL("reminder_override")).toObject().value(QSL("mode")).toString(),
           QSL("disabled"));
  m_account->setCalendarReminderOverride(feed(),
                                         QJsonObject{{QSL("mode"), QSL("local")},
                                                     {QSL("rules"),
                                                      QJsonArray{QJsonObject{{QSL("id"), QSL("local-zero")},
                                                                             {QSL("minutes_before"), 0}}}}});
  m_account->m_now = start;
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  m_server->m_events.clear();
  fetch();
  QVERIFY(records().isEmpty());
  const auto saved = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  const auto ledger =
    QJsonObject::fromVariantHash(saved.value(QSL("gcalendar")).toHash()).value(QSL("ledger")).toObject();
  QVERIFY(CalendarReminders::entries(ledger).isEmpty());
}

void TestGCalendarAccount::cacheWindowCoversCalendarOverrides() {
  QVERIFY(discover());
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 1, {});
  const auto before = feed()->customDatabaseData();
  const QJsonObject local{
    {QSL("mode"), QSL("local")},
    {QSL("rules"), QJsonArray{QJsonObject{{QSL("id"), QSL("early")}, {QSL("minutes_before"), 2880}}}}};
  QVERIFY_EXCEPTION_THROWN(m_account->setCalendarReminderOverride(feed(), local), ApplicationException);
  QCOMPARE(feed()->customDatabaseData(), before);
  QVERIFY_EXCEPTION_THROWN(m_account->setCalendarReminderOverride(feed(), {{QSL("mode"), QSL("google")}}),
                           ApplicationException);
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 30, {});
  m_account->setCalendarReminderOverride(feed(), local);
  QVERIFY_EXCEPTION_THROWN(m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 1, {}),
                           ApplicationException);
  QCOMPARE(m_account->calendarSettings().value(QSL("future_days")).toInt(), 30);
}

QList<QVariant> TestGCalendarAccount::operation(const std::function<quint64()>& start) {
  QSignalSpy completed(m_account, &GCalendarServiceRoot::eventOperationFinished);
  const auto id = start();
  if (!completed.wait(5000) || completed.first().first().toULongLong() != id) {
    return {};
  }
  return completed.first();
}

void TestGCalendarAccount::createsEditsAndPreservesLocalAndRemoteFields() {
  QVERIFY(discover());
  auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc());
  draft.m_fields.m_title = QSL("Created in RSS Guard");
  auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(result.at(4).toBool());
  const int id = result.at(5).toInt();
  QVERIFY(id > 0);
  QCOMPARE(records().size(), 1);
  auto remote = result.at(1).toJsonObject();
  remote[QSL("attendees")] = QJsonArray{QJsonObject{{QSL("email"), QSL("guest@example.invalid")}}};
  remote[QSL("conferenceData")] = QJsonObject{{QSL("untouched"), true}};
  m_server->store(remote);
  fetch();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::markMessagesReadUnread(db, {QString::number(id)}, RootItem::ReadStatus::Read);
    DatabaseQueries::markMessageImportant(db, id, RootItem::Importance::Important);
  });
  const auto retrieved = records().value(draft.eventId()).m_retrieved;
  auto edit = CalendarEventDraft::fromEvent(CalendarEvent::fromMessage(m_account->cachedMessage(id)));
  edit.m_fields.m_title = QSL("Edited in RSS Guard");
  result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), edit.baseline(), edit.changes(), false, true, id);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(result.at(4).toBool());
  const auto saved = result.at(1).toJsonObject();
  QCOMPARE(saved.value(QSL("attendees")), remote.value(QSL("attendees")));
  QCOMPARE(saved.value(QSL("conferenceData")), remote.value(QSL("conferenceData")));
  const auto row = records().value(draft.eventId());
  QVERIFY(row.m_read);
  QVERIFY(row.m_important);
  QCOMPARE(row.m_retrieved, retrieved);
  QCOMPARE(row.m_id, id);
  const auto request = m_server->m_requests.last();
  QCOMPARE(request.m_method, QByteArray("PATCH"));
  QCOMPARE(QJsonDocument::fromJson(request.m_body).object().size(), 1);
  QCOMPARE(QUrlQuery(request.m_url).queryItemValue(QSL("sendUpdates")), QSL("all"));
  QCOMPARE(request.m_headers.value("If-Match"), remote.value(QSL("etag")).toString().toUtf8());
}

void TestGCalendarAccount::ambiguousCreationReusesDraftId_data() {
  QTest::addColumn<bool>("edit_retry");
  QTest::newRow("unchanged-retry") << false;
  QTest::newRow("edited-draft") << true;
}

void TestGCalendarAccount::ambiguousCreationReusesDraftId() {
  QFETCH(bool, edit_retry);
  QVERIFY(discover());
  FormCalendarEvent dialog(m_account);
  dialog.show();
  auto* title = dialog.findChild<QLineEdit*>(QSL("m_txtTitle"));
  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  QVERIFY(title != nullptr && buttons != nullptr);
  title->setText(QSL("Only one event"));
  m_server->m_ambiguousInserts = 1;
  QSignalSpy finished(m_account, &GCalendarServiceRoot::eventOperationFinished);
  QSignalSpy accepted(&dialog, &QDialog::accepted);
  buttons->button(QDialogButtonBox::Save)->click();
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(!finished.first().at(4).toBool());
  QCOMPARE(accepted.size(), 0);
  QCOMPARE(title->text(), QSL("Only one event"));
  QVERIFY(buttons->isEnabled());
  if (edit_retry) {
    title->setText(QSL("Edited after the lost response"));
  }
  buttons->button(QDialogButtonBox::Save)->click();
  if (edit_retry) {
    QTRY_COMPARE(finished.size(), 2);
    QCOMPARE(accepted.size(), 0);
    QCOMPARE(title->text(), QSL("Edited after the lost response"));
    QVERIFY(records().values().first().m_data.contains(QSL("Only one event")));
    buttons->button(QDialogButtonBox::Save)->click();
  }
  QTRY_COMPARE(accepted.size(), 1);
  QStringList ids;
  for (const auto& request : m_server->m_requests) {
    if (request.m_method == "POST") {
      ids.append(QJsonDocument::fromJson(request.m_body).object().value(QSL("id")).toString());
    }
  }
  QCOMPARE(ids.size(), 2);
  QCOMPARE(ids.first(), ids.last());
  QCOMPARE(records().size(), 1);
  if (edit_retry) {
    const auto patch = m_server->m_requests.last();
    QCOMPARE(patch.m_method, QByteArray("PATCH"));
    QCOMPARE(QJsonDocument::fromJson(patch.m_body).object(), QJsonObject({{QSL("summary"), title->text()}}));
    QVERIFY(records().values().first().m_data.contains(title->text()));
  }
}

void TestGCalendarAccount::editorConflictKeepsDraftAndReappliesSparseChanges() {
  QVERIFY(discover());
  auto resource = calendarEvent(QSL("conflict"));
  resource[QSL("location")] = QSL("Original room");
  m_server->store(resource);
  fetch();
  const auto original = m_account->cachedMessage(records().value(QSL("conflict")).m_id);
  FormCalendarEvent dialog(m_account, &original);
  dialog.show();
  auto* title = dialog.findChild<QLineEdit*>(QSL("m_txtTitle"));
  auto* location = dialog.findChild<QLineEdit*>(QSL("m_txtLocation"));
  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  auto* reapply = dialog.findChild<QPushButton*>(QSL("m_btnReapply"));
  QVERIFY(title != nullptr && location != nullptr && buttons != nullptr && reapply != nullptr);
  saveSnapshot(&dialog, QSL("event-editor"));
  title->setText(QSL("Local title"));
  resource[QSL("summary")] = QSL("Remote title");
  resource[QSL("location")] = QSL("Remote room");
  resource[QSL("etag")] = QSL("remote-etag");
  m_server->store(resource);
  QSignalSpy accepted(&dialog, &QDialog::accepted);
  buttons->button(QDialogButtonBox::Save)->click();
  QTRY_VERIFY(reapply->isVisible());
  saveSnapshot(&dialog, QSL("event-conflict"));
  QCOMPARE(title->text(), QSL("Local title"));
  QCOMPARE(accepted.size(), 0);
  reapply->click();
  QTRY_COMPARE(location->text(), QSL("Remote room"));
  QCOMPARE(title->text(), QSL("Local title"));
  QVERIFY(buttons->isEnabled());
  buttons->button(QDialogButtonBox::Save)->click();
  QTRY_COMPARE(accepted.size(), 1);
  const auto request = m_server->m_requests.last();
  QCOMPARE(request.m_method, QByteArray("PATCH"));
  QCOMPARE(request.m_headers.value("If-Match"), QByteArray("remote-etag"));
  QCOMPARE(QJsonDocument::fromJson(request.m_body).object(), QJsonObject({{QSL("summary"), QSL("Local title")}}));
  QCOMPARE(m_account->cachedMessage(original.m_id).m_title, QSL("Local title"));
}

void TestGCalendarAccount::seriesOperationsStoreOccurrencesAndPreserveRecurrence() {
  QVERIFY(discover());
  auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc());
  draft.m_fields.m_title = QSL("Recurring discussion");
  draft.m_fields.m_recurrence = QJsonArray{QSL("RRULE:FREQ=WEEKLY;BYDAY=MO,WE;COUNT=7")};
  auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY2(result.at(4).toBool() && result.at(3).toString().isEmpty(), qPrintable(result.at(3).toString()));
  QCOMPARE(records().size(), 2);
  const auto message = m_account->cachedMessage(result.at(5).toInt());
  const auto occurrence = CalendarEvent::fromMessage(message);
  QCOMPARE(occurrence.m_resource.value(QSL("recurringEventId")).toString(), draft.eventId());
  QVERIFY(occurrence.m_resource.value(QSL("recurrence")).toArray().isEmpty());
  FormCalendarEvent dialog(m_account, &message);
  dialog.show();
  auto* target = dialog.findChild<QComboBox*>(QSL("m_cmbTarget"));
  auto* recurrence = dialog.findChild<QComboBox*>(QSL("m_cmbRecurrence"));
  auto* title = dialog.findChild<QLineEdit*>(QSL("m_txtTitle"));
  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  QVERIFY(target != nullptr && recurrence != nullptr && title != nullptr && buttons != nullptr);
  QVERIFY(!recurrence->isEnabled());
  target->setCurrentIndex(1);
  QTRY_VERIFY(recurrence->isEnabled());
  QCOMPARE(recurrence->currentIndex(), 5);
  title->setText(QSL("Whole series edited"));
  QSignalSpy accepted(&dialog, &QDialog::accepted);
  buttons->button(QDialogButtonBox::Save)->click();
  QTRY_COMPARE(accepted.size(), 1);
  QCOMPARE(m_server->m_resources.value(draft.eventId()).value(QSL("recurrence")).toArray(),
           draft.m_fields.m_recurrence);
  QCOMPARE(records().size(), 2);
  for (const auto& row : records()) {
    QVERIFY(row.m_data.contains(QSL("Whole series edited")));
  }
  for (const auto& request : m_server->m_requests) {
    if (request.m_method == "PATCH") {
      QVERIFY(!QJsonDocument::fromJson(request.m_body).object().contains(QSL("recurrence")));
    }
  }
}

void TestGCalendarAccount::savedEventsOutsideWindowRemainVisible() {
  QVERIFY(discover());
  m_server->m_filterWindow = true;
  auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc().addDays(1000));
  auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(result.at(4).toBool());
  QCOMPARE(records().size(), 1);
  fetch();
  QCOMPARE(records().size(), 1);
  const int id = result.at(5).toInt();
  auto envelope = QJsonDocument::fromJson(records().value(draft.eventId()).m_data.toUtf8()).object();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    auto metadata = envelope.value(QSL("gcalendar")).toObject();
    metadata[QSL("keep_until")] = QDateTime::currentDateTimeUtc().addSecs(-1).toString(Qt::ISODateWithMs);
    envelope[QSL("gcalendar")] = metadata;
    SqlQuery query(db);
    query.prepare(QSL("UPDATE Messages SET custom_data = :data WHERE id = :id;"));
    query.bindValue(QSL(":data"), QString::fromUtf8(QJsonDocument(envelope).toJson()));
    query.bindValue(QSL(":id"), id);
    query.exec();
  });
  fetch();
  QVERIFY(records().isEmpty());
}

void TestGCalendarAccount::failedCacheWriteReportsRemoteSuccess() {
  QVERIFY(discover());
  m_server->store(calendarEvent(QSL("cache-failure")));
  fetch();
  const int id = records().value(QSL("cache-failure")).m_id;
  const auto event = CalendarEvent::fromMessage(m_account->cachedMessage(id));
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_calendar_update BEFORE UPDATE OF title ON Messages WHEN NEW.title = "
                   "'reject-write' "
                   "BEGIN SELECT RAISE(ABORT, 'fixture cache failure'); END;"));
  });
  const auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"),
                                      event.m_resource,
                                      {{QSL("summary"), QSL("reject-write")}},
                                      false,
                                      false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(result.at(4).toBool());
  QVERIFY(!result.at(3).toString().isEmpty());
  QCOMPARE(m_server->m_resources.value(QSL("cache-failure")).value(QSL("summary")).toString(), QSL("reject-write"));
  QCOMPARE(m_account->cachedMessage(id).m_title, QSL("Meeting"));
}

void TestGCalendarAccount::stopCancelsOwnedWriteWithoutLateCacheUpdates() {
  QVERIFY(discover());
  const auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc());
  m_server->m_holdWrites.store(true);
  m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  QTRY_VERIFY(m_server->m_writeEntered.available() > 0);
  QVERIFY(m_server->m_writeEntered.tryAcquire());
  QVERIFY(!m_account->canBeDeleted());
  m_account->stop();
  QVERIFY(records().isEmpty());
  QCoreApplication::processEvents();
  QVERIFY(records().isEmpty());
}

void TestGCalendarAccount::stopCancelsNativeReadWithoutRetiringCache() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("retained"))};
  fetch();
  const int id = records().value(QSL("retained")).m_id;
  m_server->m_holdReads.store(true);
  auto future = QtConcurrent::run([&]() {
    try {
      fetch();
      return true;
    }
    catch (const ApplicationException&) {
      return false;
    }
  });
  QTRY_VERIFY(m_server->m_readEntered.available() > 0);
  m_account->stop();
  future.waitForFinished();
  QVERIFY(!future.result());
  QCOMPARE(records().size(), 1);
  QCOMPARE(records().value(QSL("retained")).m_id, id);
}

void TestGCalendarAccount::expiredTombstonesRetireConservatively() {
  QVERIFY(discover());
  m_server->m_filterWindow = true;
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 1500, 365, {});
  auto expired = calendarEvent(QSL("expired"));
  const auto start = QDateTime::currentDateTimeUtc().addDays(-1000);
  expired[QSL("start")] = QJsonObject{{QSL("dateTime"), start.toString(Qt::ISODate)}};
  expired[QSL("end")] = QJsonObject{{QSL("dateTime"), start.addSecs(3600).toString(Qt::ISODate)}};
  m_server->m_events = {expired, calendarEvent(QSL("retained")), calendarEvent(QSL("uncertain"))};
  fetch();
  const auto before = records();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    QStringList ids;
    for (const auto& record : before) {
      ids.append(QString::number(record.m_id));
    }
    DatabaseQueries::permanentlyDeleteMessages(db, ids);
    DatabaseQueries::storeCustomData(db,
                                     QSL("Messages"),
                                     before.value(QSL("uncertain")).m_id,
                                     {{QSL("gcalendar"), QVariantHash{{QSL("version"), 99}}}});
  });
  m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 365, {});
  fetch();
  const auto after = records();
  QCOMPARE(after.size(), 2);
  QVERIFY(!after.contains(QSL("expired")));
  QCOMPARE(after.value(QSL("retained")).m_id, before.value(QSL("retained")).m_id);
  QVERIFY(after.value(QSL("retained")).m_permanent);
  bool kept_uncertain = false;
  for (const auto& record : after) {
    kept_uncertain = kept_uncertain || (record.m_id == before.value(QSL("uncertain")).m_id && record.m_permanent);
  }
  QVERIFY(kept_uncertain);
}

void TestGCalendarAccount::queuedEditFollowsSnapshotStorage() {
  QVERIFY(discover());
  m_server->store(calendarEvent(QSL("serialization")));
  fetch();
  const int id = records().value(QSL("serialization")).m_id;
  const auto baseline = CalendarEvent::fromMessage(m_account->cachedMessage(id)).m_resource;
  QSignalSpy completed(m_account, &GCalendarServiceRoot::eventOperationFinished);
  {
    auto snapshot = m_account->obtainNewMessagesForUpdate(feed(), {}, {});
    m_account->saveRemoteEvent(QSL("first"), baseline, {{QSL("summary"), QSL("After snapshot")}}, false, false);
    for (auto& message : snapshot.messages) {
      message.m_accountId = m_account->accountId();
      message.m_feedId = feed()->id();
      message.sanitize(feed(), true);
    }
    m_account->updateMessages(snapshot.messages, feed(), false).throwIfFailed();
    snapshot.afterMessagesStored();
  }
  QVERIFY(completed.wait(5000));
  QVERIFY(completed.first().at(4).toBool());
  QCOMPARE(m_account->cachedMessage(id).m_title, QSL("After snapshot"));
}

void TestGCalendarAccount::previewUsesCachedDataAndRespectsPermissions() {
  QVERIFY(discover());
  auto resource = calendarEvent(QSL("preview"));
  resource[QSL("attendees")] = QJsonArray{QJsonObject{{QSL("displayName"), QSL("Guest <unsafe>")}}};
  resource[QSL("conferenceData")] =
    QJsonObject{{QSL("entryPoints"),
                 QJsonArray{QJsonObject{{QSL("entryPointType"), QSL("video")},
                                        {QSL("uri"), QSL("https://meet.example.invalid/call")}}}}};
  m_server->store(resource);
  fetch();
  auto message = m_account->cachedMessage(records().value(QSL("preview")).m_id);
  CalendarEventPreviewer preview(m_account);
  const int requests = m_server->m_requests.size();
  preview.loadMessage(message, m_account, feed());
  auto* details = preview.findChild<QTextBrowser*>(QSL("m_details"));
  auto* edit = preview.findChild<QPushButton*>(QSL("m_btnEdit"));
  auto* join = preview.findChild<QPushButton*>(QSL("m_btnJoin"));
  QVERIFY(details != nullptr && edit != nullptr && join != nullptr);
  QVERIFY(details->toPlainText().contains(QSL("Guest <unsafe>")));
  QVERIFY(edit->isEnabled());
  QVERIFY(join->isEnabled());
  preview.resize(720, 420);
  preview.show();
  saveSnapshot(&preview, QSL("event-preview"));
  QCOMPARE(m_server->m_requests.size(), requests);
  message.m_isDeleted = true;
  preview.loadMessage(message, m_account, feed());
  QVERIFY(!edit->isEnabled());
}

void TestGCalendarAccount::remindersContinueDuringRemoteReads() {
  QVERIFY(discover());
  const auto start = prepareReminder();
  QVERIFY(scan());
  m_server->m_holdReads.store(true);
  QSignalSpy completed(m_account, &GCalendarServiceRoot::eventOperationFinished);
  m_account->loadRemoteEvent(QSL("first"), QSL("reminder"));
  QTRY_VERIFY(m_server->m_readEntered.available() > 0);
  m_account->m_now = start.addSecs(60);
  QVERIFY(scan());
  QCOMPARE(m_account->m_delivered.size(), 1);
  m_server->m_holdReads.store(false);
  QTRY_COMPARE(completed.size(), 1);
}

void TestGCalendarAccount::localDeleteRestoreUsesCommonCountRefresh() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("local-counts"))};
  fetch();
  m_account->updateCounts();
  QCOMPARE(feed()->countOfAllMessages(), 1);
  const auto message = m_account->cachedMessage(records().value(QSL("local-counts")).m_id);
  const int requests = m_server->m_requests.size();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteOrRestoreMessagesToFromBin(db, {QString::number(message.m_id)}, true);
  });
  m_account->onAfterMessagesDelete(feed(), {message});
  QTRY_COMPARE(feed()->countOfAllMessages(), 0);
  QCOMPARE(m_account->recycleBin()->countOfAllMessages(), 1);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteOrRestoreMessagesToFromBin(db, {QString::number(message.m_id)}, false);
  });
  m_account->onAfterMessagesRestoredFromBin(feed(), {message});
  QTRY_COMPARE(feed()->countOfAllMessages(), 1);
  QCOMPARE(m_account->recycleBin()->countOfAllMessages(), 0);
  QCOMPARE(m_server->m_requests.size(), requests);
}

void TestGCalendarAccount::seriesOutsideWindowPinsARealOccurrence() {
  QVERIFY(discover());
  m_server->m_filterWindow = true;
  auto draft = CalendarEventDraft::create("Europe/Prague", QDateTime::currentDateTimeUtc().addDays(1000));
  draft.m_fields.m_recurrence = QJsonArray{QSL("RRULE:FREQ=DAILY;COUNT=2")};
  const auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY2(result.at(4).toBool() && result.at(3).toString().isEmpty(), qPrintable(result.at(3).toString()));
  QCOMPARE(records().size(), 1);
  const auto message = m_account->cachedMessage(result.at(5).toInt());
  const auto occurrence = CalendarEvent::fromMessage(message);
  QCOMPARE(occurrence.m_resource.value(QSL("recurringEventId")).toString(), draft.eventId());
  QVERIFY(occurrence.m_resource.value(QSL("recurrence")).toArray().isEmpty());
  QVERIFY(std::any_of(m_server->m_requests.constBegin(), m_server->m_requests.constEnd(), [](const auto& request) {
    return request.m_url.path().endsWith(QSL("/instances"));
  }));
  fetch();
  QCOMPARE(records().size(), 1);
}

void TestGCalendarAccount::writesRecheckCurrentCalendarPermissions() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("permissions"))};
  fetch();
  const auto baseline =
    CalendarEvent::fromMessage(m_account->cachedMessage(records().value(QSL("permissions")).m_id)).m_resource;
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    auto data = DatabaseQueries::loadFeedCustomData(db, feed());
    GCalendarFeed calendar;
    calendar.setCustomDatabaseData(data);
    auto resource = calendar.calendarResource();
    resource[QSL("accessRole")] = QSL("reader");
    calendar.setCalendarResource(resource);
    DatabaseQueries::storeCustomData(db, QSL("Feeds"), feed()->id(), calendar.customDatabaseData());
  });
  const int requests = m_server->m_requests.size();
  const auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"),
                                      baseline,
                                      {{QSL("summary"), QSL("Should not be sent")}},
                                      false,
                                      false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(!result.at(4).toBool());
  QVERIFY(!result.at(3).toString().isEmpty());
  QCOMPARE(m_server->m_requests.size(), requests);
}

void TestGCalendarAccount::unsupportedDatesPreserveCacheAndDoNotSendWrites() {
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("current"))};
  fetch();
  const auto id = records().value(QSL("current")).m_id;
  auto ancient = calendarEvent(QSL("ancient"));
  ancient[QSL("start")] = QJsonObject{{QSL("dateTime"), QSL("1960-01-01T12:00:00Z")}};
  ancient[QSL("end")] = QJsonObject{{QSL("dateTime"), QSL("1960-01-01T13:00:00Z")}};
  m_server->m_events.append(ancient);
  QVERIFY_EXCEPTION_THROWN(fetch(), ApplicationException);
  QCOMPARE(records().size(), 1);
  QCOMPARE(records().value(QSL("current")).m_id, id);
  const int requests = m_server->m_requests.size();
  const auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"),
                                      {},
                                      {{QSL("id"), QSL("ancient")},
                                       {QSL("summary"), QSL("Old event")},
                                       {QSL("start"), ancient.value(QSL("start"))},
                                       {QSL("end"), ancient.value(QSL("end"))}},
                                      true,
                                      false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY(!result.at(4).toBool());
  QVERIFY(result.at(3).toString().contains(QSL("1970")));
  QCOMPARE(m_server->m_requests.size(), requests);
}

void TestGCalendarAccount::oldSeriesCachesModernOccurrences() {
  QVERIFY(discover());
  m_server->m_filterWindow = true;
  auto draft =
    CalendarEventDraft::create("Europe/Prague", QDateTime::fromString(QSL("1960-01-01T12:00:00Z"), Qt::ISODate));
  draft.m_fields.m_recurrence = QJsonArray{QSL("RRULE:FREQ=DAILY")};
  m_server->m_firstInstanceOffset = int(draft.m_fields.m_startDate.daysTo(QDateTime::currentDateTimeUtc().date()));
  const auto result = operation([&]() {
    return m_account->saveRemoteEvent(QSL("first"), {}, draft.changes(), true, false);
  });
  QVERIFY(!result.isEmpty());
  QVERIFY2(result.at(4).toBool() && result.at(3).toString().isEmpty(), qPrintable(result.at(3).toString()));
  QVERIFY(result.at(5).toInt() > 0);
  QCOMPARE(records().size(), 2);
  for (const auto& row : records()) {
    const auto event = CalendarEvent::fromMessage(m_account->cachedMessage(row.m_id));
    QVERIFY(event.m_start.toMSecsSinceEpoch() >= 0);
    QCOMPARE(event.m_resource.value(QSL("recurringEventId")).toString(), draft.eventId());
  }
}

void TestGCalendarAccount::unsupportedMetadataPreservesCache_data() {
  QTest::addColumn<QString>("table");
  QTest::newRow("calendar") << QSL("Feeds");
  QTest::newRow("event") << QSL("Messages");
}

void TestGCalendarAccount::unsupportedMetadataPreservesCache() {
  QFETCH(QString, table);
  QVERIFY(discover());
  m_server->m_events = {calendarEvent(QSL("retained"))};
  fetch();
  const int id = table == QLatin1String("Feeds") ? feed()->id() : records().value(QSL("retained")).m_id;
  const auto unsupported = QSL("{\"gcalendar\":{\"version\":99},\"unknown\":\"retain\"}");
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("UPDATE %1 SET custom_data = :data WHERE id = :id;").arg(table));
    query.bindValue(QSL(":data"), unsupported);
    query.bindValue(QSL(":id"), id);
    query.exec();
  });
  m_server->m_events.clear();
  QVERIFY_EXCEPTION_THROWN(fetch(), ApplicationException);
  const auto stored = qApp->database()->worker()->read<QString>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_data FROM %1 WHERE id = :id;").arg(table));
    query.bindValue(QSL(":id"), id);
    query.exec();
    return query.next() ? query.value(0).toString() : QString();
  });
  QCOMPARE(stored, unsupported);
  QCOMPARE(records().size(), 1);
}

void TestGCalendarAccount::bundledPluginsLoadIndependentCachedRoots() {
  QVERIFY(discover());
  QString error;
  auto* plugin = PluginFactory::loadPlugin(QString::fromUtf8(TEST_GCALENDAR_PLUGIN), &error);
  QVERIFY2(plugin != nullptr, qPrintable(error));
  QCOMPARE(plugin->code(), m_account->code());
  const auto roots = plugin->initializeSubtree();
  QCOMPARE(roots.size(), 1);
  std::unique_ptr<ServiceRoot> cached(roots.first());
  QCOMPARE(cached->thread(), qApp->thread());
  cached->start(false);
  QCOMPARE(cached->getSubTreeFeeds().size(), 1);
  QCOMPARE(cached->getSubTreeFeeds().first()->id(), feed()->id());
  QCOMPARE(cached->articleColumns().size(), 5);
  QVERIFY(!cached->messageStoragePolicy().m_useArticleCleanup);
  cached->stop();
#if defined(TEST_GMAIL_PLUGIN)
  auto* gmail = PluginFactory::loadPlugin(QString::fromUtf8(TEST_GMAIL_PLUGIN), &error);
  QVERIFY2(gmail != nullptr, qPrintable(error));
  QVERIFY(gmail->code() != plugin->code());
#endif
}

void TestGCalendarAccount::calendarLayoutsSurviveRecreationAndAbsence_data() {
  QTest::addColumn<bool>("profiles");
  QTest::newRow("common") << false;
  QTest::newRow("source-profiles") << true;
}

void TestGCalendarAccount::calendarLayoutsSurviveRecreationAndAbsence() {
  QFETCH(bool, profiles);
  QVERIFY(discover());
  const int ends = MSG_MDL_LABELS + 1;
  const int location = ends + 1;
  const int all_day = location + 1;
  auto pane = std::make_unique<FeedMessageViewer>();
  auto* view = pane->messagesView();
  QVERIFY(QMetaObject::invokeMethod(view, "setColumnProfilesEnabled", Qt::DirectConnection, Q_ARG(bool, profiles)));
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, feed())));
  QCOMPARE(view->sourceModel()->columnCount(), all_day + 1);
  view->setColumnHidden(ends, false);
  view->header()->setSectionResizeMode(ends, QHeaderView::Fixed);
  view->header()->resizeSection(ends, 173);
  view->setColumnHidden(location, false);
  view->header()->setSectionResizeMode(location, QHeaderView::Interactive);
  view->header()->resizeSection(location, 247);
  view->header()->moveSection(view->header()->visualIndex(location), 1);
  view->header()->setSectionResizeMode(all_day, QHeaderView::ResizeToContents);
  view->setColumnHidden(MSG_MDL_DCREATED_INDEX, true);
  // Losing the provider must save its layout before installing the ordinary column schema.
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, nullptr)));
  QCOMPARE(view->sourceModel()->columnCount(), MSG_MDL_LABELS + 1);
  pane.reset();
  qApp->settings()->sync();
  pane = std::make_unique<FeedMessageViewer>();
  view = pane->messagesView();
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, feed())));
  QCOMPARE(view->header()->sectionSize(ends), 173);
  QCOMPARE(view->header()->sectionResizeMode(ends), QHeaderView::Fixed);
  QCOMPARE(view->header()->sectionSize(location), 247);
  QCOMPARE(view->header()->visualIndex(location), 1);
  QCOMPARE(view->header()->sectionResizeMode(all_day), QHeaderView::ResizeToContents);
  QVERIFY(view->isColumnHidden(MSG_MDL_DCREATED_INDEX));
  QCOMPARE(view->header()->sortIndicatorSection(), MSG_MDL_DCREATED_INDEX);
  QVERIFY(!view->isColumnHidden(ends));
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, m_account)));
  QCOMPARE(view->isColumnHidden(ends), profiles);
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, feed())));
  QCOMPARE(view->header()->sectionSize(location), 247);
  QVERIFY(!view->isColumnHidden(ends));
  QVERIFY(QMetaObject::invokeMethod(view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, nullptr)));
}

void TestGCalendarAccount::setupTestUsesEnteredSettingsAndStagesGrant() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::modifyAccountCustomData(db, m_account->accountId(), [](QVariantHash& data) {
      data[QSL("refresh_token")] = QSL("live-token");
      data[QSL("unknown")] = 42;
      auto calendar = QJsonObject::fromVariantHash(data.value(QSL("gcalendar")).toHash());
      calendar[QSL("ledger")] = QJsonObject{{QSL("version"), 1}, {QSL("entries"), QJsonObject()}};
      data[QSL("gcalendar")] = calendar.toVariantHash();
    });
  });
  const auto load = [&]() {
    return qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
      return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
    });
  };
  SetupDetails details(m_server);
  const QNetworkProxy proxy(QNetworkProxy::HttpProxy, QSL("setup-proxy"), 8123);
  QSignalSpy finished(&details, &GCalendarAccountDetails::setupTestFinished);
  details.testSetup(proxy);
  QVERIFY(details.isTesting());
  QVERIFY(!details.m_ui.m_btnTestSetup->isEnabled());
  const QUrlQuery authorization(details.m_authorizationUrl);
  QCOMPARE(authorization.queryItemValue(QSL("client_id")), QSL("fixture-client"));
  QCOMPARE(authorization.queryItemValue(QSL("code_challenge_method")), QSL("S256"));
  QCOMPARE(authorization.queryItemValue(QSL("access_type")), QSL("offline"));
  QVERIFY(QUrl(authorization.queryItemValue(QSL("redirect_uri"))).port() > 0);
  details.grant();
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(finished.first().first().toBool());
  QCOMPARE(details.m_ui.m_lblTestResult->status(), WidgetWithStatus::StatusType::Ok);
  QCOMPARE(details.testedRefreshToken(), QSL("tested-refresh"));
  QCOMPARE(load().value(QSL("refresh_token")).toString(), QSL("live-token"));
  QCOMPARE(m_server->m_requests.size(), 2);
  QCOMPARE(m_server->m_lastProxy, proxy);
  for (const auto& request : m_server->m_requests) {
    QCOMPARE(request.m_method, QByteArray("GET"));
    QCOMPARE(request.m_headers.value("Authorization"), QByteArray("Bearer setup-access"));
  }
  // Saving the tested grant is one fresh serialized account update, including changed client credentials.
  m_account->stop();
  m_account->applySettings(QSL("Tested account"),
                           QSL("fixture-client"),
                           QSL("fixture-secret"),
                           30,
                           365,
                           m_account->reminderSettings(),
                           details.testedRefreshToken());
  const auto saved = load();
  QCOMPARE(saved.value(QSL("refresh_token")).toString(), QSL("tested-refresh"));
  QCOMPARE(saved.value(QSL("unknown")).toInt(), 42);
  QVERIFY(QJsonObject::fromVariantHash(saved.value(QSL("gcalendar")).toHash()).contains(QSL("ledger")));
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TRIGGER reject_calendar_update BEFORE UPDATE ON Accounts "
                   "BEGIN SELECT RAISE(ABORT, 'fixture failure'); END;"));
  });
  QVERIFY_EXCEPTION_THROWN(m_account->applySettings(QSL("Rejected account"),
                                                    QSL("fixture-client"),
                                                    QSL("fixture-secret"),
                                                    30,
                                                    365,
                                                    {},
                                                    QSL("must-not-be-saved")),
                           ApplicationException);
  QCOMPARE(load(), saved);
  details.m_ui.m_txtClientId->lineEdit()->setText(QSL("another-client"));
  QVERIFY(details.testedRefreshToken().isEmpty());
  QCOMPARE(details.m_ui.m_lblTestResult->status(), WidgetWithStatus::StatusType::Information);
  details.testSetup(proxy);
  details.grant();
  QTRY_COMPARE(finished.size(), 2);
  QVERIFY(!details.testedRefreshToken().isEmpty());
  details.invalidateTest();
  QVERIFY(details.testedRefreshToken().isEmpty());
}

void TestGCalendarAccount::setupTestFailuresKeepAccountUntouched_data() {
  QTest::addColumn<int>("failure");
  QTest::newRow("authorization-declined") << 0;
  QTest::newRow("events-scope-denied") << 1;
  QTest::newRow("missing-access-token") << 2;
  QTest::newRow("missing-refresh-token") << 3;
}

void TestGCalendarAccount::setupTestFailuresKeepAccountUntouched() {
  QFETCH(int, failure);
  const auto before = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  SetupDetails details(m_server, failure == 1);
  QSignalSpy finished(&details, &GCalendarAccountDetails::setupTestFinished);
  details.testSetup({});
  if (failure == 0) {
    emit details.m_authorization->authFailed();
  }
  else {
    details.grant(failure == 2 ? QString() : QSL("setup-access"), failure == 3 ? QString() : QSL("tested-refresh"));
  }
  QTRY_COMPARE(finished.size(), 1);
  QVERIFY(!finished.first().first().toBool());
  QVERIFY(!details.isTesting());
  QVERIFY(details.m_ui.m_btnTestSetup->isEnabled());
  QVERIFY(details.testedRefreshToken().isEmpty());
  QCOMPARE(details.m_ui.m_lblTestResult->status(), WidgetWithStatus::StatusType::Error);
  const auto after = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  QCOMPARE(after, before);
}

void TestGCalendarAccount::closingSetupCancelsNetworkCheck() {
  auto details = std::make_unique<SetupDetails>(m_server);
  m_server->m_holdReads.store(true);
  details->testSetup({});
  details->grant();
  QTRY_VERIFY(m_server->m_readEntered.available() > 0);
  details.reset();
  QCoreApplication::processEvents();
  QVERIFY(records().isEmpty());
}

QList<Feed*> TestGCalendarAccount::calendarsForEditing() {
  m_server->m_calendars = {calendar(QSL("first"), QSL("First calendar")),
                           calendar(QSL("second"), QSL("Second calendar")),
                           calendar(QSL("third"), QSL("Unselected calendar"))};
  if (!discover()) {
    return {};
  }
  const auto feeds = m_account->getHashedSubTreeFeeds();
  const auto now = m_account->m_now;
  m_account->m_now = now.addSecs(-3600);
  auto initial = localOverride(0, 3);
  auto rule = initial.value(QSL("rules")).toArray().first().toObject();
  rule[QSL("id")] = QSL("first-existing-zero");
  initial[QSL("rules")] = QJsonArray{rule};
  m_account->setCalendarReminderOverride(dynamic_cast<GCalendarFeed*>(feeds.value(QSL("first"))), initial);
  m_account->m_now = now.addSecs(-1800);
  m_account->setCalendarReminderOverride(dynamic_cast<GCalendarFeed*>(feeds.value(QSL("second"))),
                                         {{QSL("mode"), QSL("disabled")}});
  m_account->m_now = now;
  const QList<Feed*> result{feeds.value(QSL("first")), feeds.value(QSL("second")), feeds.value(QSL("third"))};
  for (int i = 0; i < result.size(); ++i) {
    result.at(i)->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(i));
    result.at(i)->setAutoUpdateInterval((i + 1) * 111);
  }
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    for (auto* feed : result) {
      SqlQuery query(db);
      query.prepare(QSL("UPDATE Feeds SET update_type = :type, update_interval = :interval WHERE id = :id;"));
      query.bindValue(QSL(":type"), int(feed->autoUpdateType()));
      query.bindValue(QSL(":interval"), feed->autoUpdateInterval());
      query.bindValue(QSL(":id"), feed->id());
      query.exec();
    }
  });
  return result;
}

QHash<int, QVariantHash> TestGCalendarAccount::feedRows() const {
  return qApp->database()->worker()->read<QHash<int, QVariantHash>>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT * FROM Feeds WHERE account_id = :account;"));
    query.bindValue(QSL(":account"), m_account->accountId());
    query.exec();
    QHash<int, QVariantHash> rows;
    while (query.next()) {
      QVariantHash values;
      const auto record = query.record();
      for (int i = 0; i < record.count(); ++i) {
        values.insert(record.fieldName(i), query.value(i));
      }
      rows.insert(values.value(QSL("id")).toInt(), values);
    }
    return rows;
  });
}

void TestGCalendarAccount::calendarFeedEditorUsesSharedReminderTab() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  const int requests = m_server->requestCount();
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  QObject context;
  bool inspected = false;
  QTimer::singleShot(0, &context, [&]() {
    auto* dialog = qobject_cast<FormGCalendarFeedDetails*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      return;
    }
    auto* details = dialog->findChild<GCalendarReminderDetails*>();
    auto* tabs = dialog->findChild<QTabWidget*>(QSL("m_tabWidget"));
    if (details == nullptr || tabs == nullptr) {
      dialog->reject();
      return;
    }
    auto& ui = details->m_ui;
    inspected = tabs->count() == 2 && !ui.m_cbEnabled->isVisible() && !ui.m_spinCheckInterval->isVisible() &&
                !ui.m_timeAllDay->isVisible() && !ui.m_txtNotificationTitle->isVisible() &&
                !ui.m_panelSelection->isVisible() && ui.m_cmbMode->isEnabled() &&
                ui.m_cmbMode->currentData().toString() == QSL("local") && ui.m_txtOffsets->text() == QSL("0");
    ui.m_txtOffsets->setText(QSL("15, 0"));
    ui.m_spinRepeatInterval->setValue(7);
    dialog->findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
    auto* update_type = dialog->findChild<QComboBox*>(QSL("m_cmbAutoUpdateType"));
    update_type->setCurrentIndex(update_type->findData(int(Feed::AutoUpdateType::SpecificAutoUpdate)));
    dialog->findChild<QDoubleSpinBox*>(QSL("m_spinAutoUpdateInterval"))->setValue(123);
    saveSnapshot(dialog, QSL("calendar-feed-reminders"));
    dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
  });
  view->editItems({feeds.first()});
  QVERIFY(inspected);
  const auto rows = feedRows();
  QCOMPARE(rows.value(feeds.at(1)->id()), before.value(feeds.at(1)->id()));
  QCOMPARE(rows.value(feeds.at(2)->id()), before.value(feeds.at(2)->id()));
  QVERIFY(!feeds.first()->isQuiet());
  QCOMPARE(feeds.first()->autoUpdateType(), Feed::AutoUpdateType::SpecificAutoUpdate);
  QCOMPARE(feeds.first()->autoUpdateInterval(), 123);
  QCOMPARE(rows.value(feeds.first()->id()).value(QSL("is_quiet")).toBool(), false);
  QCOMPARE(rows.value(feeds.first()->id()).value(QSL("update_interval")).toInt(), 123);
  const auto rules = dynamic_cast<GCalendarFeed*>(feeds.first())
                       ->calendarMetadata()
                       .value(QSL("reminder_override"))
                       .toObject()
                       .value(QSL("rules"))
                       .toArray();
  QCOMPARE(rules.size(), 2);
  QCOMPARE(rules.first().toObject().value(QSL("minutes_before")).toInt(), 15);
  QCOMPARE(rules.first().toObject().value(QSL("repeat_minutes")).toInt(), 7);
  QCOMPARE(m_server->requestCount(), requests);
  AccountDialog account(m_account);
  QVERIFY(account.findChild<GCalendarReminderDetails*>() != nullptr);
}

void TestGCalendarAccount::batchCalendarEditorKeepsOverridesByDefault() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  QObject context;
  bool inspected = false;
  QTimer::singleShot(0, &context, [&]() {
    auto* dialog = qobject_cast<FormGCalendarFeedDetails*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      return;
    }
    auto* details = dialog->findChild<GCalendarReminderDetails*>();
    if (details == nullptr) {
      dialog->reject();
      return;
    }
    auto& ui = details->m_ui;
    inspected = ui.m_panelSelection->isVisible() && !ui.m_mcbReminders->isChecked() && !ui.m_cmbMode->isEnabled() &&
                ui.m_cmbMode->currentIndex() == -1 && ui.m_lblMixed->isVisible() && ui.m_lblTiming->isEnabled();
    dialog->findChild<QCheckBox*>(QSL("m_mcbSuppressFeed"))->setChecked(true);
    dialog->findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
    saveSnapshot(dialog, QSL("calendar-feed-batch-mixed"));
    dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
  });
  view->editItems({feeds.at(0), feeds.at(1)});
  QVERIFY(inspected);
  const auto after = feedRows();
  for (int i = 0; i < 2; ++i) {
    const int id = feeds.at(i)->id();
    QCOMPARE(after.value(id).value(QSL("custom_data")), before.value(id).value(QSL("custom_data")));
    QCOMPARE(after.value(id).value(QSL("is_quiet")).toBool(), false);
    auto expected = before.value(id);
    expected[QSL("is_quiet")] = after.value(id).value(QSL("is_quiet"));
    QCOMPARE(after.value(id), expected);
    QVERIFY(!feeds.at(i)->isQuiet());
  }
  QCOMPARE(after.value(feeds.at(2)->id()), before.value(feeds.at(2)->id()));
}

void TestGCalendarAccount::batchCalendarEditorAppliesSelectedMode_data() {
  QTest::addColumn<QString>("mode");
  for (const auto& mode : {QSL("inherit"), QSL("disabled"), QSL("local"), QSL("google")}) {
    QTest::newRow(qPrintable(mode)) << mode;
  }
}

void TestGCalendarAccount::batchCalendarEditorAppliesSelectedMode() {
  QFETCH(QString, mode);
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  FeedDialog dialog(m_account, {feeds.at(0), feeds.at(1)});
  auto* details = dialog.findChild<GCalendarReminderDetails*>();
  QVERIFY(details != nullptr);
  auto& ui = details->m_ui;
  ui.m_mcbReminders->setChecked(true);
  ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(mode));
  ui.m_txtOffsets->setText(QSL("0, 60"));
  ui.m_spinRepeatInterval->setValue(10);
  // Emulate a more recent writer while the dialog still has its original objects.
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    for (int i = 0; i < 2; ++i) {
      auto data = storedCalendarData(before.value(feeds.at(i)->id()));
      data[QSL("unknown")] = i + 10;
      auto calendar = data.value(QSL("gcalendar")).toObject();
      calendar[QSL("runtime_key")] = QSL("retained-%1").arg(i);
      auto resource = calendar.value(QSL("resource")).toObject();
      resource[QSL("description")] = QSL("Latest calendar %1").arg(i);
      calendar[QSL("resource")] = resource;
      auto rules = calendar.value(QSL("reminder_override")).toObject();
      rules[QSL("unknown_rule_key")] = i;
      calendar[QSL("reminder_override")] = rules;
      data[QSL("gcalendar")] = calendar;
      DatabaseQueries::storeCustomData(db, QSL("Feeds"), feeds.at(i)->id(), data.toVariantHash());
    }
    DatabaseQueries::modifyAccountCustomData(db, m_account->accountId(), [](QVariantHash& data) {
      data[QSL("refresh_token")] = QSL("fresh-token-during-feed-edit");
      data[QSL("unknown_account_key")] = 42;
      auto calendar = data.value(QSL("gcalendar")).toHash();
      calendar[QSL("ledger")] = QVariantHash{{QSL("version"), 1}, {QSL("entries"), QVariantHash()}};
      data[QSL("gcalendar")] = calendar;
    });
  });
  const auto fresh = feedRows();
  const auto account_before = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  dialog.apply();
  const auto after = feedRows();
  for (int i = 0; i < 2; ++i) {
    const int id = feeds.at(i)->id();
    const auto original = storedCalendarData(fresh.value(id));
    const auto data = storedCalendarData(after.value(id));
    const auto calendar = data.value(QSL("gcalendar")).toObject();
    const auto override = calendar.value(QSL("reminder_override")).toObject();
    QCOMPARE(data.value(QSL("unknown")), original.value(QSL("unknown")));
    QCOMPARE(calendar.value(QSL("resource")), original.value(QSL("gcalendar")).toObject().value(QSL("resource")));
    QCOMPARE(calendar.value(QSL("runtime_key")), QJsonValue(QSL("retained-%1").arg(i)));
    QCOMPARE(override.value(QSL("mode")), QJsonValue(mode));
    QCOMPARE(override.value(QSL("unknown_rule_key")).toInt(), i);
    QCOMPARE(after.value(id).value(QSL("custom_id")), before.value(id).value(QSL("custom_id")));
    QCOMPARE(after.value(id).value(QSL("icon")), before.value(id).value(QSL("icon")));
    auto common = after.value(id);
    auto old_common = before.value(id);
    common.remove(QSL("custom_data"));
    old_common.remove(QSL("custom_data"));
    QCOMPARE(common, old_common);
    QCOMPARE(QJsonObject::fromVariantHash(feeds.at(i)->customDatabaseData()), data);
    if (mode == QSL("local")) {
      const auto rules = override.value(QSL("rules")).toArray();
      QCOMPARE(rules.size(), 2);
      QCOMPARE(rules.first().toObject().value(QSL("repeat_minutes")).toInt(), 10);
      if (i == 0) {
        const auto old_rule = original.value(QSL("gcalendar"))
                                .toObject()
                                .value(QSL("reminder_override"))
                                .toObject()
                                .value(QSL("rules"))
                                .toArray()
                                .first()
                                .toObject();
        QCOMPARE(rules.first().toObject().value(QSL("enabled_at")), old_rule.value(QSL("enabled_at")));
        QCOMPARE(rules.first().toObject().value(QSL("id")), old_rule.value(QSL("id")));
      }
    }
    const auto old = original.value(QSL("gcalendar")).toObject().value(QSL("reminder_override")).toObject();
    if (old.value(QSL("mode")) == override.value(QSL("mode"))) {
      QCOMPARE(override.value(QSL("enabled_at")), old.value(QSL("enabled_at")));
    }
  }
  QCOMPARE(after.value(feeds.at(2)->id()), before.value(feeds.at(2)->id()));
  const auto account_after = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  QCOMPARE(account_after, account_before);
  // An unchanged configuration must not look mixed merely because timestamps differ.
  FeedDialog reopened(m_account, {feeds.at(0), feeds.at(1)});
  auto* shared = reopened.findChild<GCalendarReminderDetails*>();
  QVERIFY(shared != nullptr);
  QCOMPARE(shared->m_ui.m_cmbMode->currentData().toString(), mode);
  QVERIFY(shared->m_ui.m_lblMixed->isHidden());
}

void TestGCalendarAccount::failedCalendarEditsPreserveDatabaseAndMemory_data() {
  QTest::addColumn<QString>("failure");
  for (const auto& failure : {QSL("common-write"),
                              QSL("override-write"),
                              QSL("metadata"),
                              QSL("missing-calendar"),
                              QSL("duplicate-offset"),
                              QSL("cache-window")}) {
    QTest::newRow(qPrintable(failure)) << failure;
  }
}

void TestGCalendarAccount::failedCalendarEditsPreserveDatabaseAndMemory() {
  QFETCH(QString, failure);
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  if (failure == QSL("cache-window")) {
    m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 1, {});
  }
  FeedDialog dialog(m_account, {feeds.at(0), feeds.at(1)});
  auto* details = dialog.findChild<GCalendarReminderDetails*>();
  QVERIFY(details != nullptr);
  auto& ui = details->m_ui;
  ui.m_mcbReminders->setChecked(true);
  ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(failure == QSL("cache-window") ? QSL("google") : QSL("local")));
  ui.m_txtOffsets->setText(failure == QSL("duplicate-offset") ? QSL("0, 0") : QSL("0"));
  ui.m_spinRepeatInterval->setValue(10);
  dialog.findChild<QCheckBox*>(QSL("m_mcbSuppressFeed"))->setChecked(true);
  dialog.findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
  const auto original_first = feeds.first()->customDatabaseData();
  const auto original_second = feeds.at(1)->customDatabaseData();
  if (failure == QSL("metadata")) {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      auto data = feeds.at(1)->customDatabaseData();
      auto calendar = data.value(QSL("gcalendar")).toHash();
      calendar[QSL("version")] = 99;
      data[QSL("gcalendar")] = calendar;
      DatabaseQueries::storeCustomData(db, QSL("Feeds"), feeds.at(1)->id(), data);
    });
  }
  if (failure.endsWith(QSL("write"))) {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(QSL("CREATE TRIGGER reject_calendar_update BEFORE UPDATE %1 ON Feeds WHEN NEW.id = %2 "
                     "BEGIN SELECT RAISE(ABORT, 'fixture calendar save failure'); END;")
                   .arg(failure == QSL("override-write") ? QSL("OF custom_data") : QString())
                   .arg(feeds.at(1)->id()));
    });
  }
  if (failure == QSL("missing-calendar")) {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(QSL("DELETE FROM Feeds WHERE id = %1;").arg(feeds.at(1)->id()));
    });
  }
  const auto before = feedRows();
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
  QCOMPARE(feedRows(), before);
  QVERIFY(feeds.at(0)->isQuiet());
  QVERIFY(feeds.at(1)->isQuiet());
  QCOMPARE(feeds.at(0)->customDatabaseData(), original_first);
  QCOMPARE(feeds.at(1)->customDatabaseData(), original_second);
  QCOMPARE(ui.m_spinRepeatInterval->value(), 10);
  QVERIFY(ui.m_mcbReminders->isChecked());
  if (failure == QSL("missing-calendar")) {
    return;
  }
  // Repair the failing condition and retry the same draft.
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER IF EXISTS reject_calendar_update;"));
    if (failure == QSL("metadata")) {
      DatabaseQueries::storeCustomData(db, QSL("Feeds"), feeds.at(1)->id(), original_second);
    }
  });
  if (failure == QSL("cache-window")) {
    m_account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 365, {});
  }
  ui.m_txtOffsets->setText(QSL("0"));
  dialog.apply();
  QVERIFY(!feeds.at(0)->isQuiet());
  QVERIFY(!feeds.at(1)->isQuiet());
}

void TestGCalendarAccount::cancelledCalendarEditPreservesSelection() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  QObject context;
  bool opened = false;
  QTimer::singleShot(0, &context, [&]() {
    auto* dialog = qobject_cast<FormGCalendarFeedDetails*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      return;
    }
    auto* details = dialog->findChild<GCalendarReminderDetails*>();
    if (details != nullptr) {
      opened = true;
      details->m_ui.m_mcbReminders->setChecked(true);
      details->m_ui.m_cmbMode->setCurrentIndex(details->m_ui.m_cmbMode->findData(QSL("google")));
      dialog->findChild<QCheckBox*>(QSL("m_mcbSuppressFeed"))->setChecked(true);
      dialog->findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
    }
    dialog->reject();
  });
  view->editItems({feeds.at(0), feeds.at(1)});
  QVERIFY(opened);
  QCOMPARE(feedRows(), before);
  for (auto* feed : feeds) {
    QVERIFY(feed->isQuiet());
    QCOMPARE(QJsonObject::fromVariantHash(feed->customDatabaseData()), storedCalendarData(before.value(feed->id())));
  }
}

void TestGCalendarAccount::calendarReminderWidgetPreservesDependencies() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  FeedDialog dialog(m_account, {feeds.at(0), feeds.at(1)});
  auto* details = dialog.findChild<GCalendarReminderDetails*>();
  QVERIFY(details != nullptr);
  auto& ui = details->m_ui;
  QVERIFY(!ui.m_cmbMode->isEnabled());
  ui.m_mcbReminders->setChecked(true);
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
  ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(QSL("local")));
  QVERIFY(ui.m_txtOffsets->isEnabled());
  QVERIFY(!ui.m_spinRepeatInterval->isEnabled());
  ui.m_txtOffsets->setText(QSL("0"));
  ui.m_spinRepeatInterval->setValue(12);
  QVERIFY(ui.m_spinRepeatInterval->isEnabled());
  ui.m_mcbReminders->setChecked(false);
  QVERIFY(!ui.m_spinRepeatInterval->isEnabled());
  QVERIFY(ui.m_lblTiming->isEnabled());
  ui.m_mcbReminders->setChecked(true);
  QVERIFY(ui.m_spinRepeatInterval->isEnabled());
  ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(QSL("google")));
  QVERIFY(!ui.m_txtOffsets->isEnabled());
  QVERIFY(!ui.m_spinRepeatInterval->isEnabled());
  ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(QSL("local")));
  QCOMPARE(ui.m_spinRepeatInterval->value(), 12);
  ui.m_txtOffsets->clear();
  QVERIFY(!ui.m_spinRepeatInterval->isEnabled());
  QCOMPARE(ui.m_spinRepeatInterval->value(), 12);
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
}

void TestGCalendarAccount::calendarCommonSettingsPreserveRemoteData_data() {
  QTest::addColumn<bool>("absolute");
  QTest::newRow("absolute-date") << true;
  QTest::newRow("relative-hours") << false;
}

void TestGCalendarAccount::calendarCommonSettingsPreserveRemoteData() {
  QFETCH(bool, absolute);
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  FeedDialog dialog(m_account, {feeds.first()});
  dialog.findChild<QCheckBox*>(QSL("m_cbDisableFeed"))->setChecked(true);
  dialog.findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
  dialog.findChild<QCheckBox*>(QSL("m_cbExcludeFromGlobalUnreadCount"))->setChecked(false);
  dialog.findChild<QCheckBox*>(QSL("m_cbOpenArticlesAutomatically"))->setChecked(true);
  auto* rtl = dialog.findChild<QComboBox*>(QSL("m_cmbRtlBehavior"));
  rtl->setCurrentIndex(rtl->findData(QVariant::fromValue(RtlBehavior::Everywhere)));
  dialog.findChild<QCheckBox*>(QSL("m_cbAddAnyDateArticles"))->setChecked(true);
  dialog.findChild<QGroupBox*>(QSL("m_gbAvoidOldArticles"))->setChecked(true);
  dialog.findChild<QRadioButton*>(absolute ? QSL("m_rbAvoidAbsolute") : QSL("m_rbAvoidRelative"))->setChecked(true);
  auto* date = dialog.findChild<QDateTimeEdit*>(QSL("m_dtDateTimeToAvoid"));
  date->setDateTime(QDateTime::currentDateTimeUtc().addDays(-10));
  dialog.findChild<QDoubleSpinBox*>(QSL("m_spinHoursAvoid"))->setValue(48);
  dialog.findChild<QCheckBox*>(QSL("m_cbArticleLimittingCustomize"))->setChecked(true);
  dialog.findChild<QSpinBox*>(QSL("m_spinArticleCount"))->setValue(17);
  dialog.findChild<QCheckBox*>(QSL("m_cbNoRemoveImportant"))->setChecked(false);
  dialog.findChild<QCheckBox*>(QSL("m_cbNoRemoveUnread"))->setChecked(false);
  dialog.findChild<QCheckBox*>(QSL("m_cbMoveToBinNoPurge"))->setChecked(true);
  const auto expected_date = date->dateTime().toMSecsSinceEpoch();
  dialog.apply();
  const auto after = feedRows();
  const auto row = after.value(feeds.first()->id());
  QCOMPARE(row.value(QSL("is_off")).toBool(), true);
  QCOMPARE(row.value(QSL("is_quiet")).toBool(), false);
  QCOMPARE(row.value(QSL("is_rtl")).toInt(), int(RtlBehavior::Everywhere));
  QCOMPARE(row.value(QSL("open_articles")).toBool(), true);
  QCOMPARE(row.value(QSL("exclude_global_unread")).toBool(), false);
  QCOMPARE(row.value(QSL("add_any_datetime_articles")).toBool(), true);
  QCOMPARE(row.value(QSL("datetime_to_avoid")).toLongLong(), absolute ? expected_date : qint64(48));
  QCOMPARE(row.value(QSL("keep_article_customize")).toBool(), true);
  QCOMPARE(row.value(QSL("keep_article_count")).toInt(), 17);
  QCOMPARE(row.value(QSL("keep_unread_articles")).toBool(), false);
  QCOMPARE(row.value(QSL("keep_starred_articles")).toBool(), false);
  QCOMPARE(row.value(QSL("recycle_articles")).toBool(), true);
  for (const auto& key :
       {QSL("custom_id"), QSL("title"), QSL("icon"), QSL("category"), QSL("source"), QSL("date_created")}) {
    QCOMPARE(row.value(key), before.value(feeds.first()->id()).value(key));
  }
  QVERIFY(feeds.first()->isSwitchedOff());
  QVERIFY(feeds.first()->openArticlesDirectly());
  QCOMPARE(feeds.first()->articleIgnoreLimit().m_keepCountOfArticles, 17);
  QCOMPARE(after.value(feeds.at(1)->id()), before.value(feeds.at(1)->id()));
  QCOMPARE(after.value(feeds.at(2)->id()), before.value(feeds.at(2)->id()));
}

void TestGCalendarAccount::commonOnlyCalendarEditsPreserveReminders_data() {
  QTest::addColumn<QString>("scenario");
  for (const auto& scenario :
       {QSL("different-repeats"), QSL("inherited"), QSL("fresh-override"), QSL("restored-input")}) {
    QTest::newRow(qPrintable(scenario)) << scenario;
  }
}

void TestGCalendarAccount::commonOnlyCalendarEditsPreserveReminders() {
  QFETCH(QString, scenario);
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  auto* calendar = dynamic_cast<GCalendarFeed*>(feeds.first());
  QVERIFY(calendar != nullptr);
  auto settings = localOverride(0, 5);
  auto rules = settings.value(QSL("rules")).toArray();
  rules.append(QJsonObject{{QSL("id"), QSL("before-hour")}, {QSL("minutes_before"), 60}, {QSL("repeat_minutes"), 15}});
  settings[QSL("rules")] = rules;
  m_account->setCalendarReminderOverride(calendar, settings);
  if (scenario == QSL("inherited")) {
    auto data = calendar->customDatabaseData();
    auto metadata = calendar->calendarMetadata();
    metadata.remove(QSL("reminder_override"));
    data[QSL("gcalendar")] = metadata.toVariantHash();
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::storeCustomData(db, QSL("Feeds"), calendar->id(), data);
    });
    calendar->setCustomDatabaseData(data);
  }
  FeedDialog dialog(m_account, {calendar});
  auto* details = dialog.findChild<GCalendarReminderDetails*>();
  QVERIFY(details != nullptr);
  auto& ui = details->m_ui;
  if (scenario == QSL("restored-input")) {
    const auto original_mode = ui.m_cmbMode->currentIndex();
    const auto original_offsets = ui.m_txtOffsets->text();
    const int original_repeat = ui.m_spinRepeatInterval->value();
    ui.m_cmbMode->setCurrentIndex(ui.m_cmbMode->findData(QSL("google")));
    ui.m_cmbMode->setCurrentIndex(original_mode);
    ui.m_txtOffsets->clear();
    ui.m_txtOffsets->setText(original_offsets);
    ui.m_spinRepeatInterval->setValue(original_repeat + 1);
    ui.m_spinRepeatInterval->setValue(original_repeat);
  }
  if (scenario == QSL("fresh-override")) {
    auto data = calendar->customDatabaseData();
    auto metadata = calendar->calendarMetadata();
    metadata[QSL("reminder_override")] = QJsonObject{{QSL("version"), 1},
                                                     {QSL("mode"), QSL("google")},
                                                     {QSL("enabled_at"), m_account->m_now.toString(Qt::ISODateWithMs)}};
    data[QSL("gcalendar")] = metadata.toVariantHash();
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::storeCustomData(db, QSL("Feeds"), calendar->id(), data);
    });
  }
  const auto before = feedRows();
  dialog.findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
  dialog.apply();
  const auto after = feedRows();
  auto expected = before.value(calendar->id());
  expected[QSL("is_quiet")] = after.value(calendar->id()).value(QSL("is_quiet"));
  QCOMPARE(after.value(calendar->id()).value(QSL("custom_data")),
           before.value(calendar->id()).value(QSL("custom_data")));
  QCOMPARE(after.value(calendar->id()), expected);
  QCOMPARE(after.value(feeds.at(1)->id()), before.value(feeds.at(1)->id()));
  QCOMPARE(after.value(feeds.at(2)->id()), before.value(feeds.at(2)->id()));
  QCOMPARE(QJsonObject::fromVariantHash(calendar->customDatabaseData()), storedCalendarData(expected));
  QVERIFY(!calendar->isQuiet());
}

void TestGCalendarAccount::emptyOffsetsPreserveRepeatAfterReopening_data() {
  QTest::addColumn<bool>("google");
  QTest::newRow("disabled-local-reminders") << false;
  QTest::newRow("google-reminders") << true;
}

void TestGCalendarAccount::emptyOffsetsPreserveRepeatAfterReopening() {
  QFETCH(bool, google);
  AccountDialog dialog(m_account);
  auto* details = dialog.findChild<GCalendarReminderDetails*>();
  QVERIFY(details != nullptr);
  auto& ui = details->m_ui;
  ui.m_cbEnabled->setChecked(true);
  ui.m_txtOffsets->setText(QSL("0"));
  ui.m_spinRepeatInterval->setValue(10);
  ui.m_txtOffsets->clear();
  QVERIFY(!ui.m_spinRepeatInterval->isEnabled());
  QCOMPARE(ui.m_spinRepeatInterval->value(), 10);
  ui.m_cbGoogleReminders->setChecked(google);
  ui.m_cbEnabled->setChecked(google);
  dialog.apply();
  const auto saved = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadAccountCustomData(db, m_account->accountId());
  });
  m_account->setCustomDatabaseData(saved);
  AccountDialog reopened(m_account);
  auto* restored = reopened.findChild<GCalendarReminderDetails*>();
  QVERIFY(restored != nullptr);
  QVERIFY(restored->m_ui.m_txtOffsets->text().isEmpty());
  QVERIFY(!restored->m_ui.m_spinRepeatInterval->isEnabled());
  QCOMPARE(restored->m_ui.m_spinRepeatInterval->value(), 10);
  QCOMPARE(m_account->reminderSettings().m_rules.size(), 0);
  restored->m_ui.m_cbEnabled->setChecked(true);
  restored->m_ui.m_cbGoogleReminders->setChecked(false);
  restored->m_ui.m_txtOffsets->setText(QSL("0"));
  QVERIFY(restored->m_ui.m_spinRepeatInterval->isEnabled());
  QCOMPARE(restored->m_ui.m_spinRepeatInterval->value(), 10);
  reopened.apply();
  QCOMPARE(m_account->reminderSettings().m_rules.first().m_repeatMinutes, 10);
}

void TestGCalendarAccount::calendarEditorRejectsUnsupportedDataWithoutLockingUpdates() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  auto data = feeds.first()->customDatabaseData();
  auto calendar = data.value(QSL("gcalendar")).toHash();
  calendar[QSL("reminder_override")] = QVariantHash{{QSL("version"), 99}};
  data[QSL("gcalendar")] = calendar;
  feeds.first()->setCustomDatabaseData(data);
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  view->editItems({feeds.first()});
  QVERIFY(!qApp->feedUpdateLock()->isLocked());
  QCOMPARE(feedRows(), before);
  QCOMPARE(feeds.first()->customDatabaseData(), data);
}

void TestGCalendarAccount::calendarEditorRejectsBusySave() {
  const auto feeds = calendarsForEditing();
  QCOMPARE(feeds.size(), 3);
  const auto before = feedRows();
  FeedDialog dialog(m_account, {feeds.at(0), feeds.at(1)});
  dialog.findChild<QCheckBox*>(QSL("m_mcbSuppressFeed"))->setChecked(true);
  dialog.findChild<QCheckBox*>(QSL("m_cbSuppressFeed"))->setChecked(false);
  m_server->store(calendarEvent(QSL("held-read")));
  m_server->m_holdReads.store(true);
  QSignalSpy finished(m_account, &GCalendarServiceRoot::eventOperationFinished);
  m_account->loadRemoteEvent(QSL("first"), QSL("held-read"));
  QTRY_VERIFY(m_server->m_readEntered.available() > 0);
  QVERIFY_EXCEPTION_THROWN(dialog.apply(), ApplicationException);
  QCOMPARE(feedRows(), before);
  QVERIFY(feeds.at(0)->isQuiet());
  QVERIFY(feeds.at(1)->isQuiet());
  m_server->m_holdReads.store(false);
  QTRY_COMPARE(finished.size(), 1);
  dialog.apply();
  QVERIFY(!feeds.at(0)->isQuiet());
  QVERIFY(!feeds.at(1)->isQuiet());
}

void TestGCalendarAccount::accountDialogUsesDesignerForms() {
  auto* view = qApp->mainFormWidget()->findChild<FeedsView*>();
  QVERIFY(view != nullptr);
  QObject context;
  bool inspected = false;
  QTimer::singleShot(0, &context, [&]() {
    auto* dialog = qobject_cast<FormEditGCalendarAccount*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      return;
    }
    auto* name_field = dialog->findChild<LineEditWithStatus*>(QSL("m_txtName"));
    auto* name = name_field == nullptr ? nullptr : name_field->lineEdit();
    auto* enabled = dialog->findChild<QCheckBox*>(QSL("m_cbEnabled"));
    auto* interval = dialog->findChild<QSpinBox*>(QSL("m_spinCheckInterval"));
    auto* google = dialog->findChild<QCheckBox*>(QSL("m_cbGoogleReminders"));
    auto* offsets = dialog->findChild<QLineEdit*>(QSL("m_txtOffsets"));
    auto* repeat = dialog->findChild<QSpinBox*>(QSL("m_spinRepeatInterval"));
    auto* all_day = dialog->findChild<QTimeEdit*>(QSL("m_timeAllDay"));
    auto* title = dialog->findChild<QLineEdit*>(QSL("m_txtNotificationTitle"));
    auto* body = dialog->findChild<QPlainTextEdit*>(QSL("m_txtNotificationBody"));
    auto* timing_help = dialog->findChild<QWidget*>(QSL("m_lblTiming"));
    auto* placeholders = dialog->findChild<QLabel*>(QSL("m_lblPlaceholders"));
    auto* buttons = dialog->findChild<QDialogButtonBox*>();
    auto* test = dialog->findChild<QPushButton*>(QSL("m_btnTestSetup"));
    auto* result = dialog->findChild<LabelWithStatus*>(QSL("m_lblTestResult"));
    auto* refresh_calendars = dialog->findChild<QCheckBox*>(QSL("m_cbRefreshCalendarListOnStartup"));
    if (name == nullptr || enabled == nullptr || interval == nullptr || google == nullptr || offsets == nullptr ||
        repeat == nullptr || all_day == nullptr || title == nullptr || body == nullptr || timing_help == nullptr ||
        placeholders == nullptr || buttons == nullptr || test == nullptr || result == nullptr ||
        refresh_calendars == nullptr) {
      dialog->reject();
      return;
    }
    inspected = name->text() == m_account->title() && interval->value() == 30 && !refresh_calendars->isChecked();
    saveSnapshot(dialog, QSL("account-setup"));
    auto* tabs = dialog->findChild<QTabWidget*>(QSL("m_tabWidget"));
    if (tabs != nullptr) {
      tabs->setCurrentIndex(1);
      saveSnapshot(dialog, QSL("account-reminders"));
    }
    const QList<QWidget*> controls{interval, google, offsets, repeat, all_day, title, body};
    const auto controls_enabled = [&](bool value) {
      return std::all_of(controls.constBegin(), controls.constEnd(), [value](const QWidget* control) {
        return control->isEnabled() == value;
      });
    };
    inspected = inspected && !enabled->isChecked() && controls_enabled(false) && enabled->isEnabled() &&
                timing_help->isEnabled() && placeholders->isEnabled();
    enabled->setChecked(true);
    inspected = inspected && controls_enabled(true);
    interval->setValue(45);
    offsets->setText(QSL("60, 0"));
    repeat->setValue(10);
    all_day->setTime(QTime(8, 30));
    title->setText(QSL("Calendar: {title}"));
    body->setPlainText(QSL("{calendar}: {start}"));
    offsets->clear();
    inspected = inspected && !repeat->isEnabled() && repeat->value() == 10;
    google->setChecked(true);
    google->setChecked(false);
    enabled->setChecked(false);
    enabled->setChecked(true);
    inspected = inspected && !repeat->isEnabled() && repeat->value() == 10;
    offsets->setText(QSL("  "));
    inspected = inspected && !repeat->isEnabled() && repeat->value() == 10;
    offsets->setText(QSL("0"));
    inspected = inspected && repeat->isEnabled() && repeat->value() == 10;
    google->setChecked(true);
    offsets->setText(QSL("60, 0"));
    inspected = inspected && !offsets->isEnabled() && !repeat->isEnabled() && !all_day->isEnabled();
    enabled->setChecked(false);
    inspected = inspected && controls_enabled(false) && timing_help->isEnabled() && placeholders->isEnabled();
    enabled->setChecked(true);
    inspected = inspected && google->isChecked() && !offsets->isEnabled() && !repeat->isEnabled() &&
                !all_day->isEnabled() && interval->isEnabled() && google->isEnabled() && title->isEnabled() &&
                body->isEnabled();
    google->setChecked(false);
    inspected = inspected && controls_enabled(true);
    saveSnapshot(dialog, QSL("account-reminders-enabled"));
    enabled->setChecked(false);
    inspected = inspected && controls_enabled(false);
    refresh_calendars->setChecked(true);
    buttons->button(QDialogButtonBox::Ok)->click();
  });
  view->editItems({m_account});
  QVERIFY(inspected);
  QVERIFY(!m_account->reminderSettings().m_enabled);
  QCOMPARE(m_account->reminderSettings().m_checkIntervalMinutes, 45);
  QCOMPARE(m_account->reminderSettings().m_rules.size(), 2);
  QCOMPARE(m_account->reminderSettings().m_rules.first().m_minutesBefore, 60);
  QCOMPARE(m_account->reminderSettings().m_rules.first().m_repeatMinutes, 10);
  QCOMPARE(m_account->reminderSettings().m_allDayTime, QTime(8, 30));
  QCOMPARE(m_account->reminderSettings().m_title, QSL("Calendar: {title}"));
  QCOMPARE(m_account->reminderSettings().m_body, QSL("{calendar}: {start}"));
  QVERIFY(m_account->calendarSettings().value(QSL("refresh_calendar_list_on_startup")).toBool());
}

int main(int argc, char* argv[]) {
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  QDir().mkpath(directory.path() + QSL("/") + QSL(APP_CFG_PATH));
  QSettings settings(directory.path() + QSL("/") + QSL(APP_CFG_PATH) + QSL("/") + QSL(APP_CFG_FILE),
                     QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseTrayIcon, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
  // Optional integration run against a disposable local MariaDB server. Default CTest runs use SQLite.
  const int mysql_port = qEnvironmentVariableIntValue("RSSGUARD_TEST_MARIADB_PORT");
  const QString mysql_schema = QSL("rssguard_calendar_test_") + QUuid::createUuid().toString(QUuid::Id128);
  if (mysql_port > 0 && mysql_port <= 65535) {
    TextFactory::initializeSecretEncryptionKey(directory.path() + QSL("/") + QSL(APP_CFG_PATH));
    settings.setValue(GROUP(Database) + QSL("/") + Database::ActiveDriver, QSL(APP_DB_MYSQL_DRIVER));
    settings.setValue(GROUP(Database) + QSL("/") + Database::MySQLHostname, QSL("127.0.0.1"));
    settings.setValue(GROUP(Database) + QSL("/") + Database::MySQLPort, mysql_port);
    settings.setValue(GROUP(Database) + QSL("/") + Database::MySQLUsername,
                      qEnvironmentVariable("RSSGUARD_TEST_MARIADB_USER", QSL("root")));
    settings.setValue(GROUP(Database) + QSL("/") + Database::MySQLPassword,
                      TextFactory::encrypt(qEnvironmentVariable("RSSGUARD_TEST_MARIADB_PASSWORD")));
    settings.setValue(GROUP(Database) + QSL("/") + Database::MySQLDatabase, mysql_schema);
  }
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              directory.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-calendar-account-test"), argc, argv, arguments);
  if (mysql_port > 0 && application.database()->activeDatabaseDriver() != DatabaseDriver::DriverType::MySQL) {
    return 1;
  }
#if defined(Q_OS_WIN)
  if (!qEnvironmentVariableIsEmpty("RSSGUARD_TEST_UI_SNAPSHOTS")) {
    const int font =
      QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR")).filePath(QSL("Fonts/segoeui.ttf")));
    if (font >= 0) {
      application.setFont(QFont(QFontDatabase::applicationFontFamilies(font).first(), 9));
    }
  }
#endif
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  FormMain window;
  TestGCalendarAccount tests;
  const int result = QTest::qExec(&tests, argc, argv);
  if (mysql_port > 0) {
    application.database()->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(QSL("DROP DATABASE ") + DatabaseFactory::escapeIdentifier(mysql_schema));
    });
  }
  return result;
}

#include "test_gcalendaraccount.moc"
