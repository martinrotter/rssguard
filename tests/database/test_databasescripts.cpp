// For license of this file, see <project-root-folder>/LICENSE.md.
// Set RSSGUARD_TEST_MARIADB_PORT (and optionally USER/PASSWORD with the same
// prefix) to also execute initialization and migrations on a disposable schema.

#include "database/mariadbdriver.h"
#include "database/sqlitedriver.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/iofactory.h"

#include <QDir>
#include <QRegularExpression>
#include <QSqlError>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

namespace {
  template <class Driver>
  class ScriptDriver : public Driver {
    public:
      using DatabaseDriver::prepareScript;
      using Driver::Driver;

      void migrate(QSqlDatabase& database, const QString& name) {
        // Exercise the shared loader without SQLite's physical-file backup.
        DatabaseDriver::updateDatabaseSchema(database, name);
      }
  };

  template <class Function>
  void withDriver(bool maria, Function function) {
    if (maria) {
      ScriptDriver<MariaDbDriver> driver;
      function(driver);
    }
    else {
      ScriptDriver<SqliteDriver> driver(QDir::tempPath());
      function(driver);
    }
  }

  struct TestConnection {
      QString name = QUuid::createUuid().toString(QUuid::Id128);
      QString schema;
      QSqlDatabase database;

      explicit TestConnection(bool maria)
        : database(QSqlDatabase::addDatabase(maria ? QSL(APP_DB_MYSQL_DRIVER) : QSL(APP_DB_SQLITE_DRIVER), name)) {
        if (maria) {
          schema = QSL("rssguard_scripts_") + name;
          database.setHostName(QSL("127.0.0.1"));
          database.setPort(qEnvironmentVariableIntValue("RSSGUARD_TEST_MARIADB_PORT"));
          database.setUserName(qEnvironmentVariable("RSSGUARD_TEST_MARIADB_USER", QSL("root")));
          database.setPassword(qEnvironmentVariable("RSSGUARD_TEST_MARIADB_PASSWORD"));
        }
        else {
          database.setDatabaseName(QSL(":memory:"));
        }
      }

      ~TestConnection() {
        if (database.isOpen() && !schema.isEmpty()) {
          QSqlQuery query(database);
          query.exec(QSL("DROP DATABASE IF EXISTS `%1`").arg(schema));
        }
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
      }
  };

  QString historicalStatement(QString statement, int version) {
    if (statement.startsWith(QSL("CREATE TABLE Categories")) && version < 101) {
      statement.remove(QRegularExpression(QSL("(?m)^\\s*custom_data[^\\n]*\\n")));
    }
    if (statement.startsWith(QSL("CREATE TABLE Feeds"))) {
      if (version < 102) {
        statement.remove(QRegularExpression(QSL("(?m)^\\s*open_articles[^\\n]*\\n")));
      }
      if (version < 104) {
        statement.remove(QRegularExpression(QSL("(?m)^\\s*exclude_global_unread[^\\n]*\\n")));
      }
    }
    if (statement.startsWith(QSL("CREATE TABLE Messages")) && version < 103) {
      statement.remove(QRegularExpression(QSL("(?m)^\\s*date_retrieved[^\\n]*\\n")));
    }
    if (statement.startsWith(QSL("CREATE INDEX idx_Messages3")) && version < 105) {
      statement.replace(QSL("feed, is_deleted, is_pdeleted, is_read"), QSL("feed, is_deleted, is_pdeleted"));
    }
    return statement;
  }

  void addEngines() {
    QTest::addColumn<bool>("maria");
    QTest::newRow("SQLite") << false;
    QTest::newRow("MariaDB") << true;
  }
} // namespace

class TestDatabaseScripts : public QObject {
    Q_OBJECT

  private slots:
    void conditionals_data();
    void conditionals();
    void invalidDirectives_data();
    void invalidDirectives();
    void generatedStatements_data();
    void generatedStatements();
    void schemaMigrations_data();
    void schemaMigrations();
};

void TestDatabaseScripts::conditionals_data() {
  addEngines();
}

