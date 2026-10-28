// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "database/databaseworker.h"
#include "exceptions/sqlexception.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/label.h"
#include "services/abstract/serviceroot.h"

#include <cstdlib>
#include <memory>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QIcon>
#include <QLoggingCategory>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTemporaryDir>
#include <QTest>

namespace {

  constexpr int existing_per_lookup = 3000;
  constexpr int new_per_lookup = 1000;
  constexpr int lookup_kind_count = 3;
  constexpr int update_only_article_count = 10000;

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
    void updatesExistingArticles();
    void matchesCustomIdsAcrossSynchronizedAccount();
    void loadsFeedCustomDataWithoutReloading();
    void articleWriteErrorsPropagate();
    void init();
    void overwriteAndOrdinaryPolicies_data();
    void overwriteAndOrdinaryPolicies();
    void permanentDeletionWins_data();
    void permanentDeletionWins();
    void notificationsExcludeTrash_data();
    void notificationsExcludeTrash();
    void deletionDuringWriteIsNotReported();
    void guardedLabelWritesPreservePermanentDeletion();
    void forcedImportDoesNotOverwritePermanentRecord();
    void partialWriteFailureRetainsReceipts();
    void receiptsKeepInputPositions();
    void directIdLookupKeepsInputsAcrossBatches();
    void previewKeepsBodiesAndLabels_data();
    void previewKeepsBodiesAndLabels();
    void previewRespectsStoredIdentity();
    void previewRoundTripsInvalidDates();

  private:
    Message storedMessage(int id);
    QVariantList storedRow(int id);

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

  const MessageUpdateReport seeded =
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
  const MessageUpdateReport updated =
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

void TestDatabaseQueries::updatesExistingArticles() {
  QList<Message> seeded_messages;

  seeded_messages.reserve(update_only_article_count);

  for (int i = 0; i < update_only_article_count; ++i) {
    seeded_messages
      .append(makeMessage(QSL("update-only"), i, m_feed->id(), m_account->accountId(), QSL("update-only-%1").arg(i)));
  }

  const MessageUpdateReport seeded =
    DatabaseQueries::updateMessages(&m_database, &m_settings, seeded_messages, m_feed, false, true);

  QCOMPARE(int(seeded.m_all.size()), update_only_article_count);

  QList<Message> incoming_messages = seeded_messages;

  for (int i = 0; i < update_only_article_count; ++i) {
    incoming_messages[i].m_contents = QSL("update-only changed contents %1").arg(i);
  }

  QElapsedTimer timer;

  timer.start();
  const MessageUpdateReport updated =
    DatabaseQueries::updateMessages(&m_database, &m_settings, incoming_messages, m_feed, false, false);
  const qint64 elapsed_milliseconds = timer.elapsed();

  qInfo().nospace() << "updateMessages existing update batch: " << elapsed_milliseconds
                    << " ms; updated: " << update_only_article_count;

  QCOMPARE(int(updated.m_all.size()), update_only_article_count);
  QCOMPARE(int(updated.m_unread.size()), update_only_article_count);

  for (int i = 0; i < update_only_article_count; ++i) {
    QCOMPARE(incoming_messages.at(i).m_id, seeded_messages.at(i).m_id);
  }

  const int stored_articles = m_database.worker()->read<int>([](const QSqlDatabase& database) {
    return scalarQuery(database,
                       QSL("SELECT COUNT(*) FROM Messages "
                           "WHERE custom_id LIKE 'update-only-%';"));
  });
  const int changed_articles = m_database.worker()->read<int>([](const QSqlDatabase& database) {
    return scalarQuery(database,
                       QSL("SELECT COUNT(*) FROM Messages "
                           "WHERE contents LIKE 'update-only changed contents %';"));
  });

  QCOMPARE(stored_articles, update_only_article_count);
  QCOMPARE(changed_articles, update_only_article_count);
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

  const MessageUpdateReport seeded =
    DatabaseQueries::updateMessages(&m_database, &m_settings, seeded_messages, m_feed, false, true);

  QCOMPARE(int(seeded.m_all.size()), synchronized_article_count);

  QList<Message> incoming_messages = seeded_messages;

  for (Message& message : incoming_messages) {
    message.m_id = 0;
    message.m_feedId = m_secondFeed->id();
  }

  QElapsedTimer timer;

  timer.start();
  const MessageUpdateReport updated =
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

void TestDatabaseQueries::loadsFeedCustomDataWithoutReloading() {
  const QVariantHash data{{QSL("mediawiki"), QVariantHash{{QSL("version"), 1}, {QSL("cursor"), QSL("opaque token")}}},
                          {QSL("unrelated"), QSL("keep")}};
  const auto live_before = m_feed->customDatabaseData();
  m_database.worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::storeCustomData(db, QSL("Feeds"), m_feed->id(), data);
  });
  const auto loaded = m_database.worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadFeedCustomData(db, m_feed);
  });
  QCOMPARE(DatabaseQueries::serializeCustomData(loaded), DatabaseQueries::serializeCustomData(data));
  QCOMPARE(m_feed->customDatabaseData(), live_before);

  Feed missing;
  missing.setId(-123);
  QVERIFY_EXCEPTION_THROWN(m_database.worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::loadFeedCustomData(db, &missing);
  }),
                           SqlException);
}

