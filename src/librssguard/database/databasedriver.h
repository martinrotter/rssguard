// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEDRIVER_H
#define DATABASEDRIVER_H

#include <QObject>

#include <QSqlDatabase>
#include <QSqlQuery>

class DatabaseDriver : public QObject {
    Q_OBJECT

  public:
    // Describes available types of database backend.
    enum class DriverType { SQLite, MySQL };

    // Describes what type of database user wants.
    enum class DesiredStorageType { StrictlyFileBased, StrictlyInMemory, FromSettings };

    explicit DatabaseDriver(QObject* parent = nullptr);

    QSqlDatabase threadSafeConnection(const QString& connection_name,
                                      DatabaseDriver::DesiredStorageType desired_type =
                                        DatabaseDriver::DesiredStorageType::FromSettings);

    // API.
    virtual QString location() const = 0;
    virtual QString humanDriverType() const = 0;
    virtual QString qtDriverCode() const = 0;
    virtual QString ddlFilePrefix() const = 0;
    virtual DriverType driverType() const = 0;
    virtual QString autoIncrementPrimaryKey() const = 0;
    virtual QString blob() const = 0;
    virtual bool vacuumDatabase() = 0;
    virtual bool saveDatabase() = 0;
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name) = 0;
    virtual bool initiateRestoration(const QString& database_package_file) = 0;
    virtual bool finishRestoration() = 0;
    virtual qint64 databaseDataSize() = 0;
    virtual QSqlDatabase connection(const QString& connection_name,
                                    DatabaseDriver::DesiredStorageType desired_type =
                                      DatabaseDriver::DesiredStorageType::FromSettings) = 0;

  protected:
    void updateDatabaseSchema(QSqlQuery& query, int source_db_schema_version, const QString& database_name = {});

    void setSchemaVersion(QSqlQuery& query, int new_schema_version, bool empty_table);

    QStringList prepareScript(const QString& base_sql_folder,
                              const QString& sql_file,
                              const QString& database_name = {});
};

#endif // DATABASEDRIVER_H