void TestDatabaseScripts::conditionals() {
  QFETCH(bool, maria);
  QTemporaryDir folder;
  QVERIFY(folder.isValid());
  const QString script = QSL("  #if ENGINE == SQLITE\r\n"
                             "SELECT 'SQLite';\r\n-- !\r\n"
                             "#elif ENGINE == MARIADB\r\n"
                             "SELECT 'MariaDB';\r\n-- !\r\n"
                             "#else\r\nSELECT 'wrong';\r\n-- !\r\n#endif\r\n"
                             "#if ENGINE == SQLITE\r\n"
                             "#if ENGINE == MARIADB\r\nSELECT 'wrong nested';\r\n-- !\r\n"
                             "#else\r\nSELECT 'nested SQLite';\r\n-- !\r\n#endif\r\n"
                             "#else\r\nSELECT 'nested MariaDB';\r\n-- !\r\n#endif\r\n"
                             "  \r\n-- !\r\n"
                             "CREATE TRIGGER example BEGIN SELECT 1; SELECT 2; END;\r\n-- !\r\n"
                             "SELECT 'literal #if ENGINE == SQLITE';\r\n-- !\r\n"
                             "SELECT ##, '$$', '^^', '**', '~~', '§§1', '§§0';\r\n-- !\r\n  ");
  IOFactory::writeFile(folder.filePath(QSL("input.sql")), script.toUtf8());
  withDriver(maria, [&](auto& driver) {
    QCOMPARE(driver.humanDriverType().toUpper(), maria ? QSL("MARIADB") : QSL("SQLITE"));
    const auto statements = driver.prepareScript(folder.path(), QSL("input.sql"), QSL("test_schema"));
    QCOMPARE(statements.size(), 5);
    QCOMPARE(statements.at(0), maria ? QSL("SELECT 'MariaDB';") : QSL("SELECT 'SQLite';"));
    QCOMPARE(statements.at(1), maria ? QSL("SELECT 'nested MariaDB';") : QSL("SELECT 'nested SQLite';"));
    QCOMPARE(statements.at(2), QSL("CREATE TRIGGER example BEGIN SELECT 1; SELECT 2; END;"));
    QCOMPARE(statements.at(3), QSL("SELECT 'literal #if ENGINE == SQLITE';"));
    const QString substituted = statements.at(4);
    QVERIFY(substituted.contains(QSL("test_schema")));
    QVERIFY(substituted.contains(driver.autoIncrementPrimaryKey()));
    QVERIFY(substituted.contains(driver.blob()));
    QVERIFY(substituted.contains(driver.text()));
    QVERIFY(substituted.contains(driver.foreignKeysEnable()));
    QVERIFY(substituted.contains(driver.foreignKeysDisable()));
    for (const QString& placeholder : {QSL("##"), QSL("$$"), QSL("^^"), QSL("**"), QSL("~~"), QSL("§§1"), QSL("§§0")}) {
      QVERIFY(!substituted.contains(placeholder));
    }
    const QStringList empty_scripts{
      QString(),
      QSL(" \r\n-- !\r\n\t\r\n"),
      QSL("#if ENGINE == %1\nSELECT 'inactive';\n-- !\n#endif").arg(maria ? QSL("SQLITE") : QSL("MARIADB"))};
    for (const QString& empty_script : empty_scripts) {
      IOFactory::writeFile(folder.filePath(QSL("empty.sql")), empty_script.toUtf8());
      QVERIFY(driver.prepareScript(folder.path(), QSL("empty.sql")).isEmpty());
    }
  });
}

void TestDatabaseScripts::invalidDirectives_data() {
  QTest::addColumn<bool>("maria");
  QTest::addColumn<QString>("script");
  QTest::addColumn<int>("line");
  const QStringList invalid{QSL("#if ENGINE == UNKNOWN\n#endif"),
                            QSL("#if ENGINE != SQLITE\n#endif"),
                            QSL("#if OTHER == SQLITE\n#endif"),
                            QSL("#if ENGINE == SQLITE trailing\n#endif"),
                            QSL("#ifdef SQLITE\n#endif"),
                            QSL("#elif ENGINE == SQLITE"),
                            QSL("#else"),
                            QSL("#endif"),
                            QSL("#if ENGINE == SQLITE"),
                            QSL("#if ENGINE == SQLITE\n#else\n#else\n#endif"),
                            QSL("#if ENGINE == SQLITE\n#else\n#elif ENGINE == MARIADB\n#endif"),
                            QSL("#if ENGINE == SQLITE\n#endif trailing"),
                            QSL("#if ENGINE == SQLITE\n#else trailing\n#endif"),
                            // Invalid syntax must also fail inside a branch that is not selected.
                            QSL("#if ENGINE == SQLITE\n#if ENGINE == UNKNOWN\n#endif\n#endif")};
  const QList<int> lines{1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2, 2};
  for (bool maria : {false, true}) {
    for (int i = 0; i < invalid.size(); ++i) {
      QTest::newRow(qPrintable(QString::number(maria) + QL1C('-') + QString::number(i)))
        << maria << invalid.at(i) << lines.at(i);
    }
  }
}