void TestDatabaseQueries::articleWriteErrorsPropagate() {
  QList<Message> batch{
    makeMessage(QSL("write-failure"), 1, m_feed->id(), m_account->accountId(), QSL("write-failure"))};
  // Force real SQLite write failures on the writer connection, without touching other articles.
  const auto trigger = [&](const QString& operation) {
    m_database.worker()->write([&](const QSqlDatabase& db) {
      QSqlQuery query(db);
      if (!query.exec(QSL("CREATE TEMP TRIGGER test_article_write_failure BEFORE %1 ON Messages "
                          "WHEN NEW.custom_id = 'write-failure' "
                          "BEGIN SELECT RAISE(ABORT, 'simulated article failure'); END;")
                        .arg(operation))) {
        throw SqlException(query.lastError());
      }
    });
  };
  const auto drop_trigger = [&]() {
    m_database.worker()->write([](const QSqlDatabase& db) {
      QSqlQuery query(db);
      query.exec(QSL("DROP TRIGGER IF EXISTS test_article_write_failure;"));
    });
  };

  trigger(QSL("INSERT"));
  QVERIFY_EXCEPTION_THROWN(DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true)
                             .throwIfFailed(),
                           SqlException);
  drop_trigger();
  QCOMPARE(DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true).m_all.size(), 1);
  batch.first().m_title = QSL("Updated title");
  trigger(QSL("UPDATE"));
  QVERIFY_EXCEPTION_THROWN(DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, true, false)
                             .throwIfFailed(),
                           SqlException);
  drop_trigger();
}

void TestDatabaseQueries::init() {
  m_account->setSyncable(false);
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, false);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);
}

Message TestDatabaseQueries::storedMessage(int id) {
  return m_database.worker()->read<Message>([id](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT %1 FROM Messages WHERE id = :id;")
                    .arg(DatabaseQueries::messageTableAttributes().join(QSL(", "))));
    query.bindValue(QSL(":id"), id);
    query.exec();
    return query.next() ? Message::fromSqlQuery(query, {}) : Message();
  });
}

QVariantList TestDatabaseQueries::storedRow(int id) {
  return m_database.worker()->read<QVariantList>([id](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT * FROM Messages WHERE id = :id;"));
    query.bindValue(QSL(":id"), id);
    query.exec();
    QVariantList values;
    if (query.next()) {
      for (int i = 0; i < query.record().count(); ++i) {
        values.append(query.value(i));
      }
    }
    return values;
  });
}

void TestDatabaseQueries::overwriteAndOrdinaryPolicies_data() {
  QTest::addColumn<bool>("syncable");
  QTest::addColumn<bool>("ignore_body");
  QTest::addColumn<bool>("mark_unread");
  QTest::addColumn<bool>("overwrite");
  QTest::addColumn<QString>("change");
  const QStringList changes{QSL("unchanged"),
                            QSL("states"),
                            QSL("body"),
                            QSL("title"),
                            QSL("date-small"),
                            QSL("date-threshold"),
                            QSL("date-large"),
                            QSL("date-backwards"),
                            QSL("made-up-date"),
                            QSL("forced")};
  for (bool syncable : {false, true}) {
    for (bool ignore : {false, true}) {
      for (bool mark : {false, true}) {
        for (bool overwrite : {false, true}) {
          for (const QString& change : changes) {
            QTest::newRow(qPrintable(QSL("%1-sync%2-ignore%3-mark%4-overwrite%5")
                                       .arg(change)
                                       .arg(syncable)
                                       .arg(ignore)
                                       .arg(mark)
                                       .arg(overwrite)))
              << syncable << ignore << mark << overwrite << change;
          }
        }
      }
    }
  }
}

