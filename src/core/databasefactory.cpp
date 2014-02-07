#include "core/databasefactory.h"

#include "core/defs.h"
#include "core/settings.h"

#include <QApplication>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>


QPointer<DatabaseFactory> DatabaseFactory::s_instance;

DatabaseFactory::DatabaseFactory(QObject *parent)
  : QObject(parent),
    m_mysqlDatabaseInitialized(false),
    m_sqliteFileBasedDatabaseinitialized(false),
    m_sqliteInMemoryDatabaseInitialized(false) {
  setObjectName("DatabaseFactory");
  determineDriver();
}

DatabaseFactory::~DatabaseFactory() {
  qDebug("Destroying DatabaseFactory object.");
}

DatabaseFactory *DatabaseFactory::instance() {
  if (s_instance.isNull()) {
    s_instance = new DatabaseFactory(qApp);
  }

  return s_instance;
}

void DatabaseFactory::sqliteAssemblyDatabaseFilePath()  {
  if (Settings::instance()->type() == Settings::Portable) {
    m_sqliteDatabaseFilePath = qApp->applicationDirPath() +
                               QDir::separator() +
                               QString(APP_DB_PATH);
  }
  else {
    m_sqliteDatabaseFilePath = QDir::homePath() + QDir::separator() +
                               QString(APP_LOW_H_NAME) + QDir::separator() +
                               QString(APP_DB_PATH);
  }
}

