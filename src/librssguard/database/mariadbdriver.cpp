// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/mariadbdriver.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>

MariaDbDriver::MariaDbDriver(QObject* parent) : DatabaseDriver(parent), m_databaseInitialized(false) {}

QString MariaDbDriver::ddlFilePrefix() const {
  return QSL("mysql");
}

MariaDbDriver::MariaDbError MariaDbDriver::testConnection(const QString& hostname,
                                                          int port,
                                                          const QString& w_database,
                                                          const QString& username,
                                                          const QString& password) {
  QSqlDatabase database = QSqlDatabase::addDatabase(QSL(APP_DB_MYSQL_DRIVER), QSL(APP_DB_MYSQL_TEST));

  database.setHostName(hostname);
  database.setPort(port);
  database.setUserName(username);
  database.setPassword(password);
  database.setDatabaseName(w_database);

  if (database.open() && !database.lastError().isValid()) {
    QSqlQuery query(QSL("SELECT version();"), database);

    if (!query.lastError().isValid() && query.next()) {
      qDebugNN << LOGSEC_DB << "Checked MySQL database, version is" << QUOTE_W_SPACE_DOT(query.value(0).toString());

      // Connection succeeded, clean up the mess and return OK status.
      database.close();
      return MariaDbError::Ok;
    }
    else {
      database.close();
      return MariaDbError::UnknownError;
    }
  }
  else if (database.lastError().isValid()) {
    auto nat = database.lastError().nativeErrorCode();
    bool nat_converted = false;
    auto nat_int = nat.toInt(&nat_converted);

    if (nat_converted) {
      return static_cast<MariaDbError>(nat_int);
    }
    else {
      qWarningNN << LOGSEC_DB << "Failed to recognize MySQL error code:" << QUOTE_W_SPACE_DOT(nat);

      return MariaDbError::UnknownError;
    }
  }
  else {
    return MariaDbError::UnknownError;
  }
}

QString MariaDbDriver::location() const {
  return QSL("%1/%2").arg(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString(),
                          qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString());
}

QString MariaDbDriver::interpretErrorCode(MariaDbDriver::MariaDbError error_code) const {
  switch (error_code) {
    case MariaDbError::Ok:
      return tr("MySQL server works as expected.");

    case MariaDbError::UnknownDatabase:
      return tr("Selected database does not exist (yet). It will be created. It's okay.");

    case MariaDbError::CantConnect:
    case MariaDbError::ConnectionError:
    case MariaDbError::UnknownHost:
      return tr("No MySQL server is running in the target destination.");

    case MariaDbError::AccessDenied:
      return tr("Access denied. Invalid username or password used.");

    default:
      return tr("Unknown error: '%1'.").arg(int(error_code));
  }
}

QString MariaDbDriver::humanDriverType() const {
  return QSL("MariaDB");
}

QString MariaDbDriver::qtDriverCode() const {
  return QSL(APP_DB_MYSQL_DRIVER);
}

DatabaseDriver::DriverType MariaDbDriver::driverType() const {
  return DatabaseDriver::DriverType::MySQL;
}

bool MariaDbDriver::vacuumDatabase() {
  QSqlDatabase database = connection(objectName());
  QSqlQuery query_vacuum(database);

  return query_vacuum.exec(QSL("OPTIMIZE TABLE Feeds;")) && query_vacuum.exec(QSL("OPTIMIZE TABLE Messages;"));
}

bool MariaDbDriver::saveDatabase() {
  return true;
}

void MariaDbDriver::backupDatabase(const QString& backup_folder, const QString& backup_name) {
  Q_UNUSED(backup_folder)
  Q_UNUSED(backup_name)

  saveDatabase();
}

bool MariaDbDriver::initiateRestoration(const QString& database_package_file) {
  Q_UNUSED(database_package_file)
  return true;
}

bool MariaDbDriver::finishRestoration() {
  return true;
}

qint64 MariaDbDriver::databaseDataSize() {
  QSqlDatabase database = connection(metaObject()->className());
  QSqlQuery query(database);

  query.prepare("SELECT Round(Sum(data_length + index_length), 1) "
                "FROM information_schema.tables "
                "WHERE table_schema = :db "
                "GROUP BY table_schema;");
  query.bindValue(QSL(":db"), database.databaseName());

  if (query.exec() && query.next()) {
    return query.value(0).value<qint64>();
  }
  else {
    return 0;
  }
}

