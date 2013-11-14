#include <QApplication>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

#include "core/defs.h"
#include "core/databasefactory.h"
#include "core/settings.h"


QPointer<DatabaseFactory> DatabaseFactory::s_instance;

DatabaseFactory::DatabaseFactory(QObject *parent) : QObject(parent) {
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
    m_databasePath = qApp->applicationDirPath() + QDir::separator() + APP_DB_PATH;
  }
  else {
    m_databasePath = QDir::homePath() + QDir::separator() + APP_LOW_H_NAME +
                     QDir::separator() + APP_DB_PATH;
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
    database.exec("PRAGMA encoding = \"UTF-8\"");
    database.exec("PRAGMA synchronous = OFF");
    database.exec("PRAGMA journal_mode = MEMORY");
    database.exec("PRAGMA count_changes = OFF");
    database.exec("PRAGMA temp_store = MEMORY");

    // Sample query which checks for existence of tables.
    QSqlQuery q = database.exec("SELECT value FROM Information WHERE key = 'schema_version'");

    if (q.lastError().isValid()) {
      qWarning("Error occurred. Database is not initialized. Initializing now.");

      QFile file_init(APP_MISC_PATH + QDir::separator() + APP_DB_INIT_FILE);
      file_init.open(QIODevice::ReadOnly | QIODevice::Text);

      QStringList statements = QString(file_init.readAll()).split(APP_DB_INIT_SPLIT);
      database.exec("BEGIN TRANSACTION");

      foreach(QString i, statements) {
        q = database.exec(i);
        if (q.lastError().isValid()) {
          if (q.lastError().number() != -1) {
            break;
          }
        }
      }

      database.exec("COMMIT");
      qDebug("Database backend should be ready now.");
    }
    else {
      q.next();

      qDebug("Database connection '%s' to file '%s' seems to be established.",
             qPrintable(connection_name),
             qPrintable(QDir::toNativeSeparators(database.databaseName())));
      qDebug("Database has version '%s'.", qPrintable(q.value(0).toString()));
    }

    q.finish();
  }

  m_initialized = true;

  return database;
}

QSqlDatabase DatabaseFactory::addConnection(const QString &connection_name) {
  if (!m_initialized) {
    // Initialize database file and return connection if it is not
    // initialized yet.
    return initialize(connection_name);
  }
  else {
    return QSqlDatabase::addDatabase(DATABASE_DRIVER, connection_name);
  }
}

QSqlDatabase DatabaseFactory::getConnection(const QString &connection_name) {
  return QSqlDatabase::database(connection_name);
}

void DatabaseFactory::removeConnection(const QString &connection_name) {
  qDebug("Removing database connection '%s'.", qPrintable(connection_name));
  QSqlDatabase::removeDatabase(connection_name);
}
