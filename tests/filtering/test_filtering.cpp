// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "database/databaseworker.h"
#include "filtering/filteringsystem.h"
#include "miscellaneous/applicationpaths.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/serviceroot.h"

#include <cstdlib>
#include <memory>

#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace {

  class TestServiceRoot : public ServiceRoot {
    public:
      TestServiceRoot() : ServiceRoot(false, nullptr) {}

      bool isSyncable() const override {
        return false;
      }
      QString code() const override {
        return QSL("filter-test");
      }

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  void setScript(MessageFilter& filter, const QString& body) {
    filter.setName(QSL("Test filter"));
    filter.setScript(QSL("function filterMessage() { %1; return Msg.Accept; }").arg(body));
  }

} // namespace

class TestFiltering : public QObject {
    Q_OBJECT

  public:
    TestFiltering(DatabaseFactory& database, Settings& settings)
      : m_database(database), m_settings(settings), m_paths(&settings), m_localization(&settings) {}

  private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void ordinaryWritesDoNotRequestOverwrite();
    void overwriteSurvivesChainAndCanBeCancelled();
    void detachedLookupCannotOverwriteIncomingArticle();
    void overwriteReachesDatabase_data();
    void overwriteReachesDatabase();
    void workflowsShareSavePolicy_data();
    void workflowsShareSavePolicy();
    void rejectedOverwriteDoesNotSave_data();
    void rejectedOverwriteDoesNotSave();

  private:
    Message message(const QString& id) const;
    Message storedMessage(int id) const;
    std::unique_ptr<FilteringSystem> system(FilteringSystem::FiteringUseCase mode =
                                              FilteringSystem::FiteringUseCase::NewArticles);

    DatabaseFactory& m_database;
    Settings& m_settings;
    ApplicationPaths m_paths;
    Localization m_localization;
    std::unique_ptr<TestServiceRoot> m_account;
    Feed* m_feed = nullptr;
};

void TestFiltering::initTestCase() {
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, true);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);

  m_account = std::make_unique<TestServiceRoot>();
  m_account->setSortOrder(0);
  m_feed = new Feed(m_account.get());
  m_feed->setTitle(QSL("Filtering test feed"));
  m_feed->setCustomId(QSL("filtering-test-feed"));
  m_feed->setSortOrder(0);
  m_account->appendChild(m_feed);

  m_database.worker()->write([this](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, m_account.get());
    DatabaseQueries::createOverwriteFeed(db, m_feed, m_account->accountId(), NO_PARENT_CATEGORY);
  });
  QVERIFY(m_account->accountId() > 0);
  QVERIFY(m_feed->id() > 0);
}

void TestFiltering::cleanupTestCase() {
  m_account.reset();
  m_feed = nullptr;
}

void TestFiltering::init() {
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, true);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);
}

Message TestFiltering::message(const QString& id) const {
  Message result;
  result.m_title = QSL("Article %1").arg(id);
  result.m_customId = id;
  result.m_url = QSL("https://example.com/%1").arg(id);
  result.m_author = QSL("Author");
  result.m_contents = QSL("Original body");
  result.m_created = QDateTime::fromMSecsSinceEpoch(1700000000000LL, Qt::UTC);
  result.m_retrieved = result.m_created;
  result.m_feedId = m_feed->id();
  result.m_accountId = m_account->accountId();
  result.m_enclosures.append(QSharedPointer<MessageEnclosure>::create(QSL("https://example.com/audio.mp3"),
                                                                      QSL("audio/mpeg")));
  return result;
}

Message TestFiltering::storedMessage(int id) const {
  return m_database.worker()->read<Message>([id](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT %1 FROM Messages WHERE id = :id;")
                    .arg(DatabaseQueries::messageTableAttributes().join(QSL(", "))));
    query.bindValue(QSL(":id"), id);
    query.exec();
    return query.next() ? Message::fromSqlQuery(query, {}) : Message();
  });
}