QSqlDatabase DatabaseFactory::sqliteInitializeInMemoryDatabase() {
  QSqlDatabase database = QSqlDatabase::addDatabase(APP_DB_DRIVER_SQLITE);

  database.setDatabaseName(":memory:");

  if (!database.open()) {
    qFatal("In-memory SQLite database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    query_db.exec("PRAGMA encoding = \"UTF-8\"");
    query_db.exec("PRAGMA synchronous = OFF");
    query_db.exec("PRAGMA journal_mode = MEMORY");
    query_db.exec("PRAGMA page_size = 4096");
    query_db.exec("PRAGMA cache_size = 16384");
    query_db.exec("PRAGMA count_changes = OFF");
    query_db.exec("PRAGMA temp_store = MEMORY");

    // Sample query which checks for existence of tables.
    query_db.exec("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'");

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. In-memory SQLite database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_SQLITE_MEMORY);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("In-memory SQLite database initialization file '%s' from directory '%s' was not found. In-memory database is uninitialized.",
               APP_DB_INIT_SQLITE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("In-memory SQLite database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_SQLITE);
        }
      }

      database.commit();
      qDebug("In-memory SQLite database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("In-memory SQLite database connection seems to be established.");
      qDebug("In-memory SQLite database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    // Loading messages from file-based database.
    QSqlDatabase file_database = connection(objectName(), StrictlyFileBased);
    QSqlQuery copy_contents(database);

    // Attach database.
    copy_contents.exec(QString("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

    // Copy all stuff.
    QStringList tables; tables << "Information" << "Categories" <<
                                  "Feeds" << "FeedsData" <<
                                  "Messages";

    foreach (const QString &table, tables) {
      copy_contents.exec(QString("INSERT INTO main.%1 SELECT * FROM storage.%1;").arg(table));
    }

    // Detach database and finish.
    copy_contents.exec("DETACH 'storage'");
    copy_contents.finish();

    query_db.finish();
  }

  // Everything is initialized now.
  m_sqliteInMemoryDatabaseInitialized = true;

  return database;
}

QSqlDatabase DatabaseFactory::sqliteInitializeFileBasedDatabase(const QString &connection_name) {
  // Prepare file paths.
  QDir db_path(m_sqliteDatabaseFilePath);
  QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

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

  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database;

  database = QSqlDatabase::addDatabase(APP_DB_DRIVER_SQLITE,
                                       connection_name);
  database.setDatabaseName(db_file.fileName());

  if (!database.open()) {
    qFatal("File-based SQLite database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    query_db.exec("PRAGMA encoding = \"UTF-8\"");
    query_db.exec("PRAGMA synchronous = OFF");
    query_db.exec("PRAGMA journal_mode = MEMORY");
    query_db.exec("PRAGMA page_size = 4096");
    query_db.exec("PRAGMA cache_size = 16384");
    query_db.exec("PRAGMA count_changes = OFF");
    query_db.exec("PRAGMA temp_store = MEMORY");

    // Sample query which checks for existence of tables.
    query_db.exec("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'");

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. File-based SQLite database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_SQLITE);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("SQLite database initialization file '%s' from directory '%s' was not found. File-based database is uninitialized.",
               APP_DB_INIT_SQLITE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("File-based SQLite database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_SQLITE);
        }
      }

      database.commit();
      qDebug("File-based SQLite database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("File-based SQLite database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("File-based SQLite database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    query_db.finish();
  }

  // Everything is initialized now.
  m_sqliteFileBasedDatabaseinitialized = true;

  return database;
}


QSqlDatabase DatabaseFactory::connection(const QString &connection_name,
                                         DesiredType desired_type) {
  switch (m_activeDatabaseDriver) {
    case MYSQL:
      return mysqlConnection(connection_name);

    case SQLITE:
    case SQLITE_MEMORY:
    default:
      return sqliteConnection(connection_name, desired_type);
  }
}

void DatabaseFactory::removeConnection(const QString &connection_name) {
  qDebug("Removing database connection '%s'.", qPrintable(connection_name));

  QSqlDatabase::removeDatabase(connection_name);
}

void DatabaseFactory::sqliteSaveMemoryDatabase() {
  qDebug("Saving in-memory working database back to persistent file-based storage.");

  QSqlDatabase database = connection(objectName(), StrictlyInMemory);
  QSqlDatabase file_database = connection(objectName(), StrictlyFileBased);
  QSqlQuery copy_contents(database);

  // Attach database.
  copy_contents.exec(QString("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

  // Copy all stuff.
  QStringList tables; tables << "Categories" << "Feeds" << "FeedsData" <<
                                "Messages";

  foreach (const QString &table, tables) {
    copy_contents.exec(QString("DELETE FROM storage.%1;").arg(table));
    copy_contents.exec(QString("INSERT INTO storage.%1 SELECT * FROM main.%1;").arg(table));
  }

  // Detach database and finish.
  copy_contents.exec("DETACH 'storage'");
  copy_contents.finish();
}

void DatabaseFactory::determineDriver() {
  QString db_driver = Settings::instance()->value(APP_CFG_DB, "database_driver", APP_DB_DRIVER_SQLITE).toString();

  if (db_driver == APP_DB_DRIVER_MYSQL && QSqlDatabase::isDriverAvailable(APP_DB_DRIVER_MYSQL)) {
    // User wants to use MySQL and MySQL is actually available. Use it.
    // TODO: Perform username & password check if db is really fine.
    m_activeDatabaseDriver = MYSQL;

    qDebug("Working database source was as MySQL database.");
  }
  else {
    // User wants to use SQLite, which is always available. Check if file-based
    // or in-memory database will be used.
    if (Settings::instance()->value(APP_CFG_DB, "use_in_memory_db", false).toBool()) {
      // Use in-memory SQLite database.
      m_activeDatabaseDriver = SQLITE_MEMORY;

      qDebug("Working database source was determined as SQLite in-memory database.");
    }
    else {
      // Use strictly file-base SQLite database.
      m_activeDatabaseDriver = SQLITE;

      qDebug("Working database source was determined as SQLite file-based database.");
    }

    sqliteAssemblyDatabaseFilePath();
  }
}

QSqlDatabase DatabaseFactory::mysqlConnection(const QString &connection_name) {
  if (!m_mysqlDatabaseInitialized) {
    // Return initialized database.
    return mysqlInitializeDatabase(connection_name);
  }
  else {
    QSqlDatabase database;

    if (QSqlDatabase::contains(connection_name)) {
      qDebug("MySQL connection '%s' is already active.",
             qPrintable(connection_name));

      // This database connection was added previously, no need to
      // setup its properties.
      database = QSqlDatabase::database(connection_name);
    }
    else {
      // Database connection with this name does not exist
      // yet, add it and set it up.
      database = QSqlDatabase::addDatabase(APP_DB_DRIVER_MYSQL, connection_name);

      database.setHostName("localhost");
      database.setPort(3306);
      database.setUserName("root");
      database.setDatabaseName("rssguard");
      //database.setPassword("password");
    }

    if (!database.isOpen() && !database.open()) {
      qFatal("MySQL database was NOT opened. Delivered error message: '%s'.",
             qPrintable(database.lastError().text()));
    }
    else {
      qDebug("MySQL database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
    }

    return database;
  }
}

QSqlDatabase DatabaseFactory::mysqlInitializeDatabase(const QString &connection_name) {
  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database = QSqlDatabase::addDatabase(APP_DB_DRIVER_MYSQL,
                                                    connection_name);

  database.setHostName("localhost");
  database.setPort(3306);
  database.setUserName("root");
  //database.setPassword("password");

  if (!database.open()) {
    qFatal("MySQL database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);

    if (!query_db.exec("USE rssguard") ||
        !query_db.exec("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'")) {
      // If no "rssguard" database exists
      // or schema version is wrong, then initialize it.
      qWarning("Error occurred. MySQL database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_MYSQL);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("MySQL database initialization file '%s' from directory '%s' was not found. File-based database is uninitialized.",
               APP_DB_INIT_MYSQL,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("MySQL database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_MYSQL);
        }
      }

      database.commit();
      qDebug("MySQL database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("MySQL database connection '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("MySQL database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    query_db.finish();
  }

  // Everything is initialized now.
  m_mysqlDatabaseInitialized = true;

  return database;
}

QSqlDatabase DatabaseFactory::sqliteConnection(const QString &connection_name,
                                               DatabaseFactory::DesiredType desired_type) {
  if (desired_type == DatabaseFactory::StrictlyInMemory ||
      (desired_type == DatabaseFactory::FromSettings && m_activeDatabaseDriver == SQLITE_MEMORY)) {
    // We request in-memory database (either user explicitly
    // needs in-memory database or it was enabled in the settings).
    if (!m_sqliteInMemoryDatabaseInitialized) {
      // It is not initialized yet.
      return sqliteInitializeInMemoryDatabase();
    }
    else {
      QSqlDatabase database = QSqlDatabase::database();

      database.setDatabaseName(":memory:");

      if (!database.isOpen() && !database.open()) {
        qFatal("In-memory SQLite database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("In-memory SQLite database connection seems to be established.");
      }

      return database;
    }
  }
  else {
    // We request file-based database.
    if (!m_sqliteFileBasedDatabaseinitialized) {
      // File-based database is not yet initialised.
      return sqliteInitializeFileBasedDatabase(connection_name);
    }
    else {
      QSqlDatabase database;

      if (QSqlDatabase::contains(connection_name)) {
        qDebug("SQLite connection '%s' is already active.",
               qPrintable(connection_name));

        // This database connection was added previously, no need to
        // setup its properties.
        database = QSqlDatabase::database(connection_name);
      }
      else {
        // Database connection with this name does not exist
        // yet, add it and set it up.
        database = QSqlDatabase::addDatabase(APP_DB_DRIVER_SQLITE, connection_name);

        QDir db_path(m_sqliteDatabaseFilePath);
        QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

        // Setup database file path.
        database.setDatabaseName(db_file.fileName());
      }

      if (!database.isOpen() && !database.open()) {
        qFatal("File-based SQLite database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("File-based SQLite database connection '%s' to file '%s' seems to be established.",
               qPrintable(connection_name),
               qPrintable(QDir::toNativeSeparators(database.databaseName())));
      }

      return database;
    }
  }
}

void DatabaseFactory::saveDatabase() {
  switch (m_activeDatabaseDriver) {
    case SQLITE_MEMORY:
      sqliteSaveMemoryDatabase();
      break;

    default:
      break;
  }
}

bool DatabaseFactory::vacuumDatabase() {
  switch (m_activeDatabaseDriver) {
    case SQLITE_MEMORY:
    case SQLITE: {
      QSqlDatabase database = connection(objectName(), FromSettings);
      QSqlQuery query_vacuum(database);

      return query_vacuum.exec("VACUUM");
      break;
    }

    default:
      return false;
  }
}
