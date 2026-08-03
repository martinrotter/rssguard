// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "database/databaseworker.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceroot.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

namespace {

  constexpr int existing_per_lookup = 3000;
  constexpr int new_per_lookup = 1000;
  constexpr int lookup_kind_count = 3;

  class TestServiceRoot : public ServiceRoot {
    public:
      TestServiceRoot() : ServiceRoot(false, nullptr) {}

      virtual bool isSyncable() const override {
        return m_isSyncable;
      }

      void setSyncable(bool syncable) {
        m_isSyncable = syncable;
      }

      virtual QList<Message> obtainNewMessages(Feed* feed,
                                               const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                               const QHash<QString, QStringList>& tagged_messages) override {
        Q_UNUSED(feed)
        Q_UNUSED(stated_messages)
        Q_UNUSED(tagged_messages)
        return {};
      }

      virtual QString code() const override {
        return QSL("test");
      }

    private:
      bool m_isSyncable = false;
  };

  Message makeMessage(const QString& group, int sequence, int feed_id, int account_id, const QString& custom_id) {
    Message message;

    message.m_title = QSL("%1 title %2").arg(group).arg(sequence);
    message.m_url = QSL("https://example.com/%1/%2").arg(group).arg(sequence);
    message.m_author = QSL("%1 author %2").arg(group).arg(sequence);
    message.m_contents = QSL("%1 contents %2").arg(group).arg(sequence);
    message.m_customId = custom_id;
    message.m_feedId = feed_id;
    message.m_accountId = account_id;
    message.m_created = QDateTime::fromMSecsSinceEpoch(1700000000000LL + sequence, Qt::TimeSpec::UTC);
    message.m_retrieved = message.m_created;

    return message;
  }

  struct DatabaseState {
      int m_messageCount = -1;
      int m_updatedDirectCount = -1;
      int m_updatedCustomCount = -1;
      int m_updatedAnonymousCount = -1;
  };

  int scalarQuery(const QSqlDatabase& database, const QString& sql) {
    QSqlQuery query(database);

    if (!query.exec(sql) || !query.next()) {
      return -1;
    }

    return query.value(0).toInt();
  }

} // namespace

class TestDatabaseQueries : public QObject {
    Q_OBJECT

  public:
    explicit TestDatabaseQueries(DatabaseFactory& database, Settings& settings)
      : m_database(database), m_settings(settings) {}

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void updatesMixedArticleBatch();
    void matchesCustomIdsAcrossSynchronizedAccount();

  private:
    DatabaseFactory& m_database;
    Settings& m_settings;
    std::unique_ptr<TestServiceRoot> m_account;
    Feed* m_feed = nullptr;
    Feed* m_secondFeed = nullptr;
};

void TestDatabaseQueries::initTestCase() {
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, false);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);

  m_account.reset(new TestServiceRoot());
  m_account->setSortOrder(0);

  m_feed = new Feed(m_account.get());
  m_feed->setTitle(QSL("Database query test feed"));
  m_feed->setCustomId(QSL("database-query-test-feed"));
  m_feed->setSortOrder(0);
  m_account->appendChild(m_feed);

  m_secondFeed = new Feed(m_account.get());
  m_secondFeed->setTitle(QSL("Second database query test feed"));
  m_secondFeed->setCustomId(QSL("second-database-query-test-feed"));
  m_secondFeed->setSortOrder(1);
  m_account->appendChild(m_secondFeed);

  m_database.worker()->write([this](const QSqlDatabase& database) {
    DatabaseQueries::createOverwriteAccount(database, m_account.get());
    DatabaseQueries::createOverwriteFeed(database, m_feed, m_account->accountId(), NO_PARENT_CATEGORY);
    DatabaseQueries::createOverwriteFeed(database, m_secondFeed, m_account->accountId(), NO_PARENT_CATEGORY);
  });

  QVERIFY(m_account->accountId() > 0);
  QVERIFY(m_feed->id() > 0);
  QVERIFY(m_secondFeed->id() > 0);
}

void TestDatabaseQueries::cleanupTestCase() {
  m_account.reset();
  m_feed = nullptr;
  m_secondFeed = nullptr;
}

