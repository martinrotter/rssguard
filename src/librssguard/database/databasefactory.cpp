// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasefactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/sqlitedriver.h"
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

/*else if (m_activeDatabaseDriver == DatabaseDriver::UsedDriver::MYSQL) {
   QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className(),
                                                       DatabaseDriver::DesiredType::FromSettings);
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
   else {
   return 0;
   }*/

void DatabaseFactory::removeConnection(const QString& connection_name) {
  qDebugNN << LOGSEC_DB << "Removing database connection '" << connection_name << "'.";
  QSqlDatabase::removeDatabase(connection_name);
}

void DatabaseFactory::determineDriver() {
  m_allDbDrivers = {
    new SqliteDriver(qApp->settings()->value(GROUP(Database), SETTING(Database::UseInMemory)).toBool(), this)
  };

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    //m_allDbDrivers.append(new MariaDbDriver(this));
  }

  const QString db_driver = qApp->settings()->value(GROUP(Database), SETTING(Database::ActiveDriver)).toString();

  m_dbDriver = boolinq::from(m_allDbDrivers).firstOrDefault([db_driver](DatabaseDriver* driv) {
    return QString::compare(driv->qtDriverCode(), db_driver, Qt::CaseSensitivity::CaseInsensitive) == 0;
  });

  if (m_dbDriver == nullptr) {
    qFatal("DB driver for '%s' was not found.", qPrintable(db_driver));
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

DatabaseDriver::DriverType DatabaseFactory::activeDatabaseDriver() const {
  return m_dbDriver->driverType();
}
