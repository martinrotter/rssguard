// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"

#include "database/mariadbdriver.h"
#include "database/sqlitedriver.h"
#include "exceptions/applicationexception.h"
#include "exceptions/sqlexception.h"
#include "gui/messagebox.h"
#include "qtlinq/qtlinq.h"

#include <QDir>
#include <QSqlDriver>
#include <QSqlResult>
#include <QVariant>

DatabaseFactory::DatabaseFactory(const QString& driver_id, const QString& user_data_folder, QObject* parent)
  : QObject(parent), m_dbDriver(nullptr), m_dbWorker(nullptr) {
  determineDriver(driver_id, user_data_folder);
  m_dbWorker = new DatabaseWorker(m_dbDriver);
}

DatabaseFactory::~DatabaseFactory() {
  if (m_dbWorker != nullptr) {
    m_dbWorker->shutdown();
    delete m_dbWorker;
  }
}

void DatabaseFactory::determineDriver(const QString& driver_id, const QString& user_data_folder) {
  const QString sqlite_directory = QDir::cleanPath(user_data_folder + QDir::separator() + QSL(APP_DB_SQLITE_PATH));

  m_allDbDrivers = {new SqliteDriver(sqlite_directory, this)};

  if (QSqlDatabase::isDriverAvailable(QSL(APP_DB_MYSQL_DRIVER))) {
    m_allDbDrivers.append(new MariaDbDriver(this));
  }

  m_dbDriver = qlinq::from(m_allDbDrivers)
                 .firstOrDefault([driver_id](DatabaseDriver* driv) {
                   return QString::compare(driv->qtDriverCode(), driver_id, Qt::CaseSensitivity::CaseInsensitive) == 0;
                 })
                 .value_or(nullptr);

  if (m_dbDriver == nullptr) {
    qFatal("DB driver for '%s' was not found.", qPrintable(driver_id));
  }

  const auto handle_connection_failure = [this](const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Failed to reach connection to DB source:" << QUOTE_W_SPACE_DOT(ex.message());

    if (m_dbDriver->driverType() != DatabaseDriver::DriverType::SQLite) {
      MsgBox::
        show(nullptr,
             QMessageBox::Icon::Critical,
             tr("Cannot connect to database"),
             tr("Connection to your database was not established with error: %1. \n\nMaybe change used database name "
                "in settings and try again. Falling back to SQLite.")
               .arg(ex.message()));

      m_dbDriver = qlinq::from(m_allDbDrivers).first([](DatabaseDriver* driv) {
        return driv->driverType() == DatabaseDriver::DriverType::SQLite;
      });
    }
    else {
      MsgBox::show(nullptr,
                   QMessageBox::Icon::Critical,
                   tr("Cannot connect to database"),
                   tr("Connection to your database was not established with error: %1.").arg(ex.message()));
      std::exit(EXIT_FAILURE);
    }
  };

  // Try to setup connection and fallback to SQLite only for connection failures.
  try {
    m_dbDriver->connection(QSL("DatabaseFactory"));
  }
  catch (const SqlException& ex) {
    if (ex.type() == SqlException::Type::TooOldIncompatibleDbSchema ||
        ex.type() == SqlException::Type::TooNewIncompatibleDbSchema) {
      qCriticalNN << LOGSEC_DB << "Database schema is incompatible:" << QUOTE_W_SPACE_DOT(ex.message());

      MsgBox::show(nullptr,
                   QMessageBox::Icon::Critical,
                   tr("Cannot use database"),
                   tr("Application cannot start because there is a problem with DB: %1.").arg(ex.message()));
      std::exit(EXIT_FAILURE);
    }

    handle_connection_failure(ex);
  }
  catch (const ApplicationException& ex) {
    handle_connection_failure(ex);
  }
}

DatabaseWorker* DatabaseFactory::worker() const {
  return m_dbWorker.data();
}

DatabaseDriver* DatabaseFactory::driver() const {
  return m_dbDriver;
}

DatabaseDriver* DatabaseFactory::driverForType(DatabaseDriver::DriverType d) const {
  return qlinq::from(m_allDbDrivers)
    .firstOrDefault([d](DatabaseDriver* driv) {
      return driv->driverType() == d;
    })
    .value_or(nullptr);
}

QString DatabaseFactory::escapeQuery(const QString& query) {
  return QString(query).replace(QSL("'"), QSL("''"));
}

QString DatabaseFactory::escapeIdentifier(const QString& identifier) {
  return QSL("`%1`").arg(QString(identifier).replace(QL1C('`'), QSL("``")));
}

DatabaseDriver::DriverType DatabaseFactory::activeDatabaseDriver() const {
  return m_dbDriver->driverType();
}
