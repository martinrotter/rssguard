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
    virtual bool vacuumDatabase();
    virtual QString ddlFilePrefix() const;
    virtual bool saveDatabase();
    virtual bool initiateRestoration(const QString& database_package_file);
    virtual bool finishRestoration();
    virtual QSqlDatabase connection(const QString& connection_name);
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

  private:
    QSqlDatabase initializeDatabase(const QString& connection_name);
    void setPragmas(QSqlQuery& query);
    QString databaseFilePath() const;

  private:
    QString m_databaseFilePath;
    bool m_databaseInitialized;
};

#endif // SQLITEDRIVER_H
