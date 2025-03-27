// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/sqlitedriver.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

SqliteDriver::SqliteDriver(bool in_memory, QObject* parent)
  : DatabaseDriver(parent), m_inMemoryDatabase(in_memory),
    m_databaseFilePath(qApp->userDataFolder() + QDir::separator() + QSL(APP_DB_SQLITE_PATH)),
    m_fileBasedDatabaseInitialized(false), m_inMemoryDatabaseInitialized(false) {}

QString SqliteDriver::location() const {
  return QDir::toNativeSeparators(m_databaseFilePath);
}

DatabaseDriver::DriverType SqliteDriver::driverType() const {
  return DriverType::SQLite;
}

bool SqliteDriver::vacuumDatabase() {
  QSqlDatabase database;

  saveDatabase();
  database = connection(objectName(), DatabaseDriver::DesiredStorageType::StrictlyFileBased);

  QSqlQuery query_vacuum(database);

  return query_vacuum.exec(QSL("VACUUM"));
}

QString SqliteDriver::ddlFilePrefix() const {
  return QSL("sqlite");
}

int SqliteDriver::loadOrSaveDbInMemoryDb(sqlite3* in_memory_db, const char* db_filename, bool save) {
  int rc;                   /* Function return code */
  sqlite3* p_file;          /* Database connection opened on zFilename */
  sqlite3_backup* p_backup; /* Backup object used to copy data */
  sqlite3* p_to;            /* Database to copy to (pFile or pInMemory) */
  sqlite3* p_from;          /* Database to copy from (pFile or pInMemory) */

  /* Open the database file identified by zFilename. Exit early if this fails
  ** for any reason. */
  rc = sqlite3_open(db_filename, &p_file);

  if (rc == SQLITE_OK) {
    /* If this is a 'load' operation (isSave==0), then data is copied
    ** from the database file just opened to database pInMemory.
    ** Otherwise, if this is a 'save' operation (isSave==1), then data
    ** is copied from pInMemory to pFile.  Set the variables pFrom and
    ** pTo accordingly. */
    p_from = (save ? in_memory_db : p_file);
    p_to = (save ? p_file : in_memory_db);

    /* Set up the backup procedure to copy from the "main" database of
    ** connection pFile to the main database of connection pInMemory.
    ** If something goes wrong, pBackup will be set to NULL and an error
    ** code and message left in connection pTo.
    **
    ** If the backup object is successfully created, call backup_step()
    ** to copy data from pFile to pInMemory. Then call backup_finish()
    ** to release resources associated with the pBackup object.  If an
    ** error occurred, then an error code and message will be left in
    ** connection pTo. If no error occurred, then the error code belonging
    ** to pTo is set to SQLITE_OK.
    */
    p_backup = sqlite3_backup_init(p_to, "main", p_from, "main");
    if (p_backup) {
      (void)sqlite3_backup_step(p_backup, -1);
      (void)sqlite3_backup_finish(p_backup);
    }
    rc = sqlite3_errcode(p_to);
  }

  sqlite3_db_cacheflush(p_file);

  /* Close the database connection opened on database file zFilename
  ** and return the result of this function. */
  (void)sqlite3_close(p_file);
  return rc;
}

bool SqliteDriver::saveDatabase() {
  if (!m_inMemoryDatabase) {
    return true;
  }
  else {
    qDebugNN << LOGSEC_DB << "Saving in-memory working database back to persistent file-based storage.";

    QSqlDatabase database = connection(QSL("SaveFromMemory"), DatabaseDriver::DesiredStorageType::StrictlyInMemory);
    const QDir db_path(m_databaseFilePath);
    QFile db_file(db_path.absoluteFilePath(QSL(APP_DB_SQLITE_FILE)));
    QVariant v = database.driver()->handle();

    if (v.isValid() && (qstrcmp(v.typeName(), "sqlite3*") == 0)) {
      // v.data() returns a pointer to the handle
      sqlite3* handle = *static_cast<sqlite3**>(v.data());

      if (handle != nullptr) {
        loadOrSaveDbInMemoryDb(handle, QDir::toNativeSeparators(db_file.fileName()).toStdString().c_str(), 1);
      }
      else {
        throw ApplicationException(tr("cannot get native 'sqlite3' DB handle"));
      }
    }

    return true;
  }
}

