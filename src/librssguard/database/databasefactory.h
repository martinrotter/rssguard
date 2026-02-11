// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include "database/databasedriver.h"
#include "database/databaseworker.h"

#include <QObject>
#include <QPointer>
#include <QSqlDatabase>

class RSSGUARD_DLLSPEC DatabaseFactory : public QObject {
    Q_OBJECT

  public:
    explicit DatabaseFactory(QObject* parent = nullptr);

    // Removes connection.
    void removeConnection(const QString& connection_name = {});

    // Returns identification of currently active database driver.
    DatabaseDriver::DriverType activeDatabaseDriver() const;

    DatabaseWorker* worker() const;
    DatabaseDriver* driver() const;
    DatabaseDriver* driverForType(DatabaseDriver::DriverType d) const;

    static QString escapeQuery(const QString& query);

  private:
    void determineDriver();

    QList<DatabaseDriver*> m_allDbDrivers;
    DatabaseDriver* m_dbDriver;
    QPointer<DatabaseWorker> m_dbWorker;
};

#endif // DATABASEFACTORY_H
