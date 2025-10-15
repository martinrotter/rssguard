// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQLITEDRIVER_H
#define SQLITEDRIVER_H

#include "database/databasedriver.h"

#if defined(SYSTEM_SQLITE3)
#include <sqlite3.h>
#else
#include "3rd-party/sqlite/sqlite3.h"
#endif

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
                                    DatabaseDriver::DesiredStorageType desired_type =
                                      DatabaseDriver::DesiredStorageType::FromSettings);
    virtual qint64 databaseDataSize();
    virtual QString humanDriverType() const;
    virtual QString qtDriverCode() const;
    virtual void backupDatabase(const QString& backup_folder, const QString& backup_name);
    virtual QString autoIncrementPrimaryKey() const;
    virtual QString foreignKeysEnable() const;
    virtual QString foreignKeysDisable() const;
    virtual QString blob() const;
    virtual QString text() const;
    virtual QString collateNocase() const;

  private:
    QSqlDatabase initializeDatabase(const QString& connection_name, bool in_memory);
    void setPragmas(QSqlQuery& query);
    QString databaseFilePath() const;

    // Uses native "sqlite3" handle to save or load in-memory DB from/to file.
    int loadOrSaveDbInMemoryDb(sqlite3* in_memory_db, const char* db_filename, bool save);

  private:
    bool m_inMemoryDatabase;
    QString m_databaseFilePath;
    bool m_fileBasedDatabaseInitialized;
    bool m_inMemoryDatabaseInitialized;
};

#endif // SQLITEDRIVER_H
