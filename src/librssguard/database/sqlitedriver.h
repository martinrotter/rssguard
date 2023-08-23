// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLITEDRIVER_H
#define SQLITEDRIVER_H

#include "database/databasedriver.h"

class SqliteDriver : public DatabaseDriver {
  Q_OBJECT

  public:
    explicit SqliteDriver(bool in_memory, QObject* parent = nullptr);

    virtual QString location() const;
    virtual DriverType driverType() const;
    virtual bool vacuumDatabase();
    virtual QString ddlFilePrefix() const;
    virtual bool saveDatabase();
    virtual bool initiateRestoration(const QString& database_package_file);
    virtual bool finishRestoration();
    virtual QSqlDatabase connection(const QString& connection_name,
                                    DatabaseDriver::DesiredStorageType desired_type = DatabaseDriver::DesiredStorageType::FromSettings);
    virtual qint64 databaseDataSize();
    virtual QString humanDriverType() const;
    virtual QString qtDriverCode() const;
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name);
    virtual QString autoIncrementPrimaryKey() const;
    virtual QString blob() const;

  private:
    QSqlDatabase initializeDatabase(const QString& connection_name, bool in_memory);
    void setPragmas(QSqlQuery& query);
    QString databaseFilePath() const;

  private:
    bool m_inMemoryDatabase;
    QString m_databaseFilePath;
    bool m_fileBasedDatabaseInitialized;
    bool m_inMemoryDatabaseInitialized;
};

#endif // SQLITEDRIVER_H
