// For license of this file, see <project-root-folder>/LICENSE.md.
//
// Set RSSGUARD_TEST_MARIADB_PORT to opt in with a local MariaDB 10.6.1+ server.
// RSSGUARD_TEST_MARIADB_USER (default root) and RSSGUARD_TEST_MARIADB_PASSWORD
// must allow creating and dropping a temporary database. Use --max-allowed-packet=64K
// on the test server to exercise the packet boundary as well.

#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/serviceroot.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

namespace {
  class TestAccount : public ServiceRoot {
    public:
      QString code() const override {
        return QSL("test-mariadb");
      }
      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  QVariant scalar(const QString& sql) {
    return qApp->database()->worker()->write<QVariant>([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.exec(sql);
      return query.next() ? query.value(0) : QVariant();
    });
  }

  int preparedExecutions() {
    return scalar(QSL("SELECT VARIABLE_VALUE FROM information_schema.SESSION_STATUS "
                      "WHERE VARIABLE_NAME='COM_STMT_EXECUTE'")).toInt();
  }

  QList<Message> articles(const QString& prefix, int count) {
    QList<Message> result;
    for (int i = 0; i < count; ++i) {
      Message message;
      message.m_title = message.m_customId = prefix + QString::number(i);
      message.m_contents = QString::fromUtf8("literal :score, 'quoted', backslash \\ and 🌲") + QChar(0) + QSL("end");
      message.m_retrieved = message.m_created = QDateTime::fromMSecsSinceEpoch(1700000000000LL + i, Qt::UTC);
      result.append(message);
    }
    return result;
  }
} // namespace

class TestMariaDbInserts : public QObject {
    Q_OBJECT

  public:
    explicit TestMariaDbInserts(Feed* feed) : m_feed(feed) {}

  private slots:
    void groupedIdsAndPartialWrite();
    void packetLimits();

  private:
    MessageUpdateReport save(QList<Message>& messages) const {
      for (auto& message : messages) {
        message.m_feedId = m_feed->id();
        message.m_accountId = m_feed->account()->accountId();
      }
      return DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), messages, m_feed, false, false);
    }

    Feed* m_feed;
};

void TestMariaDbInserts::groupedIdsAndPartialWrite() {
  scalar(QSL("SET SESSION auto_increment_increment=3"));
  scalar(QSL("SET SESSION sql_mode=CONCAT_WS(',', @@sql_mode, 'NO_BACKSLASH_ESCAPES')"));
  scalar(QSL("CREATE TRIGGER test_group BEFORE INSERT ON Messages FOR EACH ROW BEGIN "
             "IF NEW.custom_id='group-37' THEN SET NEW.id=320007; END IF; "
             "IF NEW.custom_id='group-1040' THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected failure'; "
             "END IF; END"));
  auto messages = articles(QSL("group-"), 1080);
  Label label(QSL("test label"), {}, m_feed->account()->labelsNode());
  label.setCustomId(QSL("test-label"));
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createLabel(qApp->icons(), db, &label, m_feed->account()->accountId());
  });
  messages[1040].m_assignedLabelsByFilter.append(&label);
  const int before = preparedExecutions();
  const auto failed = save(messages);
  QVERIFY(!failed.succeeded());
  QCOMPARE(failed.count(MessageWriteOutcome::Inserted), 1000);
  QCOMPARE(failed.count(MessageWriteOutcome::Failed), 80);
  QCOMPARE(failed.m_all.size(), 1000);
  QCOMPARE(failed.m_unread.size(), 1000);
  // 32 groups for the first 1,000 rows, then two groups before the injected failure.
  QCOMPARE(preparedExecutions() - before, 34);
  QCOMPARE(scalar(QSL("SELECT COUNT(*) FROM Messages")).toInt(), 1000);
  QCOMPARE(scalar(QSL("SELECT COUNT(*) FROM LabelsInMessages")).toInt(), 0);
  QCOMPARE(messages.at(37).m_id, 320007);
  QCOMPARE(messages.at(2).m_id - messages.at(1).m_id, 3);
  for (int i = 1000; i < messages.size(); ++i) {
    QCOMPARE(messages.at(i).m_id, 0);
    QCOMPARE(failed.m_results.at(i).m_id, 0);
    QVERIFY(!messages.at(i).m_insertedUpdated);
  }
  const QString retained = QSL("SELECT COUNT(*) FROM information_schema.USER_VARIABLES "
                               "WHERE VARIABLE_NAME LIKE 'rssguard%' AND VARIABLE_VALUE IS NOT NULL");
  QCOMPARE(scalar(retained).toInt(), 0);

  scalar(QSL("DROP TRIGGER test_group"));
  const auto retried = save(messages);
  QVERIFY2(retried.succeeded(), qPrintable(retried.m_error));
  QCOMPARE(retried.count(MessageWriteOutcome::Skipped), 1000);
  QCOMPARE(retried.count(MessageWriteOutcome::Inserted), 80);
  QCOMPARE(scalar(QSL("SELECT message FROM LabelsInMessages")).toInt(), messages.at(1040).m_id);
  QCOMPARE(scalar(retained).toInt(), 0);
  const auto persisted = qApp->database()->worker()->read<QHash<QString, int>>([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("SELECT custom_id, id FROM Messages"));
    QHash<QString, int> ids;
    while (query.next()) {
      ids.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return ids;
  });
  QCOMPARE(persisted.size(), messages.size());
  for (int i = 0; i < messages.size(); ++i) {
    QCOMPARE(retried.m_results.at(i).m_id, persisted.value(messages.at(i).m_customId));
  }
  QCOMPARE(scalar(QSL("SELECT contents FROM Messages WHERE id=320007")).toString(), messages.at(37).m_contents);
}