void TestDatabaseQueries::overwriteAndOrdinaryPolicies() {
  QFETCH(bool, syncable);
  QFETCH(bool, ignore_body);
  QFETCH(bool, mark_unread);
  QFETCH(bool, overwrite);
  QFETCH(QString, change);
  m_account->setSyncable(syncable);
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, ignore_body);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, mark_unread);
  const QString key = QSL("policy-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
  Message original = makeMessage(key, 0, m_feed->id(), m_account->accountId(), key);
  original.m_isRead = original.m_isImportant = true;
  original.m_score = 42;
  QList<Message> seed{original};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  Message incoming = original;
  incoming.m_overwrite = overwrite;
  incoming.m_retrieved = original.m_retrieved.addSecs(90);
  bool ordinary_update = false;
  const bool forced = change == QL1S("forced");
  if (change == QL1S("states") || forced) {
    incoming.m_isRead = incoming.m_isImportant = false;
    incoming.m_score = 0;
    ordinary_update = syncable || forced;
  }
  else if (change == QL1S("body")) {
    incoming.m_contents = QSL("New body");
    ordinary_update = !ignore_body;
  }
  else if (change == QL1S("title")) {
    incoming.m_title = QSL("New title");
    ordinary_update = true;
  }
  else if (change.startsWith(QL1S("date-")) || change == QL1S("made-up-date")) {
    qint64 delta = 1;
    if (change == QL1S("date-threshold")) {
      delta = MSG_DATETIME_DIFF_THRESSHOLD;
    }
    else if (change == QL1S("date-large")) {
      delta = MSG_DATETIME_DIFF_THRESSHOLD + 1;
    }
    else if (change == QL1S("date-backwards")) {
      delta = -MSG_DATETIME_DIFF_THRESSHOLD - 1;
    }
    else if (change == QL1S("made-up-date")) {
      delta = 60000;
    }
    incoming.m_created = original.m_created.addMSecs(delta);
    incoming.m_createdFromFeed = change != QL1S("made-up-date");
    ordinary_update = syncable || change == QL1S("date-large") || change == QL1S("date-backwards");
  }
  QList<Message> batch{incoming};
  const QVariantList before_preview = storedRow(seed.first().m_id);
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.size(), 1);
  QCOMPARE(preview.first().m_inputIndex, 0);
  QCOMPARE(preview.first().m_id, seed.first().m_id);
  QVERIFY(preview.first().m_hasStoredMessage);
  const bool preview_saved = overwrite || (forced ? syncable : ordinary_update);
  QCOMPARE(preview.first().m_decision.m_action, preview_saved ? MessageSaveAction::Update : MessageSaveAction::Skip);
  QCOMPARE(preview.first().m_decision.m_reason,
           overwrite       ? MessageSaveReason::ExplicitOverwrite
           : preview_saved ? MessageSaveReason::OrdinaryUpdate
                           : MessageSaveReason::NoUpdateTrigger);
  QCOMPARE(storedRow(seed.first().m_id), before_preview);
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, forced, false);
  QVERIFY2(updated.succeeded(), qPrintable(updated.m_error));
  const bool saved = overwrite || ordinary_update;
  QCOMPARE(updated.m_all.size(), saved ? 1 : 0);
  QCOMPARE(updated.m_results.size(), 1);
  QCOMPARE(updated.m_results.first().m_inputIndex, 0);
  QCOMPARE(updated.m_results.first().m_id, seed.first().m_id);
  QCOMPARE(updated.m_results.first().m_outcome, saved ? MessageWriteOutcome::Updated : MessageWriteOutcome::Skipped);
  QCOMPARE(batch.first().m_id, seed.first().m_id);
  const Message stored = storedMessage(seed.first().m_id);
  QCOMPARE(stored.m_title, saved ? incoming.m_title : original.m_title);
  QCOMPARE(stored.m_contents, saved ? incoming.m_contents : original.m_contents);
  QCOMPARE(stored.m_created, saved ? incoming.m_created : original.m_created);
  QCOMPARE(stored.m_retrieved, saved ? incoming.m_retrieved : original.m_retrieved);
  QCOMPARE(stored.m_score, saved ? incoming.m_score : original.m_score);
  const bool expected_read = saved && (syncable || overwrite || mark_unread) ? incoming.m_isRead : original.m_isRead;
  const bool expected_star =
    saved && (syncable || overwrite || incoming.m_isImportant) ? incoming.m_isImportant : original.m_isImportant;
  QCOMPARE(stored.m_isRead, expected_read);
  QCOMPARE(stored.m_isImportant, expected_star);
  QCOMPARE(updated.m_unread.size(), saved && !expected_read ? 1 : 0);
  if (!forced) {
    const Message& expected = preview.first().m_expectedStored;
    QCOMPARE(stored.m_title, expected.m_title);
    QCOMPARE(stored.m_author, expected.m_author);
    QCOMPARE(stored.m_url, expected.m_url);
    QCOMPARE(stored.m_contents, expected.m_contents);
    QCOMPARE(stored.m_created, expected.m_created);
    QCOMPARE(stored.m_retrieved, expected.m_retrieved);
    QCOMPARE(stored.m_isRead, expected.m_isRead);
    QCOMPARE(stored.m_isImportant, expected.m_isImportant);
    QCOMPARE(stored.m_isDeleted, expected.m_isDeleted);
    QCOMPARE(stored.m_score, expected.m_score);
  }
  QVERIFY(!stored.m_overwrite);
}

