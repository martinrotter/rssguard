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
    m_fileBasedinitialized(false),
    m_inMemoryInitialized(false) {
  setObjectName("DatabaseFactory");
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
    m_databaseFilePath = qApp->applicationDirPath() +
                         QDir::separator() +
                         QString(APP_DB_PATH);
  }
  else {
    m_databaseFilePath = QDir::homePath() + QDir::separator() +
                         QString(APP_LOW_H_NAME) + QDir::separator() +
                         QString(APP_DB_PATH);
  }
}

QSqlDatabase DatabaseFactory::initializeInMemoryDatabase() {
  QSqlDatabase database = QSqlDatabase::addDatabase(DATABASE_DRIVER);

  database.setDatabaseName(":memory:");

  if (!database.open()) {
    qFatal("In-memory database was NOT opened. Delivered error message: '%s'",
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
      qWarning("Error occurred. In-memory database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_MEMORY);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("In-memory database initialization file '%s' from directory '%s' was not found. In-memory database is uninitialized.",
               APP_DB_INIT_FILE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("In-memory database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_FILE);
        }
      }

      database.commit();
      qDebug("In-memory database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("In-memory database connection seems to be established.");
      qDebug("In-memory database has version '%s'.", qPrintable(query_db.value(0).toString()));
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
  m_inMemoryInitialized = true;

  return database;
}

QSqlDatabase DatabaseFactory::initializeFileBasedDatabase(const QString &connection_name) {
  // Prepare file paths.
  QDir db_path(databaseFilePath());
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
    qFatal("File-based database was NOT opened. Delivered error message: '%s'",
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
      qWarning("Error occurred. File-based database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_FILE);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("Database initialization file '%s' from directory '%s' was not found. File-based database is uninitialized.",
               APP_DB_INIT_FILE,
               qPrintable(APP_MISC_PATH));
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      database.transaction();

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("File-based database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_FILE);
        }
      }

      database.commit();
      qDebug("File-based database backend should be ready now.");
    }
    else {
      query_db.next();

      qDebug("File-based database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("File-based database has version '%s'.", qPrintable(query_db.value(0).toString()));
    }

    query_db.finish();
  }

  // Everything is initialized now.
  m_fileBasedinitialized = true;

  return database;
}


QSqlDatabase DatabaseFactory::connection(const QString &connection_name,
                                         DesiredType desired_type) {
  if (desired_type == DatabaseFactory::StrictlyInMemory ||
      (desired_type == DatabaseFactory::FromSettings && m_inMemoryEnabled)) {
    // We request in-memory database (either user don't care
    // about the type or user overrided it in the settings).
    if (!m_inMemoryInitialized) {
      // It is not initialized yet.
      return initializeInMemoryDatabase();
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
  else {
    // We request file-based database.
    if (!m_fileBasedinitialized) {
      // File-based database is not yet initialised.
      return initializeFileBasedDatabase(connection_name);
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
        // Database connection with this name does not exist
        // yet, add it and set it up.
        database = QSqlDatabase::addDatabase(DATABASE_DRIVER, connection_name);

        QDir db_path(databaseFilePath());
        QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

        // Setup database file path.
        database.setDatabaseName(db_file.fileName());
      }

      if (!database.isOpen() && !database.open()) {
        qFatal("File-based database was NOT opened. Delivered error message: '%s'.",
               qPrintable(database.lastError().text()));
      }
      else {
        qDebug("File-based database connection '%s' to file '%s' seems to be established.",
               qPrintable(connection_name),
               qPrintable(QDir::toNativeSeparators(database.databaseName())));
      }

      return database;
    }
  }
}

void DatabaseFactory::removeConnection(const QString &connection_name) {
  qDebug("Removing database connection '%s'.", qPrintable(connection_name));

  QSqlDatabase::removeDatabase(connection_name);
}

void DatabaseFactory::saveMemoryDatabase() {
  if (!m_inMemoryEnabled) {
    return;
  }

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

void DatabaseFactory::determineInMemoryDatabase() {
  m_inMemoryEnabled = Settings::instance()->value(APP_CFG_GEN, "use_in_memory_db", false).toBool();

  qDebug("Working database source was determined as %s.",
         m_inMemoryEnabled ? "in-memory database" : "file-based database");
}

bool DatabaseFactory::vacuumDatabase() {
  QSqlDatabase database = connection(objectName(), FromSettings);
  QSqlQuery query_vacuum(database);

  return query_vacuum.exec("VACUUM");
}
