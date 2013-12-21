#include <QApplication>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/settings.h"


QPointer<DatabaseFactory> DatabaseFactory::s_instance;

DatabaseFactory::DatabaseFactory(QObject *parent)
  : QObject(parent), m_initialized(false) {
  assemblyDatabaseFilePath();
}

DatabaseFactory::~DatabaseFactory() {
  qDebug("Destroying DatabaseFactory object.");
}

DatabaseFactory *DatabaseFactory::getInstance() {
  if (s_instance.isNull()) {
    s_instance = new DatabaseFactory(qApp);
  }

  return s_instance;
}

void DatabaseFactory::assemblyDatabaseFilePath()  {
  if (Settings::getInstance()->type() == Settings::Portable) {
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

QString DatabaseFactory::getDatabasePath() {
  return m_databasePath;
}

QSqlDatabase DatabaseFactory::initialize(const QString &connection_name) {
  // Prepare file paths.
  QDir db_path(getDatabasePath());
  QFile db_file(db_path.absoluteFilePath(APP_DB_FILE));

  // Check if database directory exists.
  if (!db_path.exists()) {
    if (!db_path.mkpath(db_path.absolutePath())) {
      // Failure when create database file path.
      qFatal("Directory for database file '%s' was NOT created."
             "This is HUGE problem.",
             qPrintable(db_file.symLinkTarget()));
    }
  }

  // Folders are created. Create new QSQLDatabase object.
  QSqlDatabase database = QSqlDatabase::addDatabase(DATABASE_DRIVER,
                                                    connection_name);

  // Setup database file path.
  database.setDatabaseName(db_file.fileName());

  if (!database.open()) {
    qFatal("Database was NOT opened. Delivered error message: '%s'",
           qPrintable(database.lastError().text()));
  }
  else {
    QSqlQuery query_db(database);

    // TODO: smazat QSQLDatabase::exec() všude
    // a nahradit jej funkcí QSQLquery::exec()   
    query_db.exec("PRAGMA encoding = \"UTF-8\"");
    query_db.exec("PRAGMA synchronous = OFF");
    query_db.exec("PRAGMA journal_mode = MEMORY");
    query_db.exec("PRAGMA count_changes = OFF");
    query_db.exec("PRAGMA temp_store = MEMORY");

    // Sample query which checks for existence of tables.
    query_db.exec("SELECT value FROM Information WHERE key = 'schema_version'");

    if (query_db.lastError().isValid()) {
      qWarning("Error occurred. Database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_FILE);

      if (!file_init.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Database initialization file not opened. HUGE problem.
        qFatal("Database initialization file '%s' was not found. Database is uninitialized.",
               APP_DB_INIT_FILE);
      }

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT,
                                                                  QString::SkipEmptyParts);
      query_db.exec("BEGIN TRANSACTION");

      foreach(const QString &statement, statements) {
        query_db.exec(statement);

        if (query_db.lastError().isValid()) {
          qFatal("Database initialization failed. Initialization script '%s' is not correct.",
                 APP_DB_INIT_FILE);
        }
      }

      query_db.exec("COMMIT");
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

  return database;
}

QSqlDatabase DatabaseFactory::addConnection(const QString &connection_name) {
  if (!m_initialized) {
    m_initialized = true;
    return initialize(connection_name);
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
      qFatal("Database was NOT opened. Delivered error message: '%s'",
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