void TestDatabaseQueries::permanentDeletionWins_data() {
  QTest::addColumn<QString>("lookup");
  QTest::addColumn<bool>("syncable");
  QTest::addColumn<bool>("trashed");
  QTest::addColumn<int>("mode");
  for (const QString& lookup : {QSL("custom"), QSL("primary"), QSL("attributes")}) {
    for (bool syncable : {false, true}) {
      for (bool trashed : {false, true}) {
        for (int mode : {0, 1, 2}) {
          QTest::newRow(qPrintable(QSL("%1-sync%2-trash%3-mode%4").arg(lookup).arg(syncable).arg(trashed).arg(mode)))
            << lookup << syncable << trashed << mode;
        }
      }
    }
  }
}

void TestDatabaseQueries::permanentDeletionWins() {
  QFETCH(QString, lookup);
  QFETCH(bool, syncable);
  QFETCH(bool, trashed);
  QFETCH(int, mode);
  m_account->setSyncable(syncable);
  const QString key = QSL("permanent-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
  Message original = makeMessage(key, 0, m_feed->id(), m_account->accountId(), key);
  original.m_isDeleted = trashed;
  original.m_isRead = original.m_isImportant = true;
  original.m_score = 42;
  QList<Message> seed{original};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  const int id = seed.first().m_id;
  m_database.worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(id)});
    if (lookup != QL1S("custom")) {
      SqlQuery query(db);
      query.exec(QSL("UPDATE Messages SET custom_id = '' WHERE id = %1;").arg(id));
    }
  });
  const QVariantList before = storedRow(id);
  QVERIFY(!before.isEmpty());
  Message incoming = original;
  incoming.m_overwrite = mode == 1;
  incoming.m_isPdeleted = incoming.m_isRead = incoming.m_isImportant = false;
  incoming.m_isDeleted = !trashed;
  incoming.m_score = 0;
  incoming.m_contents = QSL("Replacement body");
  incoming.m_created = incoming.m_created.addDays(1);
  incoming.m_retrieved = incoming.m_retrieved.addDays(1);
  if (lookup == QL1S("primary")) {
    incoming.m_id = id;
  }
  else if (lookup == QL1S("attributes")) {
    incoming.m_customId.clear();
  }
  // Both label passes must be skipped, not just the article UPDATE.
  Label unused_label(QSL("Unused"), QIcon());
  incoming.m_assignedLabelsByFilter.append(&unused_label);
  incoming.m_deassignedLabelsByFilter.append(&unused_label);
  QList<Message> batch{incoming};
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.size(), 1);
  QCOMPARE(preview.first().m_id, id);
  QCOMPARE(preview.first().m_decision.m_action, MessageSaveAction::Skip);
  QCOMPARE(preview.first().m_decision.m_reason, MessageSaveReason::PermanentlyDeleted);
  QVERIFY(preview.first().m_expectedStored.m_isPdeleted);
  QVERIFY(!preview.first().m_labelsChanged);
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, mode == 2, false);
  QVERIFY2(updated.succeeded(), qPrintable(updated.m_error));
  QCOMPARE(updated.m_results.first().m_outcome, MessageWriteOutcome::Skipped);
  QCOMPARE(updated.m_results.first().m_decision.m_reason, MessageSaveReason::PermanentlyDeleted);
  QVERIFY(updated.m_all.isEmpty());
  QVERIFY(updated.m_unread.isEmpty());
  QCOMPARE(batch.first().m_id, id);
  QCOMPARE(storedRow(id), before);
}

void TestDatabaseQueries::notificationsExcludeTrash_data() {
  QTest::addColumn<bool>("new_article");
  QTest::addColumn<bool>("trashed");
  QTest::addColumn<bool>("read");
  QTest::addColumn<bool>("syncable");
  for (bool fresh : {false, true}) {
    for (bool trash : {false, true}) {
      for (bool read : {false, true}) {
        for (bool syncable : {false, true}) {
          QTest::newRow(qPrintable(QSL("new%1-trash%2-read%3-sync%4").arg(fresh).arg(trash).arg(read).arg(syncable)))
            << fresh << trash << read << syncable;
        }
      }
    }
  }
}

