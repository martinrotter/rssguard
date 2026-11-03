// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "services/abstract/serviceentrypoint.h"

#include <librssguard-standard/src/standardfeed.h>

#include <QDir>
#include <QPluginLoader>
#include <QSettings>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

namespace {

  // Seed an account and feed, then reopen them through the real standard plugin.
  class SeedAccount : public ServiceRoot {
    public:
      QString code() const override { return QSL(SERVICE_CODE_STD_RSS); }
      QList<Message> obtainNewMessages(Feed*, const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override { return {}; }
  };

} // namespace

class TestFeedValidators : public QObject {
    Q_OBJECT

  private slots:
    void commitOnlyAfterSuccessfulStorage();
};

void TestFeedValidators::commitOnlyAfterSuccessfulStorage() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QString source = QSL("http://127.0.0.1:%1/feed").arg(server.serverPort());
  QByteArray etag = "\"one\"";
  const QByteArray valid_body = "<rss version=\"2.0\"><channel><title>Test</title><item>"
                                "<title>Article</title><guid>id-1</guid></item></channel></rss>";
  QByteArray body = valid_body;
  bool redirect_metadata = false;
  QList<QByteArray> requests;
  connect(&server, &QTcpServer::newConnection, this, [&]() {
    QTcpSocket* socket = server.nextPendingConnection();
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    connect(socket, &QTcpSocket::readyRead, socket, [&, socket, request = QByteArray()]() mutable {
      request += socket->readAll();
      if (!request.contains("\r\n\r\n")) {
        return;
      }
      if (redirect_metadata && request.startsWith("GET /feed HTTP/")) {
        socket->write("HTTP/1.1 301 Moved Permanently\r\nLocation: /new-feed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        socket->disconnectFromHost();
        return;
      }
      QByteArray validator;
      for (const QByteArray& line : request.split('\n')) {
        if (line.toLower().startsWith("if-none-match:")) {
          validator = line.mid(line.indexOf(':') + 1).trimmed();
        }
      }
      requests.append(validator);
      if (!etag.isEmpty() && validator == etag) {
        socket->write("HTTP/1.1 304 Not Modified\r\nConnection: close\r\n\r\n");
      }
      else {
        const QByteArray header = etag.isEmpty() ? QByteArray() : "ETag: " + etag + "\r\n";
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/rss+xml\r\n" + header +
                      "Content-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
      }
      socket->disconnectFromHost();
    });
  });

  QPluginLoader loader(QString::fromUtf8(TEST_STANDARD_PLUGIN));
  auto* plugin = qobject_cast<ServiceEntryPoint*>(loader.instance());
  QVERIFY2(plugin, qPrintable(loader.errorString()));
  SeedAccount seed;
  seed.setTitle(QSL("Validator account"));
  Feed seed_feed(&seed);
  seed_feed.setCustomId(QSL("feed"));
  seed_feed.setTitle(QSL("Validator test"));
  seed_feed.setSource(source);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, &seed);
    DatabaseQueries::createOverwriteFeed(db, &seed_feed, seed.accountId(), NO_PARENT_CATEGORY);
  });
  const auto roots = plugin->initializeSubtree();
  QCOMPARE(roots.size(), 1);
  std::unique_ptr<ServiceRoot> account(roots.first());
  account->start(false);
  QCOMPARE(account->getSubTreeFeeds().size(), 1);
  Feed* feed = account->getSubTreeFeeds().first();
  account->setNetworkProxy(QNetworkProxy::NoProxy);
  auto config = feed->customDatabaseData();
  config[QSL("type")] = int(StandardFeed::Type::Rss2X);
  config[QSL("use_account_proxy")] = true;
  config[QSL("encoding")] = QSL("UTF-8");
  feed->setCustomDatabaseData(config);

  const auto refresh = [&]() {
    QSignalSpy finished(qApp->feedReader(), &FeedReader::feedUpdatesFinished);
    QSignalSpy unlocked(qApp->feedUpdateLock(), &Mutex::unlocked);
    qApp->feedReader()->updateFeeds({feed}, true);
    return (!finished.isEmpty() || finished.wait(10000)) && (!unlocked.isEmpty() || unlocked.wait(10000));
  };
  const auto stored = [&]() {
    return qApp->database()->worker()->read<int>([](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(QSL("SELECT COUNT(*) FROM Messages;"));
      return query.next() ? query.value(0).toInt() : -1;
    });
  };
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TEMP TRIGGER reject_feed_item BEFORE INSERT ON Messages "
                   "BEGIN SELECT RAISE(ABORT, 'controlled write failure'); END;"));
  });
  QVERIFY(refresh());
  QCOMPARE(feed->status(), Feed::Status::SqlError);
  QCOMPARE(stored(), 0);
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER reject_feed_item;"));
  });
  QVERIFY(refresh());
  QCOMPARE(requests.last(), QByteArray());
  QCOMPARE(stored(), 1);

  // Both 304s omit the ETag. The client must keep sending the committed validator.
  QVERIFY(refresh());
  QVERIFY(refresh());
  QCOMPARE(requests, QList<QByteArray>({{}, {}, etag, etag}));

  const QByteArray previous_etag = etag;
  etag = "\"two\"";
  body = "<broken";
  QVERIFY(refresh());
  QCOMPARE(feed->status(), Feed::Status::ParsingError);
  QCOMPARE(requests.last(), previous_etag);
  body = valid_body;
  body.replace("id-1", "id-2");
  QVERIFY(refresh());
  QCOMPARE(requests.last(), previous_etag);
  QCOMPARE(stored(), 2);
  QVERIFY(refresh());
  QCOMPARE(requests.last(), etag);

  etag.clear();
  QVERIFY(refresh()); // A successful 200 without ETag clears the prior validator.
  QVERIFY(refresh());
  QCOMPARE(requests.last(), QByteArray());

  // Resetting a feed between retrieval and commit invalidates its pending validator.
  etag = "\"new\"";
  const auto pending = account->obtainNewMessagesForUpdate(feed, {}, {});
  QVERIFY(pending.afterMessagesStored);
  account->onDatabaseCleanup();
  pending.afterMessagesStored();
  QVERIFY(refresh());
  QCOMPARE(requests.last(), QByteArray());

  // Batch metadata refresh may replace a feed's source with the redirect target.
  redirect_metadata = true;
  QVERIFY(QMetaObject::invokeMethod(feed, "fetchMetadataForItself", Qt::DirectConnection));
  QVERIFY(feed->source().endsWith(QL1S("/new-feed")));
  QVERIFY(refresh());
  QCOMPARE(requests.last(), QByteArray());
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  const QString config = directory.path() + QDir::separator() + QSL(APP_CFG_PATH);
  QDir().mkpath(config);
  QSettings settings(config + QDir::separator() + QSL(APP_CFG_FILE), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(Feeds) + QSL("/") + Feeds::FeedsUpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]), QSL("--data"), directory.path(),
                              QSL("--no-single-instance"), QSL("--force-text-browser")};
  Application application(QSL("rssguard-validator-test"), argc, argv, arguments);
  auto* reader = new FeedReader(&application);
  application.setFeedReader(reader);
  // A scheduled refresh consults the main window. This fixture drives every refresh explicitly.
  for (auto* timer : reader->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
    timer->stop();
  }
  QObject::disconnect(reader, nullptr, nullptr, nullptr); // No frontend notifications in this test.
  TestFeedValidators test;
  const int result = QTest::qExec(&test, argc, argv);
  reader->quit();
  return result;
}

#include "test_feedvalidators.moc"
