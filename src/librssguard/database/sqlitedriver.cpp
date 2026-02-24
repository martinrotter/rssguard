// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/sqlitedriver.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QSqlDriver>
#include <QSqlError>

SqliteDriver::SqliteDriver(QObject* parent)
  : DatabaseDriver(parent), m_databaseFilePath(qApp->userDataFolder() + QDir::separator() + QSL(APP_DB_SQLITE_PATH)) {}

QString SqliteDriver::location() const {
  return QDir::toNativeSeparators(m_databaseFilePath);
}

DatabaseDriver::DriverType SqliteDriver::driverType() const {
  return DriverType::SQLite;
}

qint64 SqliteDriver::databaseDataSize() {
  return qApp->database()->worker()->read<qint64>([&](const QSqlDatabase& db) {
    qint64 result = 1;
    SqlQuery query(db);

    if (query.exec(QSL("PRAGMA page_count;"), false)) {
      query.next();
      result *= query.value(0).value<qint64>();
    }
    else {
      return qint64(0);
    }

    if (query.exec(QSL("PRAGMA page_size;"), false)) {
      query.next();
      result *= query.value(0).value<qint64>();
    }
    else {
      return qint64(0);
    }

    return result;
  });
}

QString SqliteDriver::version() {
  return qApp->database()->worker()->read<QString>([&](const QSqlDatabase& db) {
    SqlQuery q(db);

    q.exec(QSL("SELECT sqlite_version();"));
    q.next();

    return q.value(0).toString();
  });
}

void SqliteDriver::saveDatabase() {
  // Perform WAL checkpoint.
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query_vacuum(db);

    query_vacuum.exec(QSL("PRAGMA wal_checkpoint(TRUNCATE);"));
  });
}

void SqliteDriver::vacuumDatabase() {
  saveDatabase();

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query_vacuum(db);

    query_vacuum.exec(QSL("REINDEX;"));
    query_vacuum.exec(QSL("PRAGMA optimize;"));
    query_vacuum.exec(QSL("VACUUM;"));
  });
}

QString SqliteDriver::ddlFilePrefix() const {
  return QSL("sqlite");
}

void SqliteDriver::initiateRestoration(const QString& database_package_file) {
  if (!IOFactory::copyFile(database_package_file,
                           m_databaseFilePath + QDir::separator() + BACKUP_NAME_DATABASE + BACKUP_SUFFIX_DATABASE)) {
    throw ApplicationException(tr("cannot copy backup SQLite file"));
  }
}

void SqliteDriver::finishRestoration() {
  const QString backup_database_file =
    m_databaseFilePath + QDir::separator() + BACKUP_NAME_DATABASE + BACKUP_SUFFIX_DATABASE;

  if (QFile::exists(backup_database_file)) {
    qDebugNN << LOGSEC_DB << "Backup database file '" << QDir::toNativeSeparators(backup_database_file)
             << "' was detected. Restoring it.";

    if (IOFactory::copyFile(backup_database_file, m_databaseFilePath + QDir::separator() + APP_DB_SQLITE_FILE)) {
      QFile::remove(backup_database_file);
      qDebugNN << LOGSEC_DB << "Database file was restored successully.";
    }
    else {
      qCriticalNN << LOGSEC_DB << "Database file was NOT restored due to error when copying the file.";
    }
  }
}

void SqliteDriver::beforeAddDatabase() {
  const QDir db_path(m_databaseFilePath);

  if (!db_path.exists()) {
    if (!db_path.mkpath(db_path.absolutePath())) {
      qFatal("Directory '%s' for SQLite database file was NOT created."
             "This is HUGE problem.",
             qPrintable(db_path.absolutePath()));
    }
  }
}

QString SqliteDriver::databaseName() const {
  return m_databaseFilePath + QDir::separator() + QSL(APP_DB_SQLITE_FILE);
}

void SqliteDriver::afterAddDatabase(QSqlDatabase& database, bool was_initialized) {
  Q_UNUSED(was_initialized)

  QString db_file_name = databaseName();

  database.setConnectOptions(QSL("QSQLITE_ENABLE_REGEXP"));
  database.setDatabaseName(db_file_name);
}

void SqliteDriver::updateDatabaseSchema(QSqlDatabase& db, const QString& database_name) {
  SqlQuery query_db(db);

  if (query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"), false)) {
    query_db.next();
    const int installed_db_schema = query_db.value(0).toString().toInt();

    if (installed_db_schema > QSL(APP_DB_SCHEMA_FIRST_VERSION).toInt() &&
        installed_db_schema < QSL(APP_DB_SCHEMA_VERSION).toInt()) {
      // Now, it would be good to create backup of SQLite DB file.
      if (IOFactory::copyFile(databaseFilePath(), databaseFilePath() + QSL("-v%1.bak").arg(installed_db_schema))) {
        qDebugNN << LOGSEC_DB << "Creating backup of SQLite DB file.";
      }
      else {
        qFatal("Creation of backup SQLite DB file failed.");
      }
    }
  }

  query_db.finish();

  DatabaseDriver::updateDatabaseSchema(db, database_name);

  // NOTE: SQLite recommends to run ANALYZE after DB schema is updated.
  if (!query_db.exec(QSL("PRAGMA optimize;"), false)) {
    qWarningNN << LOGSEC_DB << "Failed to ANALYZE updated DB schema.";
  }

  query_db.finish();
}

QString SqliteDriver::databaseFilePath() const {
  return m_databaseFilePath + QDir::separator() + APP_DB_SQLITE_FILE;
}

void SqliteDriver::setPragmas(SqlQuery& query) {
  query.exec(foreignKeysEnable());
  query.exec(QSL("PRAGMA encoding = 'UTF-8';"));
  query.exec(QSL("PRAGMA page_size = 32768;"));
  query.exec(QSL("PRAGMA cache_size = 32768;"));
  query.exec(QSL("PRAGMA mmap_size = 100000000;"));
  query.exec(QSL("PRAGMA synchronous = OFF;"));
  query.exec(QSL("PRAGMA temp_store = MEMORY;"));
  query.exec(QSL("PRAGMA journal_mode = WAL;"));
  query.exec(QSL("PRAGMA busy_timeout = 5000;"));
}

QString SqliteDriver::humanDriverType() const {
  return QSL("SQLite");
}

QString SqliteDriver::qtDriverCode() const {
  return QSL(APP_DB_SQLITE_DRIVER);
}

void SqliteDriver::backupDatabase(const QString& backup_folder, const QString& backup_name) {
  qDebugNN << LOGSEC_DB << "Creating SQLite DB backup.";

  saveDatabase();

  if (!IOFactory::copyFile(databaseFilePath(),
                           backup_folder + QDir::separator() + backup_name + BACKUP_SUFFIX_DATABASE)) {
    throw ApplicationException(tr("Database file not copied to output directory successfully."));
  }
}

QString SqliteDriver::autoIncrementPrimaryKey() const {
  return QSL("INTEGER PRIMARY KEY");
}

QString SqliteDriver::foreignKeysEnable() const {
  return QSL("PRAGMA foreign_keys=ON;");
}

QString SqliteDriver::foreignKeysDisable() const {
  return QSL("PRAGMA foreign_keys=OFF;");
}

QString SqliteDriver::blob() const {
  return QSL("BLOB");
}

QString SqliteDriver::text() const {
  return QSL("TEXT");
}

QString SqliteDriver::collateNocase() const {
  return QSL("COLLATE NOCASE");
}