void TestDatabaseQueries::notificationsExcludeTrash() {
  QFETCH(bool, new_article);
  QFETCH(bool, trashed);
  QFETCH(bool, read);
  QFETCH(bool, syncable);
  m_account->setSyncable(syncable);
  const QString key = QSL("notification-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
  Message original = makeMessage(key, 0, m_feed->id(), m_account->accountId(), key);
  original.m_isDeleted = !trashed;
  QList<Message> seed{original};
  if (!new_article) {
    DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  }
  Message incoming = original;
  incoming.m_overwrite = true;
  incoming.m_isRead = read;
  incoming.m_isDeleted = trashed;
  QList<Message> batch{incoming};
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  QCOMPARE(updated.m_all.size(), 1);
  QCOMPARE(updated.m_unread.size(), !read && !trashed ? 1 : 0);
  QCOMPARE(storedMessage(batch.first().m_id).m_isDeleted, trashed);
  if (!new_article) {
    QCOMPARE(batch.first().m_id, seed.first().m_id);
  }
}

void TestDatabaseQueries::deletionDuringWriteIsNotReported() {
  QList<Message> batch{makeMessage(QSL("late-delete"), 0, m_feed->id(), m_account->accountId(), QSL("late-delete"))};
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true);
  const int id = batch.first().m_id;
  m_database.worker()->write([id](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TEMP TRIGGER delete_before_article_update BEFORE UPDATE ON Messages "
                   "WHEN OLD.id = %1 AND NEW.is_pdeleted = 0 BEGIN "
                   "UPDATE Messages SET is_pdeleted = 1 WHERE id = OLD.id; SELECT RAISE(IGNORE); END;")
                 .arg(id));
  });
  batch.first().m_title = QSL("Must not be saved");
  batch.first().m_overwrite = true;
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.first().m_decision.m_action, MessageSaveAction::Update);
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  m_database.worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER delete_before_article_update;"));
  });
  QVERIFY(updated.m_all.isEmpty());
  QVERIFY(updated.m_unread.isEmpty());
  QCOMPARE(updated.m_results.first().m_outcome, MessageWriteOutcome::Skipped);
  QCOMPARE(updated.m_results.first().m_decision.m_reason, MessageSaveReason::PermanentlyDeleted);
  const Message stored = storedMessage(id);
  QVERIFY(stored.m_isPdeleted);
  QVERIFY(stored.m_title != batch.first().m_title);
}

void TestDatabaseQueries::guardedLabelWritesPreservePermanentDeletion() {
  QList<Message> batch{makeMessage(QSL("label-guard"), 0, m_feed->id(), m_account->accountId(), QSL("label-live")),
                       makeMessage(QSL("label-guard"), 1, m_feed->id(), m_account->accountId(), QSL("label-deleted"))};
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true);
  Label label(QSL("Guarded label"), QIcon());
  label.setCustomId(QSL("guarded-label"));
  m_database.worker()->write([&](const QSqlDatabase& db) {
    SqlQuery create_label(db);
    create_label.prepare(QSL("INSERT INTO Labels (name, icon, custom_id, account_id) "
                             "VALUES ('Guarded label', '', 'guarded-label', :account_id);"));
    create_label.bindValue(QSL(":account_id"), m_account->accountId());
    create_label.exec();
    label.setId(create_label.lastInsertId().toInt());
    DatabaseQueries::assignLabelToMessages(m_database.driver(), db, &label, batch);
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(batch.last().m_id)});
    DatabaseQueries::deassignLabelFromMessages(db, &label, batch, true);
  });
  const auto has_label = [&](int id) {
    return m_database.worker()->read<int>([&](const QSqlDatabase& db) {
      return scalarQuery(db,
                         QSL("SELECT COUNT(*) FROM LabelsInMessages WHERE label = %1 AND message = %2;")
                           .arg(label.id())
                           .arg(id));
    });
  };
  QCOMPARE(has_label(batch.first().m_id), 0);
  QCOMPARE(has_label(batch.last().m_id), 1);
  m_database.worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deassignLabelFromMessages(db, &label, batch);
    DatabaseQueries::assignLabelToMessages(m_database.driver(), db, &label, batch, true);
  });
  QCOMPARE(has_label(batch.first().m_id), 1);
  QCOMPARE(has_label(batch.last().m_id), 0);
}

