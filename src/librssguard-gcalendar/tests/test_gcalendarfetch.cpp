// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarfeed.h"
#include "gcalendarserviceroot.h"

#include <atomic>
#include <librssguard/core/messagesmodel.h>
#include <librssguard/core/messagesproxymodel.h>
#include <librssguard/database/databasequeries.h>
#include <librssguard/gui/dialogs/formmain.h>
#include <librssguard/gui/feedmessageviewer.h>
#include <librssguard/gui/messagesview.h>
#include <librssguard/gui/tabwidget.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/feedreader.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/oauth2service.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {
  QJsonObject testEvent(const QString& id) {
    const auto start = QDateTime::currentDateTimeUtc().addDays(1);
    return {{QSL("id"), id},
            {QSL("summary"), id},
            {QSL("etag"), QSL("original")},
            {QSL("start"), QJsonObject{{QSL("dateTime"), start.toString(Qt::ISODate)}}},
            {QSL("end"), QJsonObject{{QSL("dateTime"), start.addSecs(3600).toString(Qt::ISODate)}}}};
  }

  class Account : public GCalendarServiceRoot {
    public:
      explicit Account(const GCalendarNetworkFactory::Transport& transport) : GCalendarServiceRoot(nullptr, transport) {}

      virtual void start(bool) override {
        GCalendarServiceRoot::start(false);
        auto* oauth = findChild<OAuth2Service*>();
        oauth->setTokensExpireIn(QDateTime::currentDateTimeUtc().addSecs(3600));
        emit oauth->tokensRetrieved(QSL("fixture"), {}, 3600);
      }
  };
} // namespace

class TestGCalendarFetch : public QObject {
    Q_OBJECT

  private slots:
    void discoveryAndBatchFetch_data();
    void discoveryAndBatchFetch();

  public:
    MessagesView* m_view = nullptr;
};

void TestGCalendarFetch::discoveryAndBatchFetch_data() {
  QTest::addColumn<bool>("update_counts_during_fetch");
  QTest::addColumn<bool>("ignore_contents");
  QTest::newRow("batch-counts") << false << false;
  QTest::newRow("per-feed-counts") << true << false;
  QTest::newRow("global-ignore-enabled") << false << true;
}