std::unique_ptr<FilteringSystem> TestFiltering::system(FilteringSystem::FiteringUseCase mode) {
  return std::make_unique<FilteringSystem>(mode,
                                           m_feed,
                                           m_account.get(),
                                           &m_paths,
                                           &m_database,
                                           nullptr,
                                           &m_localization,
                                           nullptr);
}

void TestFiltering::ordinaryWritesDoNotRequestOverwrite() {
  Message incoming = message(QSL("ordinary-writes"));
  auto filtering = system();
  filtering->setMessage(&incoming);
  MessageFilter filter;
  setScript(filter,
            QSL("msg.isRead = true; msg.isImportant = false; msg.isDeleted = true;"
                "msg.score = 20; msg.title = 'Changed'; msg.contents = 'Body';"
                "msg.enclosures[0].url = 'https://example.com/changed';"
                "msg.removeAllEnclosures(); msg.deassignAllLabels()"));
  QCOMPARE(filtering->filterMessage(filter), FilterMessage::FilteringAction::Accept);
  QVERIFY(incoming.m_isRead);
  QVERIFY(incoming.m_isDeleted);
  QVERIFY(!incoming.m_overwrite);
  QVERIFY(incoming.m_enclosures.isEmpty());
  const int property = FilterMessage::staticMetaObject.indexOfProperty("overwrite");
  QVERIFY(property >= 0);
  QVERIFY(FilterMessage::staticMetaObject.property(property).isWritable());
}

void TestFiltering::overwriteSurvivesChainAndCanBeCancelled() {
  Message first = message(QSL("chain"));
  Message next = message(QSL("next"));
  auto filtering = system();
  MessageFilter enable, inspect, disable;
  setScript(enable, QSL("msg.overwrite = true"));
  setScript(inspect, QSL("if (!msg.overwrite) throw Error('Flag lost'); msg.isRead = true"));
  setScript(disable, QSL("msg.overwrite = false"));
  filtering->setMessage(&first);
  filtering->filterMessage(enable);
  filtering->filterMessage(inspect);
  QVERIFY(first.m_overwrite);
  filtering->filterMessage(disable);
  QVERIFY(!first.m_overwrite);
  filtering->filterMessage(enable);
  filtering->setMessage(nullptr);
  QVERIFY(first.m_overwrite);
  next.m_overwrite = true;
  filtering->setMessage(&next);
  QVERIFY(!next.m_overwrite);
  QVERIFY(first.m_overwrite);
}

void TestFiltering::detachedLookupCannotOverwriteIncomingArticle() {
  const QString id = QSL("detached");
  QList<Message> seed{message(id)};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  Message incoming = message(id);
  auto filtering = system();
  filtering->setMessage(&incoming);
  MessageFilter filter;
  setScript(filter,
            QSL("const previous = msg.getArticleFromDatabase(Msg.SameCustomId);"
                "if (previous === null) throw Error('Missing stored article');"
                "previous.overwrite = true; previous.isRead = true"));
  filtering->filterMessage(filter);
  QVERIFY(!incoming.m_overwrite);
  QVERIFY(!incoming.m_isRead);
  QVERIFY(!storedMessage(seed.first().m_id).m_isRead);
}

void TestFiltering::overwriteReachesDatabase_data() {
  QTest::addColumn<bool>("overwrite");
  QTest::addColumn<bool>("manual");
  QTest::addColumn<bool>("new_article");
  for (bool overwrite : {false, true}) {
    for (bool manual : {false, true}) {
      for (bool new_article : {false, true}) {
        if (manual && new_article) {
          continue;
        }
        QTest::newRow(qPrintable(QSL("overwrite%1-manual%2-new%3").arg(overwrite).arg(manual).arg(new_article)))
          << overwrite << manual << new_article;
      }
    }
  }
}

