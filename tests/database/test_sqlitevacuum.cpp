// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settingskeys.h"

#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class TestSqliteVacuum : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void shrinksDatabaseFileWhileConnectionsRemainOpen();
    void reportsBlockedCheckpoint();
};

void TestSqliteVacuum::init() {
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("PRAGMA wal_autocheckpoint = 0;"));
    query.exec(QSL("PRAGMA busy_timeout = 0;"));
    query.exec(QSL("DROP TABLE IF EXISTS VacuumTest;"));
    query.exec(QSL("CREATE TABLE VacuumTest (id INTEGER PRIMARY KEY, contents BLOB);"));
    query.exec(QSL("WITH RECURSIVE rows(id) AS (SELECT 1 UNION ALL SELECT id + 1 FROM rows WHERE id < 240) "
                   "INSERT INTO VacuumTest SELECT id, zeroblob(24000) FROM rows;"));
    // Clear article bodies without deleting their other fields, as in issue #2504.
    query.exec(QSL("UPDATE VacuumTest SET contents = X'' WHERE id > 12;"));
  });
  qApp->database()->driver()->saveDatabase();
}

void TestSqliteVacuum::shrinksDatabaseFileWhileConnectionsRemainOpen() {
  auto* driver = qApp->database()->driver();
  const QString path = driver->databaseName();
  const qint64 size_before = QFileInfo(path).size();
  QCOMPARE(size_before, driver->databaseDataSize());

  driver->vacuumDatabase();

  const qint64 size_after = QFileInfo(path).size();
  QVERIFY(size_after < size_before);
  QCOMPARE(size_after, driver->databaseDataSize());
  QCOMPARE(QFileInfo(path + QSL("-wal")).size(), qint64(0));
  qApp->database()->worker()->read([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("SELECT COUNT(*), SUM(length(contents)) FROM VacuumTest;"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 240);
    QCOMPARE(query.value(1).toInt(), 12 * 24000);
    query.exec(QSL("PRAGMA integrity_check;"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QSL("ok"));
  });
}

void TestSqliteVacuum::reportsBlockedCheckpoint() {
  auto* driver = qApp->database()->driver();
  const QString connection_name = QSL("vacuum-test-reader");
  QSqlDatabase reader = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);
  const auto close_reader = qScopeGuard([&]() {
    reader.close();
    reader = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection_name);
  });
  reader.setDatabaseName(driver->databaseName());
  QVERIFY(reader.open());
  QVERIFY(reader.transaction());
  QSqlQuery snapshot(reader);
  QVERIFY(snapshot.exec(QSL("SELECT COUNT(*) FROM VacuumTest;")));
  QVERIFY(snapshot.next());
  snapshot.finish();

  // The snapshot allows VACUUM to commit to the WAL, but prevents its checkpoint from completing.
  QVERIFY_EXCEPTION_THROWN(driver->vacuumDatabase(), ApplicationException);
  QVERIFY(driver->databaseDataSize() < QFileInfo(driver->databaseName()).size());

  QVERIFY(reader.rollback());
  driver->vacuumDatabase();
  QCOMPARE(QFileInfo(driver->databaseName()).size(), driver->databaseDataSize());
  QCOMPARE(QFileInfo(driver->databaseName() + QSL("-wal")).size(), qint64(0));
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  const QString config = directory.filePath(QSL(APP_CFG_PATH));
  if (!directory.isValid() || !QDir().mkpath(config)) {
    return 1;
  }
  QSettings settings(QDir(config).filePath(QSL(APP_CFG_FILE)), QSettings::IniFormat);
  settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
  settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
  settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
  settings.sync();
  const QStringList arguments{QString::fromLocal8Bit(argv[0]),
                              QSL("--data"),
                              directory.path(),
                              QSL("--no-single-instance"),
                              QSL("--force-text-browser")};
  Application application(QSL("rssguard-sqlite-vacuum-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestSqliteVacuum test;
  return QTest::qExec(&test, argc, argv);
}

#include "test_sqlitevacuum.moc"