void TestDatabaseQueries::forcedImportDoesNotOverwritePermanentRecord() {
  QList<Message> seed{makeMessage(QSL("import"), 0, m_feed->id(), m_account->accountId(), QSL("import-permanent"))};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  const int id = seed.first().m_id;
  m_database.worker()->write([id](const QSqlDatabase& db) {
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(id)});
  });
  const auto before = storedRow(id);
  QList<Message> imported{seed.first()};
  imported.first().m_contents = QSL("Imported body");
  imported.first().m_overwrite = true;
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, imported, m_feed, false, true);
  QCOMPARE(updated.m_all.size(), 1);
  QVERIFY(imported.first().m_id != id);
  QCOMPARE(storedRow(id), before);
}
void TestDatabaseQueries::partialWriteFailureRetainsReceipts() {
  QList<Message> batch;
  for (int i = 0; i < 3; ++i) {
    batch.append(makeMessage(QSL("partial-report"),
                             i,
                             m_feed->id(),
                             m_account->accountId(),
                             QSL("partial-report-%1").arg(i)));
  }
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true).throwIfFailed();
  for (auto& message : batch) {
    message.m_title += QSL(" updated");
  }
  m_database.worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("CREATE TEMP TRIGGER fail_second_report_update BEFORE UPDATE ON Messages "
                   "WHEN OLD.custom_id = 'partial-report-1' BEGIN "
                   "SELECT RAISE(ABORT, 'second update failed'); END;"));
  });
  const auto report = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  m_database.worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DROP TRIGGER fail_second_report_update;"));
  });

  QVERIFY(!report.succeeded());
  QVERIFY(report.m_error.contains(QL1S("second update failed")));
  QVERIFY_EXCEPTION_THROWN(report.throwIfFailed(), SqlException);
  QCOMPARE(report.m_results.size(), 3);
  QCOMPARE(report.m_results.at(0).m_outcome, MessageWriteOutcome::Updated);
  QCOMPARE(report.m_results.at(1).m_outcome, MessageWriteOutcome::Failed);
  QCOMPARE(report.m_results.at(2).m_outcome, MessageWriteOutcome::NotAttempted);
  QCOMPARE(report.m_all.size(), 1);
  QCOMPARE(report.m_all.first().m_id, batch.first().m_id);
  QCOMPARE(storedMessage(batch.at(0).m_id).m_title, batch.at(0).m_title);
  QVERIFY(storedMessage(batch.at(1).m_id).m_title != batch.at(1).m_title);
  QVERIFY(storedMessage(batch.at(2).m_id).m_title != batch.at(2).m_title);
}

void TestDatabaseQueries::receiptsKeepInputPositions() {
  QList<Message> batch;
  for (int i = 0; i < 3; ++i) {
    batch.append(makeMessage(QSL("receipt-position"),
                             i,
                             m_feed->id(),
                             m_account->accountId(),
                             QSL("receipt-position-%1").arg(i)));
  }
  batch[1].m_title.clear();
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.size(), 3);
  QCOMPARE(preview.at(0).m_decision.m_action, MessageSaveAction::Insert);
  QCOMPARE(preview.at(1).m_decision.m_action, MessageSaveAction::Skip);
  QCOMPARE(preview.at(1).m_decision.m_reason, MessageSaveReason::InvalidArticle);
  QCOMPARE(preview.at(2).m_decision.m_action, MessageSaveAction::Insert);
  const auto report = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  QVERIFY2(report.succeeded(), qPrintable(report.m_error));
  QCOMPARE(report.m_results.size(), 3);
  QCOMPARE(report.m_all.size(), 2);
  QCOMPARE(report.m_results.at(0).m_outcome, MessageWriteOutcome::Inserted);
  QCOMPARE(report.m_results.at(1).m_outcome, MessageWriteOutcome::Skipped);
  QCOMPARE(report.m_results.at(1).m_decision.m_reason, MessageSaveReason::InvalidArticle);
  QCOMPARE(report.m_results.at(2).m_outcome, MessageWriteOutcome::Inserted);
  for (int i = 0; i < 3; ++i) {
    QCOMPARE(report.m_results.at(i).m_inputIndex, i);
  }
  QCOMPARE(report.m_results.at(0).m_id, batch.at(0).m_id);
  QCOMPARE(report.m_results.at(2).m_id, batch.at(2).m_id);
  QVERIFY(batch.at(0).m_id != batch.at(2).m_id);

  // Two results may refer to the same database ID; each input still needs its own receipt.
  QList<Message> duplicates{batch.first(), batch.first()};
  duplicates[0].m_title = QSL("First replacement");
  duplicates[1].m_title = QSL("Second replacement");
  const auto duplicate_report =
    DatabaseQueries::updateMessages(&m_database, &m_settings, duplicates, m_feed, false, false);
  QVERIFY2(duplicate_report.succeeded(), qPrintable(duplicate_report.m_error));
  QCOMPARE(duplicate_report.m_results.size(), 2);
  QCOMPARE(duplicate_report.m_results.at(0).m_inputIndex, 0);
  QCOMPARE(duplicate_report.m_results.at(1).m_inputIndex, 1);
  QCOMPARE(duplicate_report.m_results.at(0).m_id, duplicate_report.m_results.at(1).m_id);
  QCOMPARE(duplicate_report.count(MessageWriteOutcome::Updated), 2);
  QCOMPARE(storedMessage(batch.first().m_id).m_title, duplicates.last().m_title);
}