void TestFiltering::overwriteReachesDatabase() {
  QFETCH(bool, overwrite);
  QFETCH(bool, manual);
  QFETCH(bool, new_article);
  const QString id = QString::fromLatin1(QTest::currentDataTag());
  Message original = message(id);
  original.m_isImportant = true;
  QList<Message> seed{original};
  if (!new_article) {
    DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  }
  QList<Message> incoming{manual ? storedMessage(seed.first().m_id) : message(id)};
  auto filtering =
    system(manual ? FilteringSystem::FiteringUseCase::ExistingArticles : FilteringSystem::FiteringUseCase::NewArticles);
  filtering->setMessage(&incoming.first());
  MessageFilter filter;
  setScript(filter,
            QSL("msg.isRead = true; msg.isImportant = false; msg.score = 9;"
                "msg.enclosures[0].mimeType = 'audio/ogg'; msg.overwrite = %1")
              .arg(overwrite ? QSL("true") : QSL("false")));
  QCOMPARE(filtering->filterMessage(filter), FilterMessage::FilteringAction::Accept);
  filtering->setMessage(nullptr);
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, incoming, m_feed, false, false);
  const bool saved = new_article || overwrite;
  QCOMPARE(updated.m_all.size(), saved ? 1 : 0);
  const Message stored = storedMessage(incoming.first().m_id);
  QCOMPARE(stored.m_isRead, saved);
  QCOMPARE(stored.m_isImportant, !new_article && !overwrite);
  QCOMPARE(stored.m_score, saved ? 9.0 : 0.0);
  QCOMPARE(stored.m_enclosures.first()->mimeType(), saved ? QSL("audio/ogg") : QSL("audio/mpeg"));
  QVERIFY(!stored.m_overwrite);
}

void TestFiltering::workflowsShareSavePolicy_data() {
  QTest::addColumn<bool>("manual");
  QTest::addColumn<bool>("overwrite");
  QTest::addColumn<bool>("ignore_body");
  QTest::addColumn<bool>("mark_unread");
  QTest::addColumn<QString>("change");
  const QStringList changes{QSL("accept"), QSL("states"), QSL("body"), QSL("title-read"), QSL("body-read")};
  for (bool manual : {false, true}) {
    for (bool overwrite : {false, true}) {
      for (bool ignore_body : {false, true}) {
        for (bool mark_unread : {false, true}) {
          for (const QString& change : changes) {
            QTest::newRow(qPrintable(QSL("%1-manual%2-overwrite%3-ignore%4-mark%5")
                                       .arg(change)
                                       .arg(manual)
                                       .arg(overwrite)
                                       .arg(ignore_body)
                                       .arg(mark_unread)))
              << manual << overwrite << ignore_body << mark_unread << change;
          }
        }
      }
    }
  }
}

