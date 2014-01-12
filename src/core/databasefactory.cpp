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
  : QObject(parent), m_fileBasedinitialized(false), m_inMemoryInitialized(false) {
  assemblyDatabaseFilePath();
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

void DatabaseFactory::assemblyDatabaseFilePath()  {
  if (Settings::instance()->type() == Settings::Portable) {
    m_databasePath = qApp->applicationDirPath() +
                     QDir::separator() +
                     QString(APP_DB_PATH);
  }
  else {
    m_databasePath = QDir::homePath() + QDir::separator() +
                     QString(APP_LOW_H_NAME) + QDir::separator() +
                     QString(APP_DB_PATH);
  }
}

QSqlDatabase DatabaseFactory::initializeInMemory() {
  QSqlDatabase database = QSqlDatabase::addDatabase(DATABASE_DRIVER);

  database.setDatabaseName(":memory:");

  if (!database.open()) {
    qFatal("Database was NOT opened. Delivered error message: '%s'",
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
    query_db.exec("SELECT value FROM Information WHERE key = 'schema_version'");

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. Database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_MEMORY);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("Database initialization file '%s' from directory '%s' was not found. Database is uninitialized.",
               APP_DB_INIT_FILE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("Database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_FILE);
        }
      }

      database.commit();
      qDebug("Database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("In-memory database connection seems to be established.");
      qDebug("Database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    // Loading messages from file-based database.
    QSqlDatabase file_database = connection("fdb", false);

    QSqlQuery copy_msgs(database);

    // Attach database.
    copy_msgs.exec(QString("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

    // Copy all stuff.
    QStringList tables; tables << "Categories" << "Feeds" << "FeedsData" <<
                                  "Messages";

    foreach (const QString &table, tables) {
      copy_msgs.exec(QString("INSERT INTO main.%1 SELECT * FROM storage.%1;").arg(table));
    }

    // Detach database and finish.
    copy_msgs.exec("DETACH 'storage'");
    copy_msgs.finish();

    // DB is attached.

    query_db.finish();
  }

  // Everything is initialized now.
  m_inMemoryInitialized = true;

  return database;
}

QString DatabaseFactory::getDatabasePath() {
  return m_databasePath;
}

// TODO: :memory: database je rychllejsi, overit
// na windows, a udelat to takto:
// vsechny connectiony v aplikaci budou defaultní (bez connection_name)
// a budou používat :memory: databazi (problem s vlakny?)
// na zacatku aplikace se kompletni souborova
// databaze presype do :memory: databaze
// a pri vypinani se zase :memory: presype do
// souborove databaze

QSqlDatabase DatabaseFactory::initializeFileBased(const QString &connection_name,
                                                  bool in_memory) {
  // Prepare file paths.
  QDir db_path(getDatabasePath());
  QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

  // Check if database directory exists.
  if (!db_path.exists()) {
    if (!db_path.mkpath(db_path.absolutePath())) {
      // Failure when create database file path.
      qFatal("Directory '%s' for database file '%s' was NOT created."
             "This is HUGE problem.",
             qPrintable(db_path.absolutePath()),
             qPrintable(db_file.symLinkTarget()));
    }
  }

  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database;

  database = QSqlDatabase::addDatabase(DATABASE_DRIVER,
                                       connection_name);
  database.setDatabaseName(db_file.fileName());

  if (!database.open()) {
    qFatal("Database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    query_db.exec("PRAGMA encoding = \"UTF-8\"");
    query_db.exec("PRAGMA synchronous = OFF");
    query_db.exec("PRAGMA journal_mode = MEMORY");
    // TODO: prozkoumat cache a page size a lockingmode co to je
    query_db.exec("PRAGMA page_size = 4096");
    query_db.exec("PRAGMA cache_size = 16384");
    query_db.exec("PRAGMA count_changes = OFF");
    query_db.exec("PRAGMA temp_store = MEMORY");

    // Sample query which checks for existence of tables.
    query_db.exec("SELECT value FROM Information WHERE key = 'schema_version'");

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. Database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_FILE);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("Database initialization file '%s' from directory '%s' was not found. Database is uninitialized.",
               APP_DB_INIT_FILE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("Database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_FILE);
        }
      }

      database.commit();
      qDebug("Database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("Database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("Database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    query_db.finish();
  }

  // Everything is initialized now.
  m_fileBasedinitialized = true;

  return database;
}

QSqlDatabase DatabaseFactory::connection(const QString &connection_name,
                                         bool in_memory) {
  if (in_memory) {
    // We request in-memory database.
    if (!m_inMemoryInitialized) {
      // It is not initialized yet.
      return initializeInMemory();
    }
    else {
      QSqlDatabase database = QSqlDatabase::database();

      database.setDatabaseName(":memory:");

      if (!database.isOpen() && !database.open()) {
        qFatal("In-memory database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("In-memory database connection seems to be established.");
      }

      return database;
    }
  }

  if (!m_fileBasedinitialized) {
    // File-based database is not yet initialised.
    return initializeFileBased(connection_name, in_memory);
  }
  else {
    QSqlDatabase database;

    if (QSqlDatabase::contains(connection_name)) {
      qDebug("Connection '%s' is already active.",
             qPrintable(connection_name));

      // This database connection was added previously, no need to
      // setup its properties.
      database = QSqlDatabase::database(connection_name);
    }
    else {
      database = QSqlDatabase::addDatabase(DATABASE_DRIVER, connection_name);

      QDir db_path(getDatabasePath());
      QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

      // Setup database file path.
      database.setDatabaseName(db_file.fileName());
    }

    if (!database.isOpen() && !database.open()) {
      qFatal("Database was NOT opened. Delivered error message: '%s'.",
             qPrintable(database.lastError().text()));
    }
    else {
      qDebug("Database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
    }

    return database;
  }
}

void DatabaseFactory::removeConnection(const QString &connection_name) {
  qDebug("Removing database connection '%s'.", qPrintable(connection_name));
  QSqlDatabase::removeDatabase(connection_name);
}

void DatabaseFactory::saveMemoryDatabase() {
  QSqlDatabase database = connection();
  QSqlDatabase file_database = connection("fdb", false);

  QSqlQuery copy_msgs(database);

  // Attach database.
  copy_msgs.exec(QString("ATTACH DATABASE '%1' AS 'storage';").arg(file_database.databaseName()));

  // Copy all stuff.
  QStringList tables; tables << "Categories" << "Feeds" << "FeedsData" <<
                                "Messages";

  foreach (const QString &table, tables) {
    copy_msgs.exec(QString("DELETE FROM storage.%1;").arg(table));
    copy_msgs.exec(QString("INSERT INTO storage.%1 SELECT * FROM main.%1;").arg(table));
  }

  // Detach database and finish.
  copy_msgs.exec("DETACH 'storage'");
  copy_msgs.finish();
}
