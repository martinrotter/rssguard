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

    MariaDbError testConnection(const QString& hostname,
                                int port,
                                const QString& w_database,
                                const QString& username,
                                const QString& password);

    virtual QString location() const;
    virtual QString humanDriverType() const;
    virtual QString qtDriverCode() const;
    virtual QString ddlFilePrefix() const;
    virtual DriverType driverType() const;
    virtual void vacuumDatabase();
    virtual void saveDatabase();
    virtual QString databaseName() const;
    virtual QString version();
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name);
    virtual void initiateRestoration(const QString& database_package_file);
    virtual void finishRestoration();
    virtual qint64 databaseDataSize();
    virtual QString foreignKeysEnable() const;
    virtual QString foreignKeysDisable() const;
    virtual QString autoIncrementPrimaryKey() const;
    virtual QString blob() const;
    virtual QString text() const;
    virtual QString collateNocase() const;
    virtual QString limitOffset(int limit, int offset = 0) const;

    QString interpretErrorCode(MariaDbError error_code) const;

  protected:
    virtual void beforeAddDatabase();
    virtual void afterAddDatabase(QSqlDatabase& database, bool was_initialized);
    virtual void setPragmas(SqlQuery& query);
};

#endif // MARIADBDRIVER_H