void TestDatabaseQueries::directIdLookupKeepsInputsAcrossBatches() {
  QList<Message> seed;
  for (int i = 0; i < 3; ++i) {
    seed.append(makeMessage(QSL("direct-batch"),
                            i,
                            m_feed->id(),
                            m_account->accountId(),
                            QSL("direct-batch-%1").arg(i)));
  }
  seed[0].m_isRead = true;
  seed[0].m_isImportant = true;
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true).throwIfFailed();
  m_database.worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(seed.last().m_id)});
  });
  const auto deleted_before = storedRow(seed.last().m_id);

  QList<Message> batch;
  for (int i = 0; i < 205; ++i) {
    Message incoming = seed.at(i % 2);
    if (i == 1 || i == 200) {
      incoming = seed.last();
      incoming.m_overwrite = true;
    }
    incoming.m_title = QSL("Direct replacement %1").arg(i);
    incoming.m_isRead = false;
    incoming.m_isImportant = false;
    if (i == 0) {
      // A valid direct ID takes precedence over a different existing custom ID.
      incoming.m_customId = seed.at(1).m_customId;
    }
    if (i == 203) {
      // A missing direct ID must not fall back to this existing custom ID.
      incoming.m_id = 2000000000;
      incoming.m_customId = seed.first().m_customId;
      incoming.m_title.clear();
    }
    batch.append(incoming);
    if (i == 50) {
      // Interleaving another lookup kind must not shift direct-ID input positions.
      Message custom_lookup = seed.at(1);
      custom_lookup.m_id = 0;
      custom_lookup.m_title = QSL("Custom replacement between direct inputs");
      batch.append(custom_lookup);
    }
  }

  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.size(), batch.size());
  QHash<int, QString> last_titles;
  for (int i = 0; i < batch.size(); ++i) {
    const auto& incoming = batch.at(i);
    const auto& result = preview.at(i);
    QCOMPARE(result.m_inputIndex, i);
    if (incoming.m_title.isEmpty()) {
      QCOMPARE(result.m_id, 0);
      QCOMPARE(result.m_decision.m_reason, MessageSaveReason::InvalidArticle);
      QVERIFY(!result.m_hasStoredMessage);
    }
    else if (incoming.m_id == seed.last().m_id) {
      QCOMPARE(result.m_id, incoming.m_id);
      QCOMPARE(result.m_decision.m_reason, MessageSaveReason::PermanentlyDeleted);
    }
    else {
      const int expected_id = incoming.m_id > 0 ? incoming.m_id : seed.at(1).m_id;
      QCOMPARE(result.m_id, expected_id);
      QCOMPARE(result.m_decision.m_action, MessageSaveAction::Update);
      QCOMPARE(result.m_expectedStored.m_isRead, expected_id == seed.first().m_id);
      QCOMPARE(result.m_expectedStored.m_isImportant, expected_id == seed.first().m_id);
      last_titles.insert(expected_id, incoming.m_title);
    }
  }

  const auto report = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  QVERIFY2(report.succeeded(), qPrintable(report.m_error));
  QCOMPARE(report.m_results.size(), batch.size());
  QCOMPARE(report.count(MessageWriteOutcome::Updated), batch.size() - 3);
  QCOMPARE(report.count(MessageWriteOutcome::Skipped), 3);
  for (int i = 0; i < batch.size(); ++i) {
    QCOMPARE(report.m_results.at(i).m_inputIndex, i);
    QCOMPARE(report.m_results.at(i).m_id, preview.at(i).m_id);
    QCOMPARE(report.m_results.at(i).m_decision.m_reason, preview.at(i).m_decision.m_reason);
  }
  for (auto expected = last_titles.constBegin(); expected != last_titles.constEnd(); ++expected) {
    QCOMPARE(storedMessage(expected.key()).m_title, expected.value());
  }
  QCOMPARE(storedMessage(seed.first().m_id).m_customId, seed.first().m_customId);
  QCOMPARE(storedRow(seed.last().m_id), deleted_before);
}

void TestDatabaseQueries::previewKeepsBodiesAndLabels_data() {
  QTest::addColumn<bool>("ignore_body");
  QTest::newRow("compare-body") << false;
  QTest::newRow("ignore-body") << true;
}