void TestDatabaseScripts::invalidDirectives() {
  QFETCH(bool, maria);
  QFETCH(QString, script);
  QFETCH(int, line);
  QTemporaryDir folder;
  QVERIFY(folder.isValid());
  IOFactory::writeFile(folder.filePath(QSL("invalid.sql")), script.toUtf8());
  withDriver(maria, [&](auto& driver) {
    try {
      driver.prepareScript(folder.path(), QSL("invalid.sql"));
      QFAIL("Malformed directives were accepted.");
    }
    catch (const ApplicationException& exception) {
      QVERIFY2(exception.message().endsWith(QSL("[invalid.sql:%1]").arg(line)), qPrintable(exception.message()));
    }
  });
}

void TestDatabaseScripts::generatedStatements_data() {
  addEngines();
}

void TestDatabaseScripts::generatedStatements() {
  QFETCH(bool, maria);
  withDriver(maria, [&](auto& driver) {
    const auto initialized = driver.prepareScript(APP_SQL_PATH, QSL("db_init.sql"), QSL("test_schema"));
    QCOMPARE(initialized.size(), maria ? 30 : 29);
    QVERIFY(initialized.join(QL1C('\n')).contains(QSL("CREATE INDEX idx_Messages5 ON Messages (feed, account_id);")));
    for (int from = 100; from <= 105; ++from) {
      const auto statements =
        driver.prepareScript(APP_SQL_PATH, QSL("db_update_%1_%2.sql").arg(from).arg(from + 1), QSL("test_schema"));
      QCOMPARE(statements.size(), (from == 102 || from == 104 ? 2 : 1) + int(maria));
      const QString sql = statements.join(QL1C('\n'));
      QVERIFY(!sql.contains(QL1C('#')));
      QVERIFY(!sql.contains(QSL("!!")));
      if (maria) {
        QVERIFY(statements.first().startsWith(QSL("USE ")));
      }
      else {
        QVERIFY(!sql.contains(QSL("USE ")));
      }
      if (from == 104) {
        QCOMPARE(statements.at(int(maria)),
                 maria ? QSL("DROP INDEX idx_Messages3 ON Messages;") : QSL("DROP INDEX idx_Messages3;"));
      }
      if (from == 105) {
        const QString index = maria
                                ? QSL("CREATE INDEX /*M! IF NOT EXISTS */ idx_Messages6 ON Messages (feed, url(128));")
                                : QSL("CREATE INDEX IF NOT EXISTS idx_Messages6 ON Messages (feed, url);");
        QCOMPARE(statements.last(), index);
        QCOMPARE(initialized.last(), index);
      }
    }
  });
}

void TestDatabaseScripts::schemaMigrations_data() {
  QTest::addColumn<bool>("maria");
  QTest::addColumn<int>("version");
  for (bool maria : {false, true}) {
    for (int version : {0, 100, 101, 102, 103, 104, 105}) {
      QTest::newRow(qPrintable(QString::number(maria) + QL1C('-') + QString::number(version))) << maria << version;
    }
  }
}

