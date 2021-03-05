// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>

#include "database/databasedriver.h"

#include <QSqlDatabase>

class DatabaseFactory : public QObject {
  Q_OBJECT

  public:

    // Describes possible MySQL-specific errors.
    enum class MySQLError {
      Ok = 0,
      UnknownError = 1,
      AccessDenied = 1045,
      UnknownDatabase = 1049,
      ConnectionError = 2002,
      CantConnect = 2003,
      UnknownHost = 2005
    };

    // Constructor.
    explicit DatabaseFactory(QObject* parent = nullptr);

    // Removes connection.
    void removeConnection(const QString& connection_name = {});

    // Returns identification of currently active database driver.
    DatabaseDriver::DriverType activeDatabaseDriver() const;

    DatabaseDriver* driver() const;
    DatabaseDriver* driverForType(DatabaseDriver::DriverType d) const;

  private:
    void determineDriver();

    QList<DatabaseDriver*> m_allDbDrivers;
    DatabaseDriver* m_dbDriver;
};

#endif // DATABASEFACTORY_H
