// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/sqlitedriver.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QSqlDriver>
#include <QSqlError>

SqliteDriver::SqliteDriver(QObject* parent)
  : DatabaseDriver(parent), m_databaseFilePath(qApp->userDataFolder() + QDir::separator() + QSL(APP_DB_SQLITE_PATH)),
    m_databaseInitialized(false) {}

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

bool SqliteDriver::saveDatabase() {
  // Perform WAL checkpoint.
  return qApp->database()->worker()->write<bool>([&](const QSqlDatabase& db) {
    SqlQuery query_vacuum(db);

    return query_vacuum.exec(QSL("PRAGMA wal_checkpoint(TRUNCATE);"));
  });
}

bool SqliteDriver::vacuumDatabase() {
  saveDatabase();

  return qApp->database()->worker()->write<bool>([&](const QSqlDatabase& db) {
    SqlQuery query_vacuum(db);

    return query_vacuum.exec(QSL("REINDEX;"), false) && query_vacuum.exec(QSL("PRAGMA optimize;"), false) &&
           query_vacuum.exec(QSL("VACUUM;"), false);
  });
}

QString SqliteDriver::ddlFilePrefix() const {
  return QSL("sqlite");
}

QSqlDatabase SqliteDriver::connection(const QString& connection_name) {
  if (!m_databaseInitialized) {
    return initializeDatabase(connection_name);
  }
  else {
    // No need to initialize.
    QSqlDatabase database;

    if (QSqlDatabase::contains(connection_name)) {
      qDebugNN << LOGSEC_DB << "SQLite connection" << QUOTE_W_SPACE(connection_name) << "is already active.";

      // This database connection was added previously, no need to
      // setup its properties.
      database = QSqlDatabase::database(connection_name);
    }
    else {
      database = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);

      const QDir db_path(m_databaseFilePath);
      QFile db_file(db_path.absoluteFilePath(QSL(APP_DB_SQLITE_FILE)));

      database.setConnectOptions(QSL("QSQLITE_ENABLE_REGEXP"));
      database.setDatabaseName(db_file.fileName());
    }

    if (!database.isOpen() && !database.open()) {
      qFatal("SQLite database was NOT opened. Delivered error message: '%s'.", qPrintable(database.lastError().text()));
    }
    else {
      qDebugNN << LOGSEC_DB << "SQLite database connection" << QUOTE_W_SPACE(connection_name) << "to file"
               << QUOTE_W_SPACE(database.databaseName()) << "seems to be established.";
    }

    SqlQuery query_db(database);
    setPragmas(query_db);

    return database;
  }
}

bool SqliteDriver::initiateRestoration(const QString& database_package_file) {
  return IOFactory::copyFile(database_package_file,
                             m_databaseFilePath + QDir::separator() + BACKUP_NAME_DATABASE + BACKUP_SUFFIX_DATABASE);
}

bool SqliteDriver::finishRestoration() {
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
      return false;
    }
  }

  return true;
}

QSqlDatabase SqliteDriver::initializeDatabase(const QString& connection_name) {
  finishRestoration();

  QString db_file_name;

  // Prepare file paths.
  const QDir db_path(m_databaseFilePath);
  QFile db_file(db_path.absoluteFilePath(QSL(APP_DB_SQLITE_FILE)));

  // Check if database directory exists.
  if (!db_path.exists()) {
    if (!db_path.mkpath(db_path.absolutePath())) {
      // Failure when create database file path.
      qFatal("Directory '%s' for SQLite database file '%s' was NOT created."
             "This is HUGE problem.",
             qPrintable(db_path.absolutePath()),
             qPrintable(db_file.symLinkTarget()));
    }
  }

  db_file_name = db_file.fileName();

  QSqlDatabase database = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);

  database.setConnectOptions(QSL("QSQLITE_ENABLE_REGEXP"));
  database.setDatabaseName(db_file_name);

  if (!database.open()) {
    qFatal("SQLite database was NOT opened. Delivered error message: '%s'", qPrintable(database.lastError().text()));
  }
  else {
    SqlQuery query_db(database);
    setPragmas(query_db);

    // Sample query which checks for existence of tables.
    if (!query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"), false)) {
      qWarningNN << LOGSEC_DB << "SQLite database is not initialized. Initializing now.";

      try {
        const QStringList statements = prepareScript(APP_SQL_PATH, QSL(APP_DB_SQLITE_INIT));

        for (const QString& statement : statements) {
          query_db.exec(statement);
        }

        setSchemaVersion(query_db, QSL(APP_DB_SCHEMA_VERSION).toInt(), true);
      }
      catch (const ApplicationException& ex) {
        qFatal("Error when running SQL scripts: %s.", qPrintable(ex.message()));
      }

      qDebugNN << LOGSEC_DB << "SQLite database backend should be ready now.";
    }
    else {
      query_db.next();
      const int installed_db_schema = query_db.value(0).toString().toInt();

      if (installed_db_schema < QSL(APP_DB_SCHEMA_VERSION).toInt()) {
        // Now, it would be good to create backup of SQLite DB file.
        if (IOFactory::copyFile(databaseFilePath(), databaseFilePath() + QSL("-v%1.bak").arg(installed_db_schema))) {
          qDebugNN << LOGSEC_DB << "Creating backup of SQLite DB file.";
        }
        else {
          qFatal("Creation of backup SQLite DB file failed.");
        }

        try {
          updateDatabaseSchema(query_db, installed_db_schema);

          // NOTE: SQLite recommends to run ANALYZE after DB schema is updated.
          if (!query_db.exec(QSL("PRAGMA optimize"))) {
            qWarningNN << LOGSEC_DB << "Failed to ANALYZE updated DB schema.";
          }

          qDebugNN << LOGSEC_DB << "Database schema was updated from" << QUOTE_W_SPACE(installed_db_schema) << "to"
                   << QUOTE_W_SPACE(APP_DB_SCHEMA_VERSION) << "successully.";
        }
        catch (const ApplicationException& ex) {
          qFatal("Error when updating DB schema from %d: %s.", installed_db_schema, qPrintable(ex.message()));
        }
      }
      else if (installed_db_schema > QSL(APP_DB_SCHEMA_VERSION).toInt()) {
        // NOTE: We have too new database version, likely from newer
        // RSS Guard. Abort.
        qFatal("Database schema is too new. Application requires <= %d but %d is installed.",
               QSL(APP_DB_SCHEMA_VERSION).toInt(),
               installed_db_schema);
      }

      qDebugNN << LOGSEC_DB << "File-based SQLite database connection" << QUOTE_W_SPACE(connection_name) << "to file"
               << QUOTE_W_SPACE(QDir::toNativeSeparators(database.databaseName())) << "seems to be established.";
      qDebugNN << LOGSEC_DB << "File-based SQLite database has version" << QUOTE_W_SPACE_DOT(installed_db_schema);
    }
  }

  m_databaseInitialized = true;

  return database;
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