void TestDatabaseScripts::schemaMigrations() {
  QFETCH(bool, maria);
  QFETCH(int, version);
  if (maria && !qEnvironmentVariableIsSet("RSSGUARD_TEST_MARIADB_PORT")) {
    QSKIP("Set RSSGUARD_TEST_MARIADB_PORT to run actual MariaDB schema migrations.");
  }
  TestConnection connection(maria);
  QVERIFY2(connection.database.open(), qPrintable(connection.database.lastError().text()));
  QSqlQuery query(connection.database);
  if (maria) {
    QVERIFY2(query.exec(QSL("CREATE DATABASE `%1` CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci")
                          .arg(connection.schema)),
             qPrintable(query.lastError().text()));
  }
  withDriver(maria, [&](auto& driver) {
    if (version != 0) {
      // Reverse just the historical schema changes so every supported start
      // version exercises the real shared migration chain and its data updates.
      for (const QString& statement : driver.prepareScript(APP_SQL_PATH, QSL("db_init.sql"), connection.schema)) {
        if (!statement.contains(QSL("idx_Messages6"))) {
          QVERIFY2(query.exec(historicalStatement(statement, version)), qPrintable(query.lastError().text()));
        }
      }
      QVERIFY(query.exec(QSL("INSERT INTO Information VALUES ('schema_version', '%1')").arg(version)));
      QVERIFY(query.exec(QSL("INSERT INTO Accounts (id, ordr, type) VALUES (1, 0, 'test')")));
      QVERIFY(query.exec(QSL("INSERT INTO Categories (id, ordr, parent_id, title, account_id) VALUES (1, 0, -1, "
                             "'category', 1)")));
      QVERIFY(query.exec(QSL("INSERT INTO Feeds (id, ordr, title, category, update_type, account_id, custom_id) "
                             "VALUES (1, 0, 'feed', -1, 0, 1, 'feed-id')")));
      const QString date_column = version >= 103 ? QSL(", date_retrieved") : QString();
      const QString date_value = version >= 103 ? QSL(", 456") : QString();
      QVERIFY2(query.exec(QSL("INSERT INTO Messages (id, feed, account_id, title, contents, date_created, "
                              "is_read, is_important, is_deleted, is_pdeleted, score%1) "
                              "VALUES (1, 1, 1, 'retained', 'body', 123, 1, 1, 1, 1, 25%2)")
                            .arg(date_column, date_value)),
               qPrintable(query.lastError().text()));
    }
    driver.migrate(connection.database, connection.schema);
    QVERIFY(query.exec(QSL("SELECT inf_value FROM Information WHERE inf_key='schema_version'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 106);
    query.finish();
    // A second call must leave the current schema and the data untouched.
    driver.migrate(connection.database, connection.schema);
    QVERIFY(query.exec(QSL("SELECT custom_data FROM Categories")));
    QVERIFY(query.exec(QSL("SELECT open_articles, exclude_global_unread FROM Feeds")));
    if (version != 0) {
      QVERIFY(query.next());
      QCOMPARE(query.value(0).toInt(), 0);
      QCOMPARE(query.value(1).toInt(), 0);
    }
    QVERIFY(query.exec(QSL("SELECT title, contents, date_created, date_retrieved, "
                           "is_read, is_important, is_deleted, is_pdeleted, score FROM Messages")));
    if (version != 0) {
      QVERIFY(query.next());
      QCOMPARE(query.value(0).toString(), QSL("retained"));
      QCOMPARE(query.value(1).toString(), QSL("body"));
      QCOMPARE(query.value(2).toInt(), 123);
      QCOMPARE(query.value(3).toInt(), version < 103 ? 123 : 456);
      for (int column = 4; column <= 7; ++column) {
        QCOMPARE(query.value(column).toInt(), 1);
      }
      QCOMPARE(query.value(8).toInt(), 25);
      QVERIFY(!query.next());
    }
    for (int index : {3, 6}) {
      const QString index_name = QSL("idx_Messages%1").arg(index);
      const QString sql = maria ? QSL("SELECT COLUMN_NAME, SUB_PART FROM information_schema.STATISTICS "
                                      "WHERE TABLE_SCHEMA='%1' AND INDEX_NAME='%2' ORDER BY SEQ_IN_INDEX")
                                    .arg(connection.schema, index_name)
                                : QSL("PRAGMA index_info(%1)").arg(index_name);
      QVERIFY2(query.exec(sql), qPrintable(query.lastError().text()));
      QStringList columns;
      while (query.next()) {
        columns.append(query.value(maria ? 0 : 2).toString());
        if (maria && index == 6 && columns.last() == QSL("url")) {
          QCOMPARE(query.value(1).toInt(), 128);
        }
      }
      QCOMPARE(columns,
               index == 3 ? QStringList({QSL("feed"), QSL("is_deleted"), QSL("is_pdeleted"), QSL("is_read")})
                          : QStringList({QSL("feed"), QSL("url")}));
    }
  });
}

QTEST_GUILESS_MAIN(TestDatabaseScripts)

#include "test_databasescripts.moc"
