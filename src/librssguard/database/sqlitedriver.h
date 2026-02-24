// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLITEDRIVER_H
#define SQLITEDRIVER_H

#include "database/databasedriver.h"

class SqliteDriver : public DatabaseDriver {
    Q_OBJECT

  public:
    explicit SqliteDriver(QObject* parent = nullptr);

    virtual QString location() const;
    virtual DriverType driverType() const;
    virtual void vacuumDatabase();
    virtual QString ddlFilePrefix() const;
    virtual void saveDatabase();
    virtual void initiateRestoration(const QString& database_package_file);
    virtual void finishRestoration();
    virtual QString databaseName() const;
    virtual qint64 databaseDataSize();
    virtual QString version();
    virtual QString humanDriverType() const;
    virtual QString qtDriverCode() const;
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name);
    virtual QString autoIncrementPrimaryKey() const;
    virtual QString foreignKeysEnable() const;
    virtual QString foreignKeysDisable() const;
    virtual QString blob() const;
    virtual QString text() const;
    virtual QString collateNocase() const;

  protected:
    virtual void beforeAddDatabase();
    virtual void afterAddDatabase(QSqlDatabase& database, bool was_initialized);
    virtual void setPragmas(SqlQuery& query);
    virtual void updateDatabaseSchema(QSqlDatabase& db, const QString& database_name = {});

  private:
    QString databaseFilePath() const;

  private:
    QString m_databaseFilePath;
};

#endif // SQLITEDRIVER_H
