// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEDRIVER_H
#define DATABASEDRIVER_H

#include "database/sqlquery.h"

#include <QObject>
#include <QSqlDatabase>

class RSSGUARD_DLLSPEC DatabaseDriver : public QObject {
    Q_OBJECT

  public:
    enum class DriverType {
      SQLite,
      MySQL
    };

    explicit DatabaseDriver(QObject* parent = nullptr);

    QSqlDatabase connection(const QString& connection_name);
    QSqlDatabase threadSafeConnection(const QString& connection_name);

    void setForeignKeyChecksEnabled(const QSqlDatabase& db);
    void setForeignKeyChecksDisabled(const QSqlDatabase& db);

    // API.
    virtual QString location() const = 0;
    virtual QString humanDriverType() const = 0;
    virtual QString qtDriverCode() const = 0;
    virtual QString ddlFilePrefix() const = 0;
    virtual DriverType driverType() const = 0;
    virtual QString autoIncrementPrimaryKey() const = 0;
    virtual QString blob() const = 0;
    virtual QString text() const = 0;
    virtual QString collateNocase() const = 0;
    virtual QString version() = 0;
    virtual QString databaseName() const = 0;
    virtual QString foreignKeysEnable() const = 0;
    virtual QString foreignKeysDisable() const = 0;
    virtual QString limitOffset(int limit, int offset = 0) const;
    virtual void vacuumDatabase() = 0;
    virtual void saveDatabase() = 0;
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name) = 0;
    virtual void initiateRestoration(const QString& database_package_file) = 0;
    virtual void finishRestoration() = 0;
    virtual qint64 databaseDataSize() = 0;

  protected:
    virtual void beforeAddDatabase();
    virtual void afterAddDatabase(QSqlDatabase& database, bool was_initialized);
    virtual void setPragmas(SqlQuery& query);
    virtual void updateDatabaseSchema(QSqlDatabase& db, const QString& database_name = {});

    void setSchemaVersion(SqlQuery& query, int new_schema_version, bool empty_table);

    QStringList prepareScript(const QString& base_sql_folder,
                              const QString& sql_file,
                              const QString& database_name = {});

  private:
    bool m_databaseInitialized;
};

#endif // DATABASEDRIVER_H