void TestDatabaseQueries::previewKeepsBodiesAndLabels() {
  QFETCH(bool, ignore_body);
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, ignore_body);
  QList<Message> seed;
  for (int i = 0; i < 2; ++i) {
    Message message = makeMessage(QSL("preview-labels"),
                                    i,
                                    m_feed->id(),
                                    m_account->accountId(),
                                    QSL("preview-labels-%1").arg(i));
    message.m_contents = QString(8192, QL1C('a'));
    seed.append(message);
  }
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true).throwIfFailed();
  Label retained(QSL("Retained preview label %1").arg(ignore_body), QIcon());
  Label removed(QSL("Removed preview label %1").arg(ignore_body), QIcon());
  retained.setCustomId(QSL("preview-retained-%1").arg(ignore_body));
  removed.setCustomId(QSL("preview-removed-%1").arg(ignore_body));
  m_database.worker()->write([&](const QSqlDatabase& db) {
    for (Label* label : {&retained, &removed}) {
      SqlQuery create_label(db);
      create_label.prepare(QSL("INSERT INTO Labels (name, icon, custom_id, account_id) "
                               "VALUES (:name, '', :custom_id, :account_id);"));
      create_label.bindValue(QSL(":name"), label->title());
      create_label.bindValue(QSL(":custom_id"), label->customId());
      create_label.bindValue(QSL(":account_id"), m_account->accountId());
      create_label.exec();
      label->setId(create_label.lastInsertId().toInt());
      DatabaseQueries::assignLabelToMessages(m_database.driver(), db, label, seed);
    }
    DatabaseQueries::permanentlyDeleteMessages(db, {QString::number(seed.last().m_id)});
  });
  const auto active_before = storedRow(seed.first().m_id);
  const auto deleted_before = storedRow(seed.last().m_id);
  QList<Message> incoming = seed;
  for (Message& message : incoming) {
    message.m_contents = QString(8192, QL1C('b'));
    message.m_assignedLabels = {&retained, &removed};
  }
  const auto unchanged_labels = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, incoming, m_feed);
  for (const auto& result : unchanged_labels) {
    QVERIFY(!result.m_labelsChanged);
    QCOMPARE(result.m_expectedStored.m_assignedLabels.size(), 2);
    QVERIFY(result.m_expectedStored.m_assignedLabels.contains(&retained));
    QVERIFY(result.m_expectedStored.m_assignedLabels.contains(&removed));
  }

  for (Message& message : incoming) {
    message.m_deassignedLabelsByFilter = {&removed};
  }
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, incoming, m_feed);
  QCOMPARE(preview.first().m_decision.m_action, ignore_body ? MessageSaveAction::Skip : MessageSaveAction::Update);
  QCOMPARE(preview.first().m_expectedStored.m_contents,
           ignore_body ? seed.first().m_contents : incoming.first().m_contents);
  QVERIFY(preview.first().m_labelsChanged);
  QCOMPARE(preview.first().m_expectedStored.m_assignedLabels, QList<Label*>({&retained}));
  QCOMPARE(preview.last().m_decision.m_reason, MessageSaveReason::PermanentlyDeleted);
  QCOMPARE(preview.last().m_expectedStored.m_contents, seed.last().m_contents);
  QVERIFY(!preview.last().m_labelsChanged);
  QCOMPARE(preview.last().m_expectedStored.m_assignedLabels.size(), 2);
  QVERIFY(preview.last().m_expectedStored.m_assignedLabels.contains(&retained));
  QVERIFY(preview.last().m_expectedStored.m_assignedLabels.contains(&removed));
  QCOMPARE(storedRow(seed.first().m_id), active_before);
  QCOMPARE(storedRow(seed.last().m_id), deleted_before);
}

void TestDatabaseQueries::previewRespectsStoredIdentity() {
  QList<Message> batch{
    makeMessage(QSL("stored-identity"), 0, m_feed->id(), m_account->accountId(), QSL("stored-custom-id"))};
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true).throwIfFailed();
  const int id = batch.first().m_id;
  batch.first().m_customId = QSL("script-custom-id");
  batch.first().m_overwrite = true;
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.first().m_id, id);
  QCOMPARE(preview.first().m_expectedStored.m_customId, QSL("stored-custom-id"));
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false).throwIfFailed();
  QCOMPARE(storedMessage(id).m_customId, preview.first().m_expectedStored.m_customId);

  m_database.worker()->write([id](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("UPDATE Messages SET custom_id = '' WHERE id = %1;").arg(id));
  });
  batch = {storedMessage(id)};
  const auto fixup_preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(fixup_preview.first().m_decision.m_action, MessageSaveAction::Skip);
  QCOMPARE(fixup_preview.first().m_expectedStored.m_customId, QString::number(id));
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false).throwIfFailed();
  QCOMPARE(storedMessage(id).m_customId, fixup_preview.first().m_expectedStored.m_customId);
}

void TestDatabaseQueries::previewRoundTripsInvalidDates() {
  QList<Message> batch{
    makeMessage(QSL("invalid-dates"), 0, m_feed->id(), m_account->accountId(), QSL("invalid-dates"))};
  DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, true).throwIfFailed();
  const int id = batch.first().m_id;
  batch.first().m_created = QDateTime();
  batch.first().m_retrieved = QDateTime();
  batch.first().m_overwrite = true;
  const auto preview = DatabaseQueries::previewMessageUpdates(&m_database, &m_settings, batch, m_feed);
  QCOMPARE(preview.first().m_decision.m_action, MessageSaveAction::Update);
  QVERIFY(!batch.first().m_created.isValid());
  QVERIFY(!batch.first().m_retrieved.isValid());
  const auto report = DatabaseQueries::updateMessages(&m_database, &m_settings, batch, m_feed, false, false);
  QVERIFY2(report.succeeded(), qPrintable(report.m_error));
  const Message stored = storedMessage(id);
  QCOMPARE(preview.first().m_expectedStored.m_created, stored.m_created);
  QCOMPARE(preview.first().m_expectedStored.m_retrieved, stored.m_retrieved);
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