QSqlDatabase SqliteDriver::connection(const QString& connection_name, DesiredStorageType desired_type) {
  bool want_in_memory = desired_type == DatabaseDriver::DesiredStorageType::StrictlyInMemory ||
                        (desired_type == DatabaseDriver::DesiredStorageType::FromSettings && m_inMemoryDatabase);

  if ((want_in_memory && !m_inMemoryDatabaseInitialized) || (!want_in_memory && !m_fileBasedDatabaseInitialized)) {
    return initializeDatabase(connection_name, want_in_memory);
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

      if (want_in_memory) {
        database.setConnectOptions(QSL("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE;QSQLITE_ENABLE_REGEXP"));
        database.setDatabaseName(QSL("file::memory:"));
      }
      else {
        const QDir db_path(m_databaseFilePath);
        QFile db_file(db_path.absoluteFilePath(QSL(APP_DB_SQLITE_FILE)));

        database.setConnectOptions(QSL("QSQLITE_ENABLE_SHARED_CACHE;QSQLITE_ENABLE_REGEXP"));
        database.setDatabaseName(db_file.fileName());
      }
    }

    if (!database.isOpen() && !database.open()) {
      qFatal("SQLite database was NOT opened. Delivered error message: '%s'.", qPrintable(database.lastError().text()));
    }
    else {
      qDebugNN << LOGSEC_DB << "SQLite database connection" << QUOTE_W_SPACE(connection_name) << "to file"
               << QUOTE_W_SPACE(database.databaseName()) << "seems to be established.";
    }

    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
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

QSqlDatabase SqliteDriver::initializeDatabase(const QString& connection_name, bool in_memory) {
  finishRestoration();

  QString db_file_name;

  if (!in_memory) {
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
  }
  else {
    db_file_name = QSL("file::memory:");
  }

  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database;

  database = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);

  if (in_memory) {
    database.setConnectOptions(QSL("QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE;QSQLITE_ENABLE_REGEXP"));
  }
  else {
    database.setConnectOptions(QSL("QSQLITE_ENABLE_SHARED_CACHE;QSQLITE_ENABLE_REGEXP"));
  }

  database.setDatabaseName(db_file_name);

  if (!database.open()) {
    qFatal("SQLite database was NOT opened. Delivered error message: '%s'", qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    setPragmas(query_db);

    // Sample query which checks for existence of tables.
    if (!query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"))) {
      qWarningNN << LOGSEC_DB << "SQLite database is not initialized. Initializing now.";

      try {
        const QStringList statements = prepareScript(APP_SQL_PATH, QSL(APP_DB_SQLITE_INIT));

        for (const QString& statement : statements) {
          query_db.exec(statement);

          if (query_db.lastError().isValid()) {
            throw ApplicationException(query_db.lastError().text());
          }
        }

        setSchemaVersion(query_db, QSL(APP_DB_SCHEMA_VERSION).toInt(), true);
      }
      catch (const ApplicationException& ex) {
        qFatal("Error when running SQL scripts: %s.", qPrintable(ex.message()));
      }

      qDebugNN << LOGSEC_DB << "SQLite database backend should be ready now.";
    }
    else if (!in_memory) {
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
          qDebugNN << LOGSEC_DB << "Database schema was updated from" << QUOTE_W_SPACE(installed_db_schema) << "to"
                   << QUOTE_W_SPACE(APP_DB_SCHEMA_VERSION) << "successully.";
        }
        catch (const ApplicationException& ex) {
          qFatal("Error when updating DB schema from %d: %s.", installed_db_schema, qPrintable(ex.message()));
        }
      }

      qDebugNN << LOGSEC_DB << "File-based SQLite database connection '" << connection_name << "' to file '"
               << QDir::toNativeSeparators(database.databaseName()) << "' seems to be established.";
      qDebugNN << LOGSEC_DB << "File-based SQLite database has version '" << installed_db_schema << "'.";
    }
    else {
      query_db.next();
      qDebugNN << LOGSEC_DB << "SQLite database has version" << QUOTE_W_SPACE_DOT(query_db.value(0).toString());
    }
  }

  if (in_memory) {
    // Loading messages from file-based database.
    QSqlDatabase file_database = connection(objectName(), DatabaseDriver::DesiredStorageType::StrictlyFileBased);
    QSqlQuery copy_contents(database);

    // Attach database.
    copy_contents.exec(QSL("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

    // Copy all stuff.
    QStringList tables;

    if (copy_contents.exec(QSL("SELECT name FROM storage.sqlite_master WHERE type = 'table';"))) {
      while (copy_contents.next()) {
        tables.append(copy_contents.value(0).toString());
      }
    }
    else {
      qFatal("Cannot obtain list of table names from file-based SQLite database.");
    }

    for (const QString& table : tables) {
      copy_contents.exec(QSL("INSERT INTO main.%1 SELECT * FROM storage.%1;").arg(table));
    }

    qDebugNN << LOGSEC_DB << "Copying data from file-based database into working in-memory database.";

    // Detach database and finish.
    copy_contents.exec(QSL("DETACH 'storage'"));

    file_database.close();
    QSqlDatabase::removeDatabase(file_database.connectionName());
  }

  // Everything is initialized now.
  if (in_memory) {
    m_inMemoryDatabaseInitialized = true;
  }
  else {
    m_fileBasedDatabaseInitialized = true;
  }

  return database;
}

QString SqliteDriver::databaseFilePath() const {
  return m_databaseFilePath + QDir::separator() + APP_DB_SQLITE_FILE;
}

void SqliteDriver::setPragmas(QSqlQuery& query) {
  query.exec(QSL("PRAGMA encoding = \"UTF-8\""));
  query.exec(QSL("PRAGMA page_size = 32768"));
  query.exec(QSL("PRAGMA cache_size = 32768"));
  query.exec(QSL("PRAGMA mmap_size = 100000000"));
  query.exec(QSL("PRAGMA count_changes = OFF"));
  query.exec(QSL("PRAGMA temp_store = MEMORY"));
  query.exec(QSL("PRAGMA synchronous = OFF"));
  query.exec(QSL("PRAGMA journal_mode = MEMORY"));
}

qint64 SqliteDriver::databaseDataSize() {
  QSqlDatabase database = connection(metaObject()->className(), DatabaseDriver::DesiredStorageType::FromSettings);
  qint64 result = 1;
  QSqlQuery query(database);

  if (query.exec(QSL("PRAGMA page_count;"))) {
    query.next();
    result *= query.value(0).value<qint64>();
  }
  else {
    return 0;
  }

  if (query.exec(QSL("PRAGMA page_size;"))) {
    query.next();
    result *= query.value(0).value<qint64>();
  }
  else {
    return 0;
  }

  return result;
}

QString SqliteDriver::humanDriverType() const {
  return tr("SQLite (embedded database)");
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

QString SqliteDriver::blob() const {
  return QSL("BLOB");
}

QString SqliteDriver::text() const {
  return QSL("TEXT");
}