void TestDatabaseQueries::updatesMixedArticleBatch() {
  const int feed_id = m_feed->id();
  const int account_id = m_account->accountId();
  QList<Message> seeded_messages;

  seeded_messages.reserve(existing_per_lookup * lookup_kind_count);

  for (int i = 0; i < existing_per_lookup; ++i) {
    seeded_messages.append(makeMessage(QSL("direct"), i, feed_id, account_id, QSL("direct-%1").arg(i)));
  }

  for (int i = 0; i < existing_per_lookup; ++i) {
    seeded_messages.append(makeMessage(QSL("custom"), i, feed_id, account_id, QSL("custom-%1").arg(i)));
  }

  for (int i = 0; i < existing_per_lookup; ++i) {
    seeded_messages.append(makeMessage(QSL("anonymous"), i, feed_id, account_id, {}));
  }

  const UpdatedArticles seeded =
    DatabaseQueries::updateMessages(&m_database, &m_settings, seeded_messages, m_feed, false, true);

  QCOMPARE(int(seeded.m_all.size()), existing_per_lookup * lookup_kind_count);
  QCOMPARE(seeded.m_unread.size(), seeded.m_all.size());

  for (const Message& message : std::as_const(seeded_messages)) {
    QVERIFY(message.m_id > 0);
  }

  QList<Message> incoming_messages;

  incoming_messages.reserve((existing_per_lookup + new_per_lookup) * lookup_kind_count);

  // Existing direct IDs are updated. Missing direct IDs must fall back to insertion.
  for (int i = 0; i < existing_per_lookup; ++i) {
    Message message = seeded_messages.at(i);

    message.m_title = QSL("direct updated %1").arg(i);
    incoming_messages.append(message);
  }

  for (int i = 0; i < new_per_lookup; ++i) {
    Message message = makeMessage(QSL("new-direct"), i, feed_id, account_id, QSL("new-direct-%1").arg(i));

    message.m_id = 1000000 + i;
    incoming_messages.append(message);
  }

  // Existing custom IDs are looked up within this feed. New custom IDs are inserted.
  for (int i = 0; i < existing_per_lookup; ++i) {
    Message message = seeded_messages.at(existing_per_lookup + i);

    message.m_id = 0;
    message.m_title = QSL("custom updated %1").arg(i);
    incoming_messages.append(message);
  }

  for (int i = 0; i < new_per_lookup; ++i) {
    incoming_messages.append(makeMessage(QSL("new-custom"), i, feed_id, account_id, QSL("new-custom-%1").arg(i)));
  }

  // Articles without IDs are matched by feed, title, URL and author.
  for (int i = 0; i < existing_per_lookup; ++i) {
    Message message = seeded_messages.at((existing_per_lookup * 2) + i);

    message.m_id = 0;
    message.m_contents = QSL("anonymous updated contents %1").arg(i);
    incoming_messages.append(message);
  }

  for (int i = 0; i < new_per_lookup; ++i) {
    incoming_messages.append(makeMessage(QSL("new-anonymous"), i, feed_id, account_id, {}));
  }

  QElapsedTimer timer;

  timer.start();
  const UpdatedArticles updated =
    DatabaseQueries::updateMessages(&m_database, &m_settings, incoming_messages, m_feed, false, false);
  const qint64 elapsed_milliseconds = timer.elapsed();

  qInfo().nospace() << "updateMessages mixed batch: " << elapsed_milliseconds
                    << " ms; incoming: " << incoming_messages.size() << "; matched by DB ID: " << existing_per_lookup
                    << "; matched by custom ID: " << existing_per_lookup
                    << "; matched anonymously: " << existing_per_lookup
                    << "; inserted: " << (new_per_lookup * lookup_kind_count);

  QCOMPARE(updated.m_all.size(), incoming_messages.size());
  QCOMPARE(updated.m_unread.size(), incoming_messages.size());

  for (int i = 0; i < existing_per_lookup; ++i) {
    QCOMPARE(incoming_messages.at(i).m_id, seeded_messages.at(i).m_id);
    QCOMPARE(incoming_messages.at(existing_per_lookup + new_per_lookup + i).m_id,
             seeded_messages.at(existing_per_lookup + i).m_id);
    QCOMPARE(incoming_messages.at((existing_per_lookup + new_per_lookup) * 2 + i).m_id,
             seeded_messages.at((existing_per_lookup * 2) + i).m_id);
  }

  for (int i = 0; i < new_per_lookup; ++i) {
    const Message& formerly_missing_direct = incoming_messages.at(existing_per_lookup + i);

    QVERIFY(formerly_missing_direct.m_id > 0);
    QVERIFY(formerly_missing_direct.m_id != 1000000 + i);
  }

  for (const Message& message : std::as_const(incoming_messages)) {
    QVERIFY(message.m_id > 0);
  }

  const DatabaseState state = m_database.worker()->read<DatabaseState>([](const QSqlDatabase& database) {
    DatabaseState result;

    result.m_messageCount = scalarQuery(database, QSL("SELECT COUNT(*) FROM Messages;"));
    result.m_updatedDirectCount =
      scalarQuery(database, QSL("SELECT COUNT(*) FROM Messages WHERE title LIKE 'direct updated %';"));
    result.m_updatedCustomCount =
      scalarQuery(database, QSL("SELECT COUNT(*) FROM Messages WHERE title LIKE 'custom updated %';"));
    result.m_updatedAnonymousCount =
      scalarQuery(database, QSL("SELECT COUNT(*) FROM Messages WHERE contents LIKE 'anonymous updated contents %';"));

    return result;
  });

  QCOMPARE(state.m_messageCount, (existing_per_lookup + new_per_lookup) * lookup_kind_count);
  QCOMPARE(state.m_updatedDirectCount, existing_per_lookup);
  QCOMPARE(state.m_updatedCustomCount, existing_per_lookup);
  QCOMPARE(state.m_updatedAnonymousCount, existing_per_lookup);
}