void TestFiltering::workflowsShareSavePolicy() {
  QFETCH(bool, manual);
  QFETCH(bool, overwrite);
  QFETCH(bool, ignore_body);
  QFETCH(bool, mark_unread);
  QFETCH(QString, change);
  m_settings.setValue(GROUP(Messages), Messages::IgnoreContentsChanges, ignore_body);
  m_settings.setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, mark_unread);

  const QString id = QSL("shared-policy-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
  const Message original = message(id);
  QList<Message> seed{original};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  QList<Message> incoming{manual ? storedMessage(seed.first().m_id) : message(id)};
  auto filtering =
    system(manual ? FilteringSystem::FiteringUseCase::ExistingArticles : FilteringSystem::FiteringUseCase::NewArticles);
  filtering->setMessage(&incoming.first());

  const bool body_changed = change.startsWith(QL1S("body"));
  const bool title_changed = change == QL1S("title-read");
  const bool states_changed = change == QL1S("states") || change.endsWith(QL1S("-read"));
  QString script;
  if (body_changed) {
    script += QSL("msg.contents = 'Replacement body';");
  }
  if (title_changed) {
    script += QSL("msg.title = 'Replacement title';");
  }
  if (states_changed) {
    script += QSL("msg.isRead = true; msg.score = 17;");
  }
  script += QSL("msg.overwrite = %1;").arg(overwrite ? QSL("true") : QSL("false"));
  MessageFilter filter;
  setScript(filter, script);
  QCOMPARE(filtering->filterMessage(filter), FilterMessage::FilteringAction::Accept);
  filtering->setMessage(nullptr);

  // Both callers now enter persistence without workflow-specific overrides.
  const auto updated = DatabaseQueries::updateMessages(&m_database, &m_settings, incoming, m_feed, false, false);
  const bool saved = overwrite || title_changed || (body_changed && !ignore_body);
  QCOMPARE(updated.m_all.size(), saved ? 1 : 0);
  QCOMPARE(incoming.first().m_id, seed.first().m_id);
  const Message stored = storedMessage(seed.first().m_id);
  QCOMPARE(stored.m_isRead, saved && states_changed && (overwrite || mark_unread));
  QCOMPARE(stored.m_score, saved && states_changed ? 17.0 : 0.0);
  QCOMPARE(stored.m_contents, saved && body_changed ? QSL("Replacement body") : original.m_contents);
  QCOMPARE(stored.m_title, saved && title_changed ? QSL("Replacement title") : original.m_title);
  QCOMPARE(updated.m_unread.size(), saved && !stored.m_isRead ? 1 : 0);
}

void TestFiltering::rejectedOverwriteDoesNotSave_data() {
  QTest::addColumn<QString>("decision");
  QTest::newRow("ignore") << QSL("Ignore");
  QTest::newRow("purge-during-fetch") << QSL("Purge");
}

void TestFiltering::rejectedOverwriteDoesNotSave() {
  QFETCH(QString, decision);
  const QString id = QSL("rejected-%1").arg(decision);
  QList<Message> seed{message(id)};
  DatabaseQueries::updateMessages(&m_database, &m_settings, seed, m_feed, false, true);
  QList<Message> incoming{message(id)};
  auto filtering = system();
  filtering->setMessage(&incoming.first());
  MessageFilter filter;
  filter.setName(id);
  filter.setScript(QSL("function filterMessage() { msg.isRead = true; msg.overwrite = true; return Msg.%1; }")
                     .arg(decision));
  const auto result = filtering->filterMessage(filter);
  QVERIFY(result != FilterMessage::FilteringAction::Accept);
  filtering->setMessage(nullptr);
  // FeedDownloader removes rejected articles before calling persistence.
  incoming.clear();
  QVERIFY(DatabaseQueries::updateMessages(&m_database, &m_settings, incoming, m_feed, false, false).m_all.isEmpty());
  QVERIFY(!storedMessage(seed.first().m_id).m_isRead);
}
int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTemporaryDir data_directory;
  if (!data_directory.isValid() || !QDir().mkpath(data_directory.filePath(QSL("config")))) {
    return EXIT_FAILURE;
  }
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QCoreApplication::setApplicationName(QSL(APP_NAME));
  QCoreApplication::setApplicationVersion(QSL(APP_VERSION));
  QCoreApplication::setOrganizationDomain(QSL(APP_URL));
  int result;
  {
    Settings settings(data_directory.filePath(QSL("config/config.ini")),
                      QSettings::IniFormat,
                      SettingsProperties::SettingsType::Custom);
    TextFactory::initializeSecretEncryptionKey(settings.pathName());
    DatabaseFactory database(APP_DB_SQLITE_DRIVER, data_directory.path());
    QLoggingCategory::setFilterRules(QSL("*.debug=false"));
    TestFiltering test(database, settings);
    result = QTest::qExec(&test, argc, argv);
  }
  for (const QString& connection : QSqlDatabase::connectionNames()) {
    QSqlDatabase::removeDatabase(connection);
  }
  return result;
}

#include "test_filtering.moc"