void TestMariaDbInserts::packetLimits() {
  const int packet = scalar(QSL("SELECT @@SESSION.max_allowed_packet")).toInt();
  if (packet < 32768 || packet > 1048576) {
    QSKIP("Use --max-allowed-packet=64K on the disposable server to exercise packet boundaries.");
  }
  // Each UTF-8 value fits, but a 32-row packet cannot. Include one large, still-valid single row.
  auto messages = articles(QSL("packet-"), 65);
  for (auto& message : messages) {
    message.m_contents = QString(packet / 24, QChar(0x4e00));
  }
  messages[32].m_contents = QString(packet - 2048, QL1C('x'));
  const int before = preparedExecutions();
  const auto saved = save(messages);
  const int executions = preparedExecutions() - before;
  QVERIFY2(saved.succeeded(), qPrintable(saved.m_error));
  QVERIFY(executions > 3 && executions < messages.size());
  QCOMPARE(saved.count(MessageWriteOutcome::Inserted), messages.size());
  QCOMPARE(scalar(QSL("SELECT OCTET_LENGTH(contents) FROM Messages WHERE custom_id='packet-1'")).toInt(),
           messages.at(1).m_contents.toUtf8().size());
  QCOMPARE(scalar(QSL("SELECT OCTET_LENGTH(contents) FROM Messages WHERE custom_id='packet-32'")).toInt(),
           messages.at(32).m_contents.size());

  // Earlier groups in this 1,000-row transaction must roll back when an individual row cannot fit.
  auto too_large = articles(QSL("oversize-"), 65);
  too_large[47].m_contents = QString(packet, QChar(0x4e00));
  const auto failed = save(too_large);
  QVERIFY(!failed.succeeded());
  QCOMPARE(failed.count(MessageWriteOutcome::Failed), too_large.size());
  QVERIFY(failed.m_all.isEmpty());
  QVERIFY(failed.m_unread.isEmpty());
  QCOMPARE(scalar(QSL("SELECT COUNT(*) FROM Messages WHERE custom_id LIKE 'oversize-%'")).toInt(), 0);
  for (const auto& receipt : failed.m_results) {
    QCOMPARE(receipt.m_id, 0);
  }
}

int main(int argc, char* argv[]) {
  if (!qEnvironmentVariableIsSet("RSSGUARD_TEST_MARIADB_PORT")) {
    return 77;
  }
  const int port = qEnvironmentVariableIntValue("RSSGUARD_TEST_MARIADB_PORT");
  QTemporaryDir profile;
  const QString config = profile.filePath(QSL(APP_CFG_PATH));
  if (port <= 0 || port > 65535 || !profile.isValid() || !QDir().mkpath(config)) {
    return 1;
  }
  const QString schema = QSL("rssguard_test_") + QUuid::createUuid().toString(QUuid::Id128);
  TextFactory::initializeSecretEncryptionKey(config);
  QSettings settings(QDir(config).filePath(QSL(APP_CFG_FILE)), QSettings::IniFormat);
  settings.beginGroup(GROUP(Database));
  settings.setValue(Database::ActiveDriver, QSL(APP_DB_MYSQL_DRIVER));
  settings.setValue(Database::MySQLHostname, QSL("127.0.0.1"));
  settings.setValue(Database::MySQLPort, port);
  settings.setValue(Database::MySQLUsername, qEnvironmentVariable("RSSGUARD_TEST_MARIADB_USER", QSL("root")));
  settings.setValue(Database::MySQLPassword,
                    TextFactory::encrypt(qEnvironmentVariable("RSSGUARD_TEST_MARIADB_PASSWORD")));
  settings.setValue(Database::MySQLDatabase, schema);
  settings.endGroup();
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.sync();
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              profile.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-mariadb-test"), argc, argv, arguments);
  if (scalar(QSL("SELECT DATABASE()")).toString() != schema) {
    return 1;
  }
  TestAccount account;
  Feed feed(&account);
  feed.setTitle(QSL("Test feed"));
  feed.setCustomId(QSL("test-feed"));
  application.database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, &account);
    DatabaseQueries::createOverwriteFeed(db, &feed, account.accountId(), NO_PARENT_CATEGORY);
  });
  TestMariaDbInserts test(&feed);
  const int result = QTest::qExec(&test, argc, argv);
  scalar(QSL("DROP DATABASE ") + schema);
  return result;
}

#include "test_mariadbinserts.moc"