void TestDatabaseQueries::matchesCustomIdsAcrossSynchronizedAccount() {
  constexpr int synchronized_article_count = 2500;
  QList<Message> seeded_messages;

  m_account->setSyncable(true);
  seeded_messages.reserve(synchronized_article_count);

  for (int i = 0; i < synchronized_article_count; ++i) {
    seeded_messages
      .append(makeMessage(QSL("synchronized"), i, m_feed->id(), m_account->accountId(), QSL("synchronized-%1").arg(i)));
  }

  const UpdatedArticles seeded =
    DatabaseQueries::updateMessages(&m_database, &m_settings, seeded_messages, m_feed, false, true);

  QCOMPARE(int(seeded.m_all.size()), synchronized_article_count);

  QList<Message> incoming_messages = seeded_messages;

  for (Message& message : incoming_messages) {
    message.m_id = 0;
    message.m_feedId = m_secondFeed->id();
  }

  QElapsedTimer timer;

  timer.start();
  const UpdatedArticles updated =
    DatabaseQueries::updateMessages(&m_database, &m_settings, incoming_messages, m_secondFeed, false, false);
  const qint64 elapsed_milliseconds = timer.elapsed();

  qInfo().nospace() << "updateMessages synchronized account batch: " << elapsed_milliseconds
                    << " ms; matched and moved: " << synchronized_article_count;

  QCOMPARE(int(updated.m_all.size()), synchronized_article_count);

  for (int i = 0; i < synchronized_article_count; ++i) {
    QCOMPARE(incoming_messages.at(i).m_id, seeded_messages.at(i).m_id);
  }

  const int synchronized_rows = m_database.worker()->read<int>([](const QSqlDatabase& database) {
    return scalarQuery(database, QSL("SELECT COUNT(*) FROM Messages WHERE custom_id LIKE 'synchronized-%';"));
  });
  const int moved_rows = m_database.worker()->read<int>([this](const QSqlDatabase& database) {
    return scalarQuery(database,
                       QSL("SELECT COUNT(*) FROM Messages "
                           "WHERE custom_id LIKE 'synchronized-%' AND feed = %1;")
                         .arg(m_secondFeed->id()));
  });

  QCOMPARE(synchronized_rows, synchronized_article_count);
  QCOMPARE(moved_rows, synchronized_article_count);
  m_account->setSyncable(false);
}

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir data_directory;

  if (!data_directory.isValid() || !QDir().mkpath(data_directory.filePath(QSL("config")))) {
    return EXIT_FAILURE;
  }

  QSettings::setDefaultFormat(QSettings::Format::IniFormat);
  QCoreApplication::setApplicationName(QSL(APP_NAME));
  QCoreApplication::setApplicationVersion(QSL(APP_VERSION));
  QCoreApplication::setOrganizationDomain(QSL(APP_URL));

  int result;

  {
    Settings settings(data_directory.filePath(QSL("config/config.ini")),
                      QSettings::Format::IniFormat,
                      SettingsProperties::SettingsType::Custom);

    TextFactory::initializeSecretEncryptionKey(settings.pathName());

    DatabaseFactory database(APP_DB_SQLITE_DRIVER, data_directory.path());

    // Keep timing output visible while avoiding thousands of per-query debug messages.
    QLoggingCategory::setFilterRules(QSL("*.debug=false"));

    TestDatabaseQueries test(database, settings);

    result = QTest::qExec(&test, argc, argv);
  }

  // DatabaseFactory owns the worker, so its destruction must precede removal of named connections.
  const QStringList connection_names = QSqlDatabase::connectionNames();

  for (const QString& connection_name : connection_names) {
    QSqlDatabase::removeDatabase(connection_name);
  }

  return result;
}

#include "test_databasequeries.moc"
