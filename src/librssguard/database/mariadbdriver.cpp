// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/mariadbdriver.h"

#include "definitions/definitions.h"
#include "exceptions/sqlexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QSqlError>

MariaDbDriver::MariaDbDriver(QObject* parent) : DatabaseDriver(parent) {}

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
    SqlQuery q(database);

    q.exec(QSL("SELECT version();"), false);

    if (!q.lastError().isValid() && q.next()) {
      qDebugNN << LOGSEC_DB << "Checked MySQL database, version is" << QUOTE_W_SPACE_DOT(q.value(0).toString());

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
                          databaseName());
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

void MariaDbDriver::vacuumDatabase() {
  saveDatabase();

  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    SqlQuery query_vacuum(db);

    query_vacuum.exec(QSL("OPTIMIZE TABLE Feeds;"));
    query_vacuum.exec(QSL("OPTIMIZE TABLE Messages;"));
  });
}

void MariaDbDriver::saveDatabase() {}

QString MariaDbDriver::version() {
  return qApp->database()->worker()->read<QString>([&](const QSqlDatabase& db) {
    SqlQuery q(db);

    q.exec(QSL("SELECT VERSION();"));
    q.next();

    return q.value(0).toString();
  });
}

QString MariaDbDriver::foreignKeysEnable() const {
  return QSL("SET FOREIGN_KEY_CHECKS=1;");
}

QString MariaDbDriver::foreignKeysDisable() const {
  return QSL("SET FOREIGN_KEY_CHECKS=0;");
}

void MariaDbDriver::backupDatabase(const QString& backup_folder, const QString& backup_name) {
  Q_UNUSED(backup_folder)
  Q_UNUSED(backup_name)

  saveDatabase();
}

void MariaDbDriver::initiateRestoration(const QString& database_package_file) {
  Q_UNUSED(database_package_file)
}

void MariaDbDriver::finishRestoration() {}

qint64 MariaDbDriver::databaseDataSize() {
  QSqlDatabase database = connection(metaObject()->className());
  SqlQuery query(database);

  query.prepare("SELECT ROUND(SUM(data_length + index_length), 1) "
                "FROM information_schema.tables "
                "WHERE table_schema = :db "
                "GROUP BY table_schema;");
  query.bindValue(QSL(":db"), database.databaseName());
  query.exec();

  if (query.next()) {
    return query.value(0).value<qint64>();
  }
  else {
    return 0;
  }
}

void MariaDbDriver::setPragmas(SqlQuery& query) {
  query.exec(QSL("SET NAMES 'utf8mb4';"));
  query.exec(QSL("SET CHARACTER SET utf8mb4;"));
}

void MariaDbDriver::beforeAddDatabase() {}

QString MariaDbDriver::databaseName() const {
  return qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLDatabase)).toString();
}

void MariaDbDriver::afterAddDatabase(QSqlDatabase& database, bool was_initialized) {
  const QString database_name = databaseName();

  database.setHostName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLHostname)).toString());
  database.setPort(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLPort)).toInt());
  database.setUserName(qApp->settings()->value(GROUP(Database), SETTING(Database::MySQLUsername)).toString());
  database.setPassword(qApp->settings()->password(GROUP(Database), SETTING(Database::MySQLPassword)).toString());

  if (!was_initialized) {
    if (!database.open()) {
      THROW_EX(SqlException, database.lastError());
    }

    // Ensure the DB exists.
    SqlQuery query_db(database);

    if (!query_db.exec(QSL("USE %1;").arg(database_name), false)) {
      const QStringList statements =
        prepareScript(APP_SQL_PATH, QSL(APP_DB_INIT_FILE_PATTERN).arg(ddlFilePrefix()), database_name);

      // Only create DB, exec "CREATE DATABASE" command.
      query_db.exec(statements.at(1));
      query_db.finish();
    }
  }

  database.setDatabaseName(database_name);
}

QString MariaDbDriver::autoIncrementPrimaryKey() const {
  return QSL("INTEGER AUTO_INCREMENT PRIMARY KEY");
}

QString MariaDbDriver::blob() const {
  return QSL("MEDIUMBLOB");
}

QString MariaDbDriver::text() const {
  return QSL("MEDIUMTEXT");
}

QString MariaDbDriver::collateNocase() const {
  // NOTE: We do not explicitly specify collate for column as it is specified for the whole database.
  return QString();
}

QString MariaDbDriver::limitOffset(int limit, int offset) const {
  if (offset > 0 && limit <= 0) {
    return QSL("LIMIT 184467440737095 OFFSET %1").arg(QString::number(offset));
  }
  else {
    return DatabaseDriver::limitOffset(limit, offset);
  }
}
