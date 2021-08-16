// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MARIADBDRIVER_H
#define MARIADBDRIVER_H

#include "database/databasedriver.h"

class MariaDbDriver : public DatabaseDriver {
  Q_OBJECT

  public:
    enum class MariaDbError {
      Ok = 0,
      UnknownError = 1,
      AccessDenied = 1045,
      UnknownDatabase = 1049,
      ConnectionError = 2002,
      CantConnect = 2003,
      UnknownHost = 2005
    };

    explicit MariaDbDriver(QObject* parent = nullptr);

    MariaDbError testConnection(const QString& hostname, int port, const QString& w_database,
                                const QString& username, const QString& password);

    virtual QString location() const;
    virtual QString humanDriverType() const;
    virtual QString qtDriverCode() const;
    virtual DriverType driverType() const;
    virtual bool vacuumDatabase();
    virtual bool saveDatabase();
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name);
    virtual bool initiateRestoration(const QString& database_package_file);
    virtual bool finishRestoration();
    virtual qint64 databaseDataSize();
    virtual QSqlDatabase connection(const QString& connection_name,
                                    DatabaseDriver::DesiredStorageType desired_type = DatabaseDriver::DesiredStorageType::FromSettings);
    virtual QString autoIncrementPrimaryKey() const;
    virtual QString blob() const;

    QString interpretErrorCode(MariaDbError error_code) const;

  private:
    bool updateDatabaseSchema(const QSqlDatabase& database,
                              const QString& source_db_schema_version,
                              const QString& database_name);
    QSqlDatabase initializeDatabase(const QString& connection_name);

  private:
    bool m_databaseInitialized;
};

#endif // MARIADBDRIVER_H