QSqlDatabase MariaDbDriver::initializeDatabase(const QString& connection_name) {
  // Folders are created. Create new QSqlDatabase object.
  QSqlDatabase database = QSqlDatabase::addDatabase(QSL(APP_DB_MYSQL_DRIVER), connection_name);
  const QString database_name = qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString();

  database.setHostName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
  database.setPort(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
  database.setUserName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
  database.setPassword(qApp->settings()->password(GROUP(Database), SETTING(Database::MySQLPassword)).toString());

  if (!database.open()) {
    // NOTE: In this case throw exception and fallback SQL backend will be used.
    throw ApplicationException(database.lastError().text());
  }
  else {
    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    setPragmas(query_db);

    if (!query_db.exec(QSL("USE %1").arg(database_name)) ||
        !query_db.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version'"))) {
      // If no "rssguard" database exists or schema version is wrong, then initialize it.
      qWarningNN << LOGSEC_DB << "Error occurred. MySQL database is not initialized. Initializing now.";

      try {
        const QStringList statements = prepareScript(APP_SQL_PATH, QSL(APP_DB_MYSQL_INIT), database_name);

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

      qDebugNN << LOGSEC_DB << "MySQL database backend should be ready now.";
    }
    else {
      // Database was previously initialized. Now just check the schema version.
      query_db.next();
      const int installed_db_schema = query_db.value(0).toString().toInt();

      if (installed_db_schema < QSL(APP_DB_SCHEMA_VERSION).toInt()) {
        try {
          updateDatabaseSchema(query_db, installed_db_schema, database_name);
          qDebugNN << LOGSEC_DB << "Database schema was updated from" << QUOTE_W_SPACE(installed_db_schema) << "to"
                   << QUOTE_W_SPACE(APP_DB_SCHEMA_VERSION) << "successully.";
        }
        catch (const ApplicationException& ex) {
          qFatal("Error when updating DB schema from %d: %s.", installed_db_schema, qPrintable(ex.message()));
        }
      }
    }

    query_db.finish();
  }

  m_databaseInitialized = true;
  return database;
}

void MariaDbDriver::setPragmas(QSqlQuery& query) {
  query.exec(QSL("SET NAMES 'utf8mb4';"));
  query.exec(QSL("SET CHARACTER SET utf8mb4;"));
}

QSqlDatabase MariaDbDriver::connection(const QString& connection_name,
                                       DatabaseDriver::DesiredStorageType desired_type) {
  Q_UNUSED(desired_type)

  if (!m_databaseInitialized) {
    // Return initialized database.
    return initializeDatabase(connection_name);
  }
  else {
    QSqlDatabase database;

    if (QSqlDatabase::contains(connection_name)) {
      qDebugNN << LOGSEC_DB << "MySQL connection '" << connection_name << "' is already active.";

      // This database connection was added previously, no need to
      // setup its properties.
      database = QSqlDatabase::database(connection_name);
    }
    else {
      // Database connection with this name does not exist
      // yet, add it and set it up.
      database = QSqlDatabase::addDatabase(QSL(APP_DB_MYSQL_DRIVER), connection_name);
      database.setHostName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
      database.setPort(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
      database.setUserName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
      database.setPassword(qApp->settings()->password(GROUP(Database), SETTING(Database::MySQLPassword)).toString());
      database.setDatabaseName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString());
    }

    if (!database.isOpen() && !database.open()) {
      // NOTE: In this case throw exception and fallback SQL backend will be used.
      throw ApplicationException(database.lastError().text());
    }
    else {
      qDebugNN << LOGSEC_DB << "MySQL database connection" << QUOTE_W_SPACE(connection_name) << "to file"
               << QUOTE_W_SPACE(QDir::toNativeSeparators(database.databaseName())) << "seems to be established.";
    }

    QSqlQuery query_db(database);

    query_db.setForwardOnly(true);
    setPragmas(query_db);

    return database;
  }
}

QString MariaDbDriver::autoIncrementPrimaryKey() const {
  return QSL("INTEGER AUTO_INCREMENT PRIMARY KEY");
}

QString MariaDbDriver::blob() const {
  return QSL("MEDIUMBLOB");
}
