// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/mariadbdriver.h"
#include "database/sqlitedriver.h"
#include "exceptions/applicationexception.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

DatabaseFactory::DatabaseFactory(QObject* parent)
  : QObject(parent), m_dbDriver(nullptr) {
  determineDriver();
}

void DatabaseFactory::removeConnection(const QString& connection_name) {
  qDebugNN << LOGSEC_DB << "Removing database connection '" << connection_name << "'.";
  QSqlDatabase::removeDatabase(connection_name);
}

void DatabaseFactory::determineDriver() {
  m_allDbDrivers = {
    new SqliteDriver(qApp->settings()->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool(), this)
  };

  if (QSqlDatabase::isDriverAvailable(QSL(APP_DB_MYSQL_DRIVER))) {
    m_allDbDrivers.append(new MariaDbDriver(this));
  }

  const QString db_driver = qApp->settings()->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString();

  m_dbDriver = boolinq::from(m_allDbDrivers).firstOrDefault([db_driver](DatabaseDriver* driv) {
    return QString::compare(driv->qtDriverCode(), db_driver, Qt::CaseSensitivity::CaseInsensitive) == 0;
  });

  if (m_dbDriver == nullptr) {
    qFatal("DB driver for '%s' was not found.", qPrintable(db_driver));
  }

  if (m_dbDriver->driverType() != DatabaseDriver::DriverType::SQLite) {
    // Try to setup connection and fallback to SQLite.
    try {
      m_dbDriver->connection(QSL("DatabaseFactory"));
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_DB
                  << "Failed to reach connection to DB source, let's fallback to SQLite:"
                  << QUOTE_W_SPACE_DOT(ex.message());

      MessageBox::show(nullptr,
                       QMessageBox::Icon::Critical,
                       tr("Cannot connect to database"),
                       tr("Connection to your database was not established with error: '%1'. "
                          "Falling back to SQLite.").arg(ex.message()));

      m_dbDriver = boolinq::from(m_allDbDrivers).first([](DatabaseDriver* driv) {
        return driv->driverType() == DatabaseDriver::DriverType::SQLite;
      });
    }
  }
}

DatabaseDriver* DatabaseFactory::driver() const {
  return m_dbDriver;
}

DatabaseDriver* DatabaseFactory::driverForType(DatabaseDriver::DriverType d) const {
  return boolinq::from(m_allDbDrivers).firstOrDefault([d](DatabaseDriver* driv) {
    return driv->driverType() == d;
  });
}

QString DatabaseFactory::escapeQuery(const QString& query) {
  return QString(query)
         .replace(QSL("'"), QSL("''"));

  //.replace(QSL("\""), QSL("\\\""));
}

DatabaseDriver::DriverType DatabaseFactory::activeDatabaseDriver() const {
  return m_dbDriver->driverType();
}