void TestGCalendarFetch::discoveryAndBatchFetch() {
  QFETCH(bool, update_counts_during_fetch);
  QFETCH(bool, ignore_contents);
  qApp->settings()->setValue(GROUP(Feeds), Feeds::UpdateFeedListDuringFetching, update_counts_during_fetch);
  qApp->settings()->setValue(GROUP(Messages), Messages::IgnoreContentsChanges, ignore_contents);
  qApp->settings()->setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);
  const auto first = testEvent(QSL("first"));
  const auto second = testEvent(QSL("second"));
  QJsonObject edited = first;
  edited[QSL("reminders")] =
    QJsonObject{{QSL("useDefault"), false},
                {QSL("overrides"), QJsonArray{QJsonObject{{QSL("method"), QSL("popup")}, {QSL("minutes"), 30}}}}};
  auto written = edited;
  written[QSL("location")] = QSL("From the editor");
  written[QSL("etag")] = QSL("saved");
  const QJsonArray calendars{
    QJsonObject{{QSL("id"), QSL("first")}, {QSL("summary"), QSL("First")}, {QSL("accessRole"), QSL("owner")}},
    QJsonObject{{QSL("id"), QSL("second")}, {QSL("summary"), QSL("Second")}, {QSL("accessRole"), QSL("reader")}}};

  std::atomic_bool update{false};
  auto* account = new Account([&](const auto& request, const auto&) -> GCalendarNetworkFactory::Response {
    QJsonObject result;
    if (request.m_method == "PATCH") {
      result = written;
    }
    else if (request.m_url.path().endsWith(QSL("/calendarList"))) {
      result = {{QSL("kind"), QSL("calendar#calendarList")}, {QSL("items"), calendars}};
    }
    else if (request.m_url.path().contains(QSL("/first/"))) {
      result = {{QSL("kind"), QSL("calendar#events")}, {QSL("items"), QJsonArray{update ? edited : first}}};
    }
    else {
      const QJsonObject cancelled{{QSL("id"), QSL("second")}, {QSL("status"), QSL("cancelled")}};
      result = {{QSL("kind"), QSL("calendar#events")}, {QSL("items"), QJsonArray{update ? cancelled : second}}};
    }
    return {NetworkResult(QNetworkReply::NoError, 200, {}, {}), QJsonDocument(result).toJson(), {}};
  });
  account->applySettings(QSL("Calendar fixture"), QSL("fixture-client"), {}, 30, 365, {});
  auto* reader = qApp->feedReader();
  QVERIFY(reader->feedsModel()->addServiceAccount(account, false));
  const auto cleanup = qScopeGuard([&] {
    disconnect(reader, nullptr, this, nullptr);
    QMetaObject::invokeMethod(m_view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, nullptr));
    account->stop();
    qApp->database()->worker()->write([&](const QSqlDatabase& db) { DatabaseQueries::deleteAccount(db, account); });
    reader->feedsModel()->removeItem(account, false);
  });

  QSignalSpy discovery(account, &GCalendarServiceRoot::calendarDiscoveryFinished);
  account->requestSyncIn();
  QVERIFY(discovery.wait(5000));
  QVERIFY(discovery.first().first().toBool());
  const auto feeds = account->getHashedSubTreeFeeds();
  QCOMPARE(feeds.size(), 2);
  QVERIFY(feeds.value(QSL("first"))->id() > 0);

  int completions = 0;
  connect(reader, &FeedReader::feedUpdatesFinished, this, [&](const auto&) { ++completions; });
  reader->updateFeeds(feeds.values());
  QTRY_COMPARE(completions, 1);
  QCOMPARE(feeds.value(QSL("first"))->countOfAllMessages(), 1);
  QCOMPARE(feeds.value(QSL("second"))->countOfAllMessages(), 1);
  const auto stored = [&]() {
    return qApp->database()->worker()->read<QList<Message>>([&](const QSqlDatabase& db) {
      return DatabaseQueries::getUndeletedMessagesForFeed(db, feeds.value(QSL("first"))->id(), {});
    });
  };
  const auto previous = stored();
  QCOMPARE(previous.size(), 1);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    const auto id = previous.first().m_id;
    DatabaseQueries::markMessagesReadUnread(db, {QString::number(id)}, RootItem::ReadStatus::Read);
    DatabaseQueries::markMessageImportant(db, id, RootItem::Importance::Important);
  });

  QVERIFY(QMetaObject::invokeMethod(m_view, "loadItem", Qt::DirectConnection, Q_ARG(RootItem*, account)));
  QVERIFY(m_view->model()->rowCount() >= 2);
  for (int row = 0; row < m_view->model()->rowCount(); ++row) {
    if (m_view->model()->index(row, MSG_MDL_ID_INDEX).data(Qt::EditRole).toInt() == previous.first().m_id) {
      const auto title = m_view->model()->index(row, MSG_MDL_TITLE_INDEX);
      m_view->selectionModel()->setCurrentIndex(title, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      break;
    }
  }
  QVERIFY(m_view->currentIndex().isValid());
  QSignalSpy resets(m_view->sourceModel(), &QAbstractItemModel::modelReset);
  update = true;
  reader->updateFeeds(feeds.values());
  QTRY_COMPARE(completions, 2);
  QCOMPARE(resets.count(), 1);
  const auto selected = m_view->currentIndex();
  QVERIFY(selected.isValid());
  QCOMPARE(selected.sibling(selected.row(), MSG_MDL_ID_INDEX).data(Qt::EditRole).toInt(), previous.first().m_id);
  QCOMPARE(feeds.value(QSL("second"))->countOfAllMessages(), 0);
  const auto after = stored();
  QCOMPARE(after.size(), 1);
  QCOMPARE(after.first().m_id, previous.first().m_id);
  QVERIFY(after.first().m_isRead);
  QVERIFY(after.first().m_isImportant);
  QCOMPARE(CalendarEvent::fromMessage(after.first()).m_resource, edited);

  if (ignore_contents) {
    const int event_id = after.first().m_id;
    const auto setTrashed = [&](bool trashed) {
      qApp->database()->worker()->write([&](const QSqlDatabase& db) {
        DatabaseQueries::deleteOrRestoreMessagesToFromBin(db, {QString::number(event_id)}, trashed);
      });
    };
    setTrashed(true);

    // Fetch the same event with only the reminder removed.
    update = false;
    reader->updateFeeds({feeds.value(QSL("first"))});
    QTRY_COMPARE(completions, 3);
    const auto trashed = account->cachedMessage(event_id);
    QCOMPARE(trashed.m_id, event_id);
    QCOMPARE(CalendarEvent::fromMessage(trashed).m_resource, first);
    QVERIFY(trashed.m_isDeleted);
    QVERIFY(trashed.m_isRead);
    QVERIFY(trashed.m_isImportant);
    QVERIFY(stored().isEmpty());
    setTrashed(false);

    QSignalSpy saved(account, &GCalendarServiceRoot::eventOperationFinished);
    account->saveRemoteEvent(QSL("first"), first, {{QSL("location"), written.value(QSL("location"))}}, false, false);
    QVERIFY(saved.wait(5000));
    QVERIFY2(saved.first().at(4).toBool(), qPrintable(saved.first().at(3).toString()));
    QVERIFY2(saved.first().at(3).toString().isEmpty(), qPrintable(saved.first().at(3).toString()));
    const auto stored_edit = stored();
    QCOMPARE(stored_edit.size(), 1);
    QVERIFY(stored_edit.first().m_isRead);
    QVERIFY(stored_edit.first().m_isImportant);
    QCOMPARE(CalendarEvent::fromMessage(stored_edit.first()).m_resource, written);
  }
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }

  const auto config = QDir(directory.path()).filePath(QSL(APP_CFG_PATH));
  QDir().mkpath(config);
  QSettings settings(QDir(config).filePath(QSL(APP_CFG_FILE)), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseTrayIcon, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::AutoUpdateEnabled, false);
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              directory.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-calendar-fetch-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  application.setFeedReader(new FeedReader(&application));
  FormMain window;
  TestGCalendarFetch tests;
  tests.m_view = window.tabWidget()->feedMessageViewer()->messagesView();
  return QTest::qExec(&tests, argc, argv);
}

#include "test_gcalendarfetch.moc"

