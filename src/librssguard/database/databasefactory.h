// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>

#include "database/databasedriver.h"

#include <QSqlDatabase>

class DatabaseFactory : public QObject {
  Q_OBJECT

  public:
    explicit DatabaseFactory(QObject* parent = nullptr);

    // Removes connection.
    void removeConnection(const QString& connection_name = {});

    // Returns identification of currently active database driver.
    DatabaseDriver::DriverType activeDatabaseDriver() const;

    DatabaseDriver* driver() const;
    DatabaseDriver* driverForType(DatabaseDriver::DriverType d) const;

    static QString lastExecutedQuery(const QSqlQuery& query);
    static QString escapeQuery(const QString& query);

  private:
    void determineDriver();

    QList<DatabaseDriver*> m_allDbDrivers;
    DatabaseDriver* m_dbDriver;
};

#endif // DATABASEFACTORY_H
