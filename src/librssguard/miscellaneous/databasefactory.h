// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include <QObject>
#include <QSqlDatabase>

class DatabaseFactory : public QObject {
  Q_OBJECT

  public:

    // Describes available typos of database backend.
    enum class UsedDriver {
      SQLITE,
      SQLITE_MEMORY,
      MYSQL
    };

    // Describes what type of database user wants.
    enum class DesiredType {
      StrictlyFileBased,
      StrictlyInMemory,
      FromSettings
    };

    // Describes possible MySQL-specific errors.
    enum class MySQLError {
      Ok = 0,
      UnknownError = 1,
      AccessDenied = 1045,
      UnknownDatabase = 1049,
      ConnectionError = 2002,
      CantConnect = 2003,
      UnknownHost = 2005
    };

    //
    // GENERAL stuff.
    //

    // Constructor.
    explicit DatabaseFactory(QObject* parent = nullptr);

    // Destructor.
    virtual ~DatabaseFactory() = default;

    // Returns size of DB file.
    qint64 getDatabaseFileSize() const;

    // Returns size of data contained in the DB file.
    qint64 getDatabaseDataSize() const;

    // If in-memory is true, then :memory: database is returned
    // In-memory database is DEFAULT database.
    // NOTE: This always returns OPENED database.
    QSqlDatabase connection(const QString& connection_name, DesiredType desired_type = DesiredType::FromSettings);

    QString humanDriverName(UsedDriver driver) const;
    QString humanDriverName(const QString& driver_code) const;

    // Removes connection.
    void removeConnection(const QString& connection_name = QString());

    QString obtainBeginTransactionSql() const;

    // Performs any needed database-related operation to be done
    // to gracefully exit the application.
    void saveDatabase();

    // Performs cleanup of the database.
    bool vacuumDatabase();

    // Returns identification of currently active database driver.
    UsedDriver activeDatabaseDriver() const;

    // Copies selected backup database (file) to active database path.
    bool initiateRestoration(const QString& database_backup_file_path);

    // Finishes restoration from backup file.
    void finishRestoration();

    //
    // SQLITE stuff.
    //
    QString sqliteDatabaseFilePath() const;

    //
    // MySQL stuff.
    //

    // Tests if database connection with given data
    // can be established and returns 0 if it can.
    // Otherwise returns MySQL-specific error code.
    MySQLError mysqlTestConnection(const QString& hostname, int port, const QString& w_database,
                                   const QString& username, const QString& password);

    // Interprets MySQL error code.
    QString mysqlInterpretErrorCode(MySQLError error_code) const;

  private:

    //
    // GENERAL stuff.
    //

    // Decides which database backend will be used in this
    // application session.
    void determineDriver();

    // Holds the type of currently activated database backend.
    UsedDriver m_activeDatabaseDriver;

    //
    // MYSQL stuff.
    //

    // Returns (always OPENED) MySQL database connection.
    QSqlDatabase mysqlConnection(const QString& connection_name);

    // Initializes MySQL database.
    QSqlDatabase mysqlInitializeDatabase(const QString& connection_name);

    // Updates database schema.
    bool mysqlUpdateDatabaseSchema(const QSqlDatabase& database, const QString& source_db_schema_version, const QString& db_name);

    // Runs "VACUUM" on the database.
    bool mysqlVacuumDatabase();

    // True if MySQL database is fully initialized for use,
    // otherwise false.
    bool m_mysqlDatabaseInitialized;

    //
    // SQLITE stuff.
    //

    QSqlDatabase sqliteConnection(const QString& connection_name, DesiredType desired_type);

    // Runs "VACUUM" on the database.
    bool sqliteVacuumDatabase();

    // Performs saving of items from in-memory database
    // to file-based database.
    void sqliteSaveMemoryDatabase();

    // Assemblies database file path.
    void sqliteAssemblyDatabaseFilePath();

    // Updates database schema.
    bool sqliteUpdateDatabaseSchema(const QSqlDatabase& database, const QString& source_db_schema_version);

    // Creates new connection, initializes database and
    // returns opened connections.
    QSqlDatabase sqliteInitializeInMemoryDatabase();
    QSqlDatabase sqliteInitializeFileBasedDatabase(const QString& connection_name);

    // Path to database file.
    QString m_sqliteDatabaseFilePath;

    // Is database file initialized?
    bool m_sqliteFileBasedDatabaseInitialized;
    bool m_sqliteInMemoryDatabaseInitialized;
};

#endif // DATABASEFACTORY_H
